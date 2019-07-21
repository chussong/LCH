///////////////////////////////////////////////////////////////////////////////
// thread_pool.hpp: contains a generic thread pool class that allows allocation 
// of pools and assignment of tasks to the contained threads.
//
// Simply create a thread pool, then add jobs to it with AddTask; adding a task 
// returns a std::future from which you can retrieve the result using 
// std::future::get(). Each task must be invocable in the same way as a 
// 0-argument function (usually it's easiest to do this with a lambda).
//
// LCH::ThreadPool threadPool;
// int a = 7;
// int b = 91;
// std::future<int> futureResult = threadPool.AddTask([a,b](){ return a + b; });
// int result = futureResult.get();
//
// Because jobs are run using std::packaged_task, exceptions thrown by the jobs
// will be packed into the associated std::future and then re-thrown when the 
// std::future is unpacked with get(). Therefore, if a task can throw, you may
// want to get() it inside a try block to catch exceptions from the task.
//
// This uses a templated-struct-based method for storing tasks rather than
// std::function because std::function is required to be copyable; ThreadPool
// should have no trouble with tasks that contain move-only internal state.
//
// !!WARNING!! !!WARNING!! !!WARNING!!
// Because this class contains a bunch of threads, it can't be destroyed until
// they've been joined. This means that the destructor blocks until the threads'
// jobs have finished, and therefore exiting any scope containing a threadPool 
// may block for potentially infinite time. To avoid this, you can call 
// WaitUntilFinished() manually, after which point the destructor will not
// block.
//
// There are only two major sources of exceptions in this class, but they are
// important: first, adding a task may throw due to allocation failures. Second
// and potentially more seriously, any function using a mutex may throw due to
// the OS failing to acquire the necessary lock. Crucially, this means that
// WaitUntilFinished() and therefore also the destructor may throw (terminating
// the entire program in the latter case). If you think you can handle this
// better, call WaitUntilFinished() manually inside a try block and make sure
// the ThreadPool is guaranteed to be shut down before it is destroyed.
// !!WARNING!! !!WARNING!! !!WARNING!!
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 by Joyz Inc of Tokyo, Japan (author: Charles Hussong) //
//                                                                           //
// Licensed under the Apache License, Version 2.0 (the "License");           //
// you may not use this file except in compliance with the License.          //
// You may obtain a copy of the License at                                   //
//                                                                           //
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
#include <future>

namespace LCH {

class ThreadPool {
  private:
    class AbstractTask;

  public:
    // A default-constructed thread pool contains hardware_concurrency() threads
    explicit ThreadPool(std::size_t threadCount 
                        = std::thread::hardware_concurrency()):
            finished(false), noMoreTasks(false), waiting(0) {
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(&ThreadPool::WaitForTask, this);
        }
    }

    // ThreadPools are neither movable nor copyable because they contain a 
    // mutex; they wait for all tasks to be completed before they're destroyed. 
    // If you want to end the threads faster than this, manually call StopASAP.
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

    // Immediately marks the pool as finished, causing a logic_error to be 
    // thrown if you attempt to add any new tasks. Then adds a bunch of empty
    // tasks to wake up the threads and waits for them to finish.
    //
    // Blocks until all threads and tasks are destroyed.
    void WaitUntilFinished() {
        std::lock_guard<std::mutex> threadLock(threadMutex);
        if (threads.empty()) return;

        finished = true;
        notifier.notify_all();
        for (auto& thread : threads) thread.join();
        threads.clear();

        ClearFutureTasks();
    }

    // Immediately marks the pool as finished, as above, but also marks it
    // noMoreTasks, so that when a thread finishes a task it will not get 
    // another. Aborts all existing tasks after the ones currently being 
    // executed; their std::futures will throw std::future_error.
    //
    // This is the closest you can come to forcing a thread to terminate without
    // crashing your entire program; if you want more immediate termination than
    // this, you'll need to arrange for the thread itself to return or throw.
    //
    // Blocks until future tasks can be destroyed, but does not block for 
    // threads to finish executing their current tasks.
    void StopASAP() {
        std::lock_guard<std::mutex> threadLock(threadMutex);
        if (noMoreTasks || threads.empty()) return;

        finished = true;
        noMoreTasks = true;
        notifier.notify_all();

        ClearFutureTasks();
    }

