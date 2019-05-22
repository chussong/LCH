///////////////////////////////////////////////////////////////////////////////
// atomic_containers.hpp: provides thread-safe versions of some standard library
// containers, attempting to duplicate their interfaces as much as possible; 
// some major changes are:
// 1. All returns are copies because references can be invalidated at any time.
// 2. empty() and size() are not allowed because their results would immediately
//    become useless.
// 3. swap() is not allowed since there's no safe way to implement it.
//
// Note: the "const"-ness of these containers refers to their contents, so the
// mutexes are marked mutable.
//
// TODO: possibly allow the full range of operations in the following way:
// - Add a member function which gives a unique_lock on the contained mutex
// - Add the unsafe member functions (as well as the safe ones?) with an extra
//   parameter for a reference to a unique_lock. If it contains the mutex for
//   this container, then do the unsafe operation (or do the safe operation 
//   without the container trying to acquire its own lock).
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

#ifndef LCH_ATOMIC_CONTAINERS_HPP
#define LCH_ATOMIC_CONTAINERS_HPP

#include <mutex>
#include <condition_variable>
#include <deque>
#include <queue>
#include <stdexcept>

namespace LCH {

// Provides an atomic version of std::queue<T>, subject to the differences
// described above; also, this container is bounds-checked and throws
// std::out_of_range if the contents are accessed while it's empty.
//
// An important difference from std::queue: AtomicQueue::pop returns the value
// that was popped from the queue, so if you plan to read and pop a value you
// should do it this way as one operation.
//
// Also, front() and back() return copies of the elements at the front and back,
// since someone else could mess with a reference while you're holding it.
//
// WARNING: don't allow this to be deleted (e.g. by going out of scope) while
// someone is waiting on it. It's probably a good idea to give a shared pointer
// to *this to any object which will wait on it.
template<class T, class Container=std::deque<T>>
class AtomicQueue {
  private:
    using Lock = std::lock_guard<std::mutex>;
    using ULock = std::unique_lock<std::mutex>;

  public:
    T front() const { 
        ULock lock(data_mutex); 
        if (data.empty()) {
            data_cv.wait(lock, [this](){ return !data.empty(); });
        }
        T output = data.front();
        lock.unlock();
        data_cv.notify_one();
        return output;
    }
    T back() const { 
        ULock lock(data_mutex); 
        if (data.empty()) {
            data_cv.wait(lock, [this](){ return !data.empty(); });
        }
        T output = data.back();
        lock.unlock();
        data_cv.notify_one();
        return output;
    }

    void push(const T& value) { 
        {
            Lock lock(data_mutex); 
            data.push(value); 
        }
        data_cv.notify_one();
    }
    void push(T&& value) { 
        {
            Lock lock(data_mutex); 
            data.push(std::move(value)); 
        }
        data_cv.notify_one();
    }

    template<class... Args>
    decltype(auto) emplace(Args&&... args) { 
        {
            Lock lock(data_mutex); 
            data.emplace(std::forward<Args>(args)...); 
        }
        data_cv.notify_one();
    }

    // atomic pop removes the front entry and returns it all at once; it is not
    // exception safe unless T has a noexcept move constructor
    T pop() { 
        ULock lock(data_mutex); 
        if (data.empty()) {
            data_cv.wait(lock, [this](){ return !data.empty(); });
        }
        T output = std::move(data.front());
        data.pop(); 
        return output;
    }

    void clear() {
        Lock lock(data_mutex);
        data = {};
    }

    friend bool operator==(const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data == rhs.data;
    }
    friend bool operator!=(const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data != rhs.data;
    }
    friend bool operator< (const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data < rhs.data;
    }
    friend bool operator<=(const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data <= rhs.data;
    }
    friend bool operator> (const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data > rhs.data;
    }
    friend bool operator>=(const AtomicQueue& lhs, const AtomicQueue& rhs) {
        std::lock(lhs.data_mutex, rhs.data_mutex);
        Lock lock1(lhs.data_mutex, std::adopt_lock);
        Lock lock2(rhs.data_mutex, std::adopt_lock);
        return lhs.data >= rhs.data;
    }

  private:
    std::queue<T, Container> data;
    mutable std::mutex data_mutex;
    mutable std::condition_variable data_cv;
};

} // namespace LCH

#endif // LCH_ATOMIC_CONTAINERS_HPP
