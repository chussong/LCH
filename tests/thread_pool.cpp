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

int Add(int x) {
    return x;
}

template<typename... Args>
int Add(int x, Args... rest) {
    return x + Add(rest...);
}

int Add(const std::vector<int>& args) {
    int total = 0;
    for (int x : args) {
        total += x;
    }

    return total;
}

const std::vector<std::vector<int>> testCases {
    {1, 2, 3},
    {62, 88234, 462234, 1241511},
    {},
    {88},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

TEST_CASE("thread_pool works when passed lambdas", "[thread_pool]") {
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
