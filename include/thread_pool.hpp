///////////////////////////////////////////////////////////////////////////////
// thread.hpp: contains a generic thread pool class that allows allocation of 
// pools and assignment of tasks to the contained threads.
//
// This uses a templated-struct-based method for storing tasks rather than
// std::function because std::function is required to be copyable; ThreadPool
// should have no trouble with tasks that contain move-only internal state.
//
// The default error handler will simply rethrow any exceptions, which will
// probably call terminate(). You can set a different exception handler by
// calling SetExceptionHandler, but this is actually rarely used -- exceptions
// thrown by the Tasks themselves will simply be placed into the std::future
// returned by AddTask.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 by Joyz Inc of Tokyo, Japan (author: Charles Hussong) //
//                                                                           //
// Licensed under the Apache License, Version 2.0 (the "License");           //
// you may not use this file except in compliance with the License.          //
// You may obtain a copy of the License at                                   //
//                                                                           //
//    ../LICENSE (if this file is in the official LCH repository)            //
//    https://github.com/joyz-inc/LCH/blob/master/LICENSE                    //
//    http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                           //
// Unless required by applicable law or agreed to in writing, software       //
// distributed under the License is distributed on an "AS IS" BASIS,         //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
// See the License for the specific language governing permissions and       //
// limitations under the License.                                            //
///////////////////////////////////////////////////////////////////////////////

#ifndef LCH_THREAD_POOL_HPP
#define LCH_THREAD_POOL_HPP

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <queue>
#include <memory>
#include <stdexcept>
#include <type_traits> // std::is_invocable
#include <future>

namespace LCH {

class ThreadPool {
  private:
    class AbstractTask;
    class ExceptionHandler;

  public:
    // A default-constructed thread pool contains hardware_concurrency() threads
    explicit ThreadPool(std::size_t threadCount 
                        = std::thread::hardware_concurrency()):
            exceptionHandler(std::make_unique<ExceptionHandler>()), 
            finished(false), terminate(false), waiting(0) {
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(&ThreadPool::WaitForTask, this);
        }
    }

    // ThreadPools are neither movable nor copyable because they contain a 
    // mutex; they wait for all tasks to be completed before they're destroyed. 
    // If you want to end the threads faster than this, manually call 
    // TerminateAfterCurrentTasks.
    virtual ~ThreadPool() { WaitUntilFinished(); }
    
    // Adds a task to the queue and wakes up a thread to work on it. newTask can
    // be any movable type with an operator()(), such as a 0-argument function.
    // Returns a std::future that will receive the function's return value or
    // any exception thrown by it (even void functions should be checked for
    // exceptions)
    template<class Callable>
    auto AddTask(Callable&& newTask) {
        using PTask = std::packaged_task<typename std::result_of<Callable()>::type()>;
        PTask packaged = PTask(std::forward<Callable>(newTask));
        auto futureResult = packaged.get_future();
        AddTaskDirectly(std::make_unique<Task<PTask>>(std::move(packaged)));
        return futureResult;
    }

    template<class Callable>
    void SetExceptionHandler(Callable&& handler) {
        std::lock_guard<std::mutex> lock(exceptionMutex);
        exceptionHandler = std::make_unique<CustomExceptionHandler<Callable>>(
                std::forward<Callable>(handler));
    }

    // Immediately marks the pool as finished, causing a logic_error to be 
    // thrown if you attempt to add any new tasks. Then adds a bunch of empty
    // tasks to wake up the threads and waits for them to finish.
    //
    // Blocks until all threads are destroyed.
    // FIXME: does not necessarily block if another thread called this first!
    void WaitUntilFinished() {
        if (finished) return;
        finished = true;
        WakeAndJoinThreads();
    }

    // Immediately marks the pool as finished, as above, but also marks it
    // terminate, so that when a thread finishes a task it will not get another.
    //
    // This is the closest you can come to forcing a thread to terminate without
    // crashing your entire program; if you want more immediate termination than
    // this, you'll need to arrange for the thread itself to return or throw.
    //
    // Blocks until all threads are destroyed.
    // FIXME: does not necessarily block if another thread called this first!
    void TerminateAfterCurrentTasks() {
        if (terminate) return;
        finished = true;
        terminate = true;
        WakeAndJoinThreads();
    }

