#include "math.hpp"

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

#include <array>
#include <cmath>
#include <iostream>

static constexpr std::array<double,10> input {
    7.96294,
    1.29735,
    -6.91204,
    5.35568,
    6.72322,
    7.95431,
    8.15581,
    -4.10639,
    5.46351,
    -5.17758
};

static constexpr double mean = 2.67168;
static constexpr double stdDev = 5.95056;

static constexpr std::array<double,10> standardized {
    0.889204,
    -0.230958,
    -1.61056,
    0.45105,
    0.680867,
    0.887754,
    0.921617,
    -1.13907,
    0.469171,
    -1.31908
};

bool CloseEnough(double x, double y) {
    static constexpr double tolerance = 1e-4;
    return std::abs(x - y) <= tolerance;
}

template<typename ContainerA, typename ContainerB>
bool CloseEnough(ContainerA x, ContainerB y) {
    if (x.size() != y.size()) return false;

    for (typename ContainerA::size_type i = 0; i < x.size(); ++i) {
        if (!CloseEnough(x[i], y[i])) return false;
    }

    return true;
}

TEST_CASE("math routines work", "[math]") {
    SECTION("math works with iterators") {
        REQUIRE(CloseEnough(LCH::Mean(input.cbegin(), input.cend()), mean));
        std::array<double,10> inputCopy = input;
        LCH::Standardize(inputCopy.begin(), inputCopy.end());
        REQUIRE(CloseEnough(inputCopy, standardized));
        auto standardScores = LCH::StandardScores(input.cbegin(), input.cend());
        REQUIRE(CloseEnough(standardScores, inputCopy));

        auto meanAndStdDev = LCH::MeanAndStdDev(input.cbegin(), input.cend());
        REQUIRE(CloseEnough(meanAndStdDev[0], mean));
        REQUIRE(CloseEnough(meanAndStdDev[1], stdDev));
    }

    SECTION("math works with pointers") {
        REQUIRE(CloseEnough(LCH::Mean(input.data(), input.data() + input.size()), mean));
        std::array<double,10> inputCopy = input;
        LCH::Standardize(inputCopy.data(), inputCopy.data() + inputCopy.size());
        REQUIRE(CloseEnough(inputCopy, standardized));
        auto standardScores = LCH::StandardScores(input.data(), 
                                                  input.data() + input.size());
        REQUIRE(CloseEnough(standardScores, inputCopy));

        auto meanAndStdDev = LCH::MeanAndStdDev(input.data(), 
                                                input.data() + input.size());
        REQUIRE(CloseEnough(meanAndStdDev[0], mean));
        REQUIRE(CloseEnough(meanAndStdDev[1], stdDev));
    }
}
