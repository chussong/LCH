#include "thread_pool.hpp"

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

#include "Catch2/catch.hpp"

#include <vector>
#include <condition_variable>
#include <atomic>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::this_thread::sleep_for

int Add(const std::vector<int>& args) {
    int total = 0;
    for (int x : args) {
        total += x;
    }

    return total;
}

int AddLater(const std::vector<int>& args, const std::atomic<bool>& go,
             std::mutex& mutex, std::condition_variable& cv,
             std::atomic<std::size_t>& waiting) {
    std::unique_lock<std::mutex> lock(mutex);
    ++waiting;
    cv.wait(lock, [&](){ return go.load(); });
    --waiting;
    return Add(args);
}

const std::vector<std::vector<int>> testCases {
    {1, 2, 3},
    {62, 88234, 462234, 1241511},
    {},
    {88},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

TEST_CASE("thread_pool works when passed lambdas", "[thread_pool_lambda]") {
    LCH::ThreadPool threadPool(4);

    std::vector<std::future<int>> results;
    std::vector<int> targets;

    SECTION("works with a task that gets copied in") {
        for (const auto& testCase : testCases) {
            auto task = [testCase](){ return Add(testCase); };
            results.push_back(threadPool.AddTask(task));
        }
    }

    SECTION("works with a task that gets moved in") {
        for (const auto& testCase : testCases) {
            auto task = [&testCase](){ return Add(testCase); };
            results.push_back(threadPool.AddTask(std::move(task)));
        }
    }

    for (const auto& testCase : testCases) {
        targets.push_back(Add(testCase));
    }

    REQUIRE(targets.size() == results.size());
    for (std::size_t i = 0; i < targets.size(); ++i) {
        REQUIRE(targets[i] == results[i].get());
    }
}

TEST_CASE("thread_pool can be manually shut down", "[thread_pool_shutdown]") {
    LCH::ThreadPool threadPool(4);

    std::vector<std::future<int>> results;
    std::vector<int> targets;

    for (std::size_t i = 0; i < testCases.size()/2; ++i) {
        auto task = [tc = testCases[i]](){ return Add(tc); };
        results.push_back(threadPool.AddTask(task));
    }
    threadPool.WaitUntilFinished();
    REQUIRE_THROWS(results.push_back(threadPool.AddTask([](){ return 0; })));

    threadPool.Restart(4);
    for (std::size_t i = testCases.size()/2; i < testCases.size(); ++i) {
        auto task = [tc = testCases[i]](){ return Add(tc); };
        results.push_back(threadPool.AddTask(task));
    }
    threadPool.WaitUntilFinished();
    REQUIRE_THROWS(results.push_back(threadPool.AddTask([](){ return 0; })));

    for (const auto& testCase : testCases) {
        targets.push_back(Add(testCase));
    }

    // tasks should still be completed
    REQUIRE(targets.size() == results.size());
    for (std::size_t i = 0; i < targets.size(); ++i) {
        REQUIRE(targets[i] == results[i].get());
    }
}

TEST_CASE("thread_pool can be interrupted", "[thread_pool_stop]") {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> go{false};
    std::atomic<std::size_t> waiting{0};

    LCH::ThreadPool threadPool(3);

    std::vector<std::future<int>> results;
    std::vector<int> targets;

    for (std::size_t i = 0; i < testCases.size(); ++i) {
        const auto& tc = testCases[i];
        auto task = [&](){ return AddLater(tc, go, mutex, cv, waiting); };
        results.push_back(threadPool.AddTask(std::move(task)));
    }

    std::size_t timesWaited = 0;
    while (waiting < threadPool.ThreadCount() && timesWaited < 100) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        ++timesWaited;
    }
    threadPool.StopASAP();
    go = true;
    cv.notify_all();

    for (const auto& testCase : testCases) {
        targets.push_back(Add(testCase));
    }

    REQUIRE(targets.size() == results.size());
    // one task per thread should be successfully completed
    for (std::size_t i = 0; i < threadPool.ThreadCount(); ++i) {
        REQUIRE(targets[i] == results[i].get());
    }
    // all the rest should throw std::future_errors
    for (std::size_t i = threadPool.ThreadCount(); i < results.size(); ++i) {
        REQUIRE_THROWS_AS(results[i].get(), std::future_error);
    }
}