    // Restart a thread pool that has been shut down (throw a logic error if
    // it's still running).
    void Restart(std::size_t threadCount) {
        if (threads.size() != 0) {
            throw std::logic_error("LCH::ThreadPool::Restart: pool has not been"
                                   "shut down so it can not be restarted");
        }

        terminate = false;
        finished = false;
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(&ThreadPool::WaitForTask, this);
        }
    }

    std::size_t ThreadCount() const { return threads.size(); }
    std::size_t IdleThreadCount() const { return waiting; }
    std::size_t RunningThreadCount() const { return threads.size() - IdleThreadCount(); }
    bool Idle() const { return IdleThreadCount() == threads.size(); }
    bool Running() const { return !Idle(); }

  protected:
    std::unique_ptr<ExceptionHandler> exceptionHandler;
    std::mutex exceptionMutex;

    std::queue<std::unique_ptr<AbstractTask>> tasks;
    std::mutex taskMutex;
    std::condition_variable notifier;

    std::atomic<bool> finished;
    std::atomic<bool> terminate;
    std::atomic<std::size_t> waiting;

    std::vector<std::thread> threads;

    // Each thread loops through this function until encountering an empty task
    // or until the pool is marked terminate, whichever comes first.
    void WaitForTask() {
        std::unique_ptr<AbstractTask> myTask;
        while (!terminate) {
            {
                std::unique_lock<std::mutex> lock(taskMutex);
                if (tasks.empty()) {
                    ++waiting;
                    notifier.wait(lock, [this]{return !tasks.empty();});
                    --waiting;
                }
                myTask = std::move(tasks.front());
                tasks.pop();
            }
            if (myTask->IsEmpty()) break;
            try {
                (*myTask)();
            } catch (...) {
                std::lock_guard<std::mutex> lock(exceptionMutex);
                (*exceptionHandler)();
            }
        }
    }

  private:
    // private class definitions ----------------------------------------------

    class AbstractTask {
      public:
        virtual ~AbstractTask() = default;
        virtual void operator()() = 0;

        virtual bool IsEmpty() const { return false; }
    };

    class EmptyTask : public AbstractTask {
      public:
        void operator()() {}

        bool IsEmpty() const { return true; }
    };

    template<class Callable>
    class Task : public AbstractTask {
      public:
        explicit Task(Callable&& func): func(std::forward<Callable>(func)) {}
        void operator()() { func(); }
      private:
        Callable func;
    };

    class ExceptionHandler {
      public:
        virtual ~ExceptionHandler() = default;
        virtual void operator()() { throw; }
    };

    template<class Callable>
    class CustomExceptionHandler : public ExceptionHandler {
      public:
        explicit CustomExceptionHandler(Callable&& func): 
            func(std::forward<Callable>(func)) {}
        void operator()() { func(); }

      private:
        Callable func;
    };

    // private functions ------------------------------------------------------

    // Move an already-constructed task pointer into the queue.
    void AddTaskDirectly(std::unique_ptr<AbstractTask> newTask) {
        if (finished && !newTask->IsEmpty()) {
            throw std::logic_error("LCH::ThreadPool::AddTaskDirectly: attempted"
                                   " to add a task to a ThreadPool which has "
                                   "been marked as finished");
        }

        {
            std::lock_guard<std::mutex> lock(taskMutex);
            tasks.push(std::move(newTask));
        }
        notifier.notify_one();
    }

    void AddEmptyTask() {
        AddTaskDirectly(std::make_unique<EmptyTask>());
    }

    // Add some empty tasks to wake up the threads; join them when finished,
    // then make sure the task queue is also empty (since there might still be
    // stuff in there storing internal state).
    void WakeAndJoinThreads() {
        for (std::size_t i = 0; i < threads.size(); ++i) AddEmptyTask();
        for (auto& thread : threads) thread.join();
        threads.clear();

        // below code means "tasks = {};" which doesn't compile on clang
        decltype(tasks) emptyTasks;
        std::swap(emptyTasks, tasks);
    }
};

} // namespace LCH

#endif // LCH_THREAD_POOL_HPP
