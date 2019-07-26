// math.hpp: generally applicable math functions not found in the stdlib.
//
// For functions which take a standard deviation, there is an argument isSample
// (defaults to true). If the range being passed to the function is a sample of
// a larger population, Bessel's correction must be used in computing the 
// standard deviation; if the range is the entire population, set isSample to
// false to avoid Bessel's correction.

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

#ifndef LCH_MATH_HPP
#define LCH_MATH_HPP

#include <numeric>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <array>
#include <vector>

namespace LCH {

// ***** basic statistical operations on ranges ***** -------------------------

// get the mean value of [begin, end)
template<typename Iterator>
typename std::iterator_traits<Iterator>::value_type Mean(Iterator begin, Iterator end) {
    using T = typename std::iterator_traits<Iterator>::value_type;
    return std::accumulate(begin, end, T()) / std::distance(begin, end);
}

// get the mean value of anything with a begin() and an end()
template<typename Container>
auto Mean(const Container& container) {
    return Mean(container.begin(), container.end());
}

// subtract Mean(begin, end) from everything in [begin, end)
template<typename Iterator>
void SubtractMean(Iterator begin, Iterator end) {
    using T = typename std::iterator_traits<Iterator>::value_type;
    T mean = Mean(begin, end);
    std::transform(begin, end, begin, [mean](const T& val){ return val-mean; });
}

// subtract the mean from everything in a container with a begin() and an end()
template<typename Container>
void SubtractMean(Container& container) {
    SubtractMean(container.begin(), container.end());
}

// if the data in [begin, end) *has zero mean already*, standardize it by 
// dividing by the standard deviation
template<typename Iterator>
void StandardizeFromZeroMean(Iterator begin, Iterator end, bool isSample = true) {
    using T = typename std::iterator_traits<Iterator>::value_type;
    if (std::next(begin) == end) return;

    // if this is a sample, we need Bessel's correction
    T denominator = std::distance(begin, end) - (isSample ? 1 : 0);
    T variance = std::inner_product(begin, end, begin, T()) / denominator;
    T sd = std::sqrt(variance);
    std::transform(begin, end, begin, [sd](const T& val){ return val/sd; });
}

// standardize zero-mean data in a container with a begin() and an end()
template<typename Container>
void StandardizeFromZeroMean(Container& container, bool isSample = true) {
    StandardizeFromZeroMean(container.begin(), container.end(), isSample);
}

// standardize the data in [begin, end) by subtacting the mean and dividing by
// the standard deviation
template<typename Iterator>
void Standardize(Iterator begin, Iterator end, bool isSample = true) {
    if (begin == end) return;

    SubtractMean(begin, end);
    StandardizeFromZeroMean(begin, end, isSample);
}

// standardize the data in a container with a begin() and an end()
template<typename Container>
void Standardize(Container& container, bool isSample = true) {
    Standardize(container.begin(), container.end(), isSample);
}

// get the "standard scores" in the statistical sense of a number of standard
// deviations away from the mean; i.e. copy the data in [begin, end) and
// standardize the copy
template<typename Iterator>
std::vector<typename std::iterator_traits<Iterator>::value_type> 
StandardScores(Iterator begin, Iterator end, bool isSample = true) {
    using T = typename std::iterator_traits<Iterator>::value_type;
    std::vector<T> output;
    output.reserve(std::distance(begin, end));
    std::copy(begin, end, std::back_inserter(output));
    Standardize(output.begin(), output.end(), isSample);
    return output;
}

// get the standard scores of the given container's contents without modifying
template<typename Container>
Container StandardScores(const Container& container, bool isSample = true) {
    Container output;
    std::copy(container.begin(), container.end(), std::back_inserter(output));
    Standardize(output.begin(), output.end(), isSample);
    return output;
}

// partial specialization of above for vectors since they can be reserved
template<typename T>
std::vector<T> StandardScores(const std::vector<T>& container, bool isSample = true) {
    std::vector<T> output;
    output.reserve(container.size());
    std::copy(container.begin(), container.end(), std::back_inserter(output));
    Standardize(output.begin(), output.end(), isSample);
    return output;
}

// get the mean and standard deviation of the data in [begin, end) without
// changing the original data
template<typename Iterator>
std::array<typename std::iterator_traits<Iterator>::value_type,2> 
MeanAndStdDev(Iterator begin, Iterator end, bool isSample = true) {
    using T = typename std::iterator_traits<Iterator>::value_type;
    std::array<T,2> meanAndStdDev {0.0, 0.0};
    if (begin == end) return meanAndStdDev;

    std::vector<T> deviations;
    deviations.reserve(std::distance(begin, end));
    std::copy(begin, end, std::back_inserter(deviations));
    SubtractMean(deviations.begin(), deviations.end());

    T firstValue = deviations[0];
    meanAndStdDev[0] = *begin - firstValue;
    if (deviations.size() <= 1) return meanAndStdDev;

    StandardizeFromZeroMean(deviations.begin(), deviations.end(), isSample);
    meanAndStdDev[1] = firstValue / deviations[0];

    return meanAndStdDev;
}

} // namespace LCH

#endif // LCH_MATH_HPP