    // Restart a thread pool that has been shut down (throw a logic error if
    // it's still running).
    void Restart(std::size_t threadCount) {
        std::lock_guard<std::mutex> threadLock(threadMutex);
        if (threads.size() != 0) {
            throw std::logic_error("LCH::ThreadPool::Restart: pool has not been"
                                   "shut down so it can not be restarted");
        }

        noMoreTasks = false;
        finished = false;
        for (std::size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back(&ThreadPool::WaitForTask, this);
        }
    }

    std::size_t ThreadCount() const noexcept { 
        return threads.size(); 
    }
    std::size_t IdleThreadCount() const noexcept { 
        return waiting; 
    }
    std::size_t RunningThreadCount() const noexcept { 
        return ThreadCount() - IdleThreadCount(); 
    }
    bool Idle() const noexcept { 
        return ThreadCount() == IdleThreadCount(); 
    }
    bool Running() const noexcept { 
        return !Idle(); 
    }

  protected:
    std::mutex taskMutex;
    std::condition_variable notifier;
    std::queue<std::unique_ptr<AbstractTask>> tasks;

    std::atomic<bool> finished;
    std::atomic<bool> noMoreTasks;
    std::atomic<std::size_t> waiting;

    std::mutex threadMutex;
    std::vector<std::thread> threads;

    // Each thread loops through this function until either the pool is marked
    // noMoreTasks or the task queue is exhausted with the pool marked finished.
    void WaitForTask() {
        std::unique_ptr<AbstractTask> myTask;
        while (true) {
            {
                std::unique_lock<std::mutex> taskLock(taskMutex);
                if (tasks.empty()) {
                    if (finished) break;
                    ++waiting;
                    notifier.wait(taskLock, [this]{return ThreadShouldWake();});
                    --waiting;
                }
                if (noMoreTasks || tasks.empty()) break;
                myTask = std::move(tasks.front());
                tasks.pop();
            }
            (*myTask)();
        }
    }

  private:
    // private class definitions ----------------------------------------------

    class AbstractTask {
      public:
        virtual ~AbstractTask() = default;
        virtual void operator()() = 0;
    };

    template<class Callable>
    class Task : public AbstractTask {
      public:
        explicit Task(Callable&& func): func(std::forward<Callable>(func)) {}
        void operator()() { func(); }
      private:
        Callable func;
    };

    // private functions ------------------------------------------------------

    // A sleeping thread should wake up if it has a task to do or if it should
    // be cleaned up.
    bool ThreadShouldWake() const noexcept {
        return !tasks.empty() || finished || noMoreTasks;
    }

    // Move an already-constructed task pointer into the queue.
    void AddTaskDirectly(std::unique_ptr<AbstractTask> newTask) {
        if (finished) {
            throw std::logic_error("LCH::ThreadPool::AddTaskDirectly: attempted"
                                   " to add a task to a ThreadPool which has "
                                   "been marked as finished");
        }

        {
            std::lock_guard<std::mutex> taskLock(taskMutex);
            tasks.push(std::move(newTask));
        }
        notifier.notify_one();
    }

    // Clear all future tasks not currently being worked on by threads. After
    // clearing, their associated std::futures will throw std::future_error
    // when opened.
    void ClearFutureTasks() {
        std::lock_guard<std::mutex> taskLock(taskMutex);
        // below code means "tasks = {};" which doesn't compile on clang
        decltype(tasks) emptyTasks;
        std::swap(emptyTasks, tasks);
    }
};

} // namespace LCH

#endif // LCH_THREAD_POOL_HPP
