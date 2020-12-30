// levenshtein.hpp: a templated implementation of the "one-row exhaustive"
// method for computing the Levenshtein edit distance between two iterable
// objects.
//
// The two arguments a and b can be given as iterator pairs like in the standard
// library, or as containers with begin() and end() functions, or as C-style 
// strings. Call like this:
//
// std::size_t dist = LCH::levenshtein_distance(aBegin, aEnd, bBegin, bEnd, costs);
// std::size_t dist = LCH::levenshtein_distance(aBegin, aEnd, b, costs);
// std::size_t dist = LCH::levenshtein_distance(a, bBegin, bEnd, costs);
// std::size_t dist = LCH::levenshtein_distance(a, b, costs);
//
// Costs is an instance of the LevenshteinCosts struct, and can be omitted. The
// default costs are 1 for each of substitution, insertion, and deletion.
//
// The left argument ("a" above) is taken to be the "base", so if b is longer
// there will be insertions and if b is shorter there will be deletions.
//
// WARNING: const char* and char arrays are assumed to represent C-style
// strings, so their final character (assumed to be '\0') is not considered.
// This is so that they can be compared with std::string and std::string_view;
// however, it means that a char[n] containing non-string data will be processed
// incorrectly. Note that all characters in std::array<char, n> are considered.

///////////////////////////////////////////////////////////////////////////////
// Copyright 2020 by Charles Hussong                                         //
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

#include <vector>
#include <tuple>
#include <algorithm> // min
#include <iterator> // distance
#include <type_traits> // enable_if and other template magic

namespace LCH {

struct LevenshteinCosts {
    std::size_t sub = 1;
    std::size_t ins = 1;
    std::size_t del = 1;
};


// is_iterable is from Jarod42's post here https://stackoverflow.com/a/29634934
namespace Detail {
    // To allow ADL with custom begin/end
    using std::begin;
    using std::end;

    // Types which can match this template are iterable.
    template<typename T>
    auto is_iterable_impl(int)
    -> decltype (
        begin(std::declval<T&>()) != end(std::declval<T&>()), // has begin/end and operator !=
        void(), // Handle evil operator ,
        ++std::declval<decltype(begin(std::declval<T&>()))&>(), // has operator ++
        void(*begin(std::declval<T&>())), // has operator*
        std::true_type{});

    // Of types which can't match the above, only const char* is iterable.
    template<typename T>
    std::is_same<T, const char*> is_iterable_impl(...);

    const char* begin(const char* str) noexcept {
        return str;
    }
    const char* end(const char* str) noexcept {
        if (str == nullptr) return str;

        while (*str != '\0') ++str;
        return str;
    }
} // namespace Detail

template<typename T>
using is_iterable = decltype(Detail::is_iterable_impl<T>(0));


template<class InputItA, class InputItB>
std::size_t levenshtein_distance(
        InputItA aBegin, InputItA aEnd, InputItB bBegin, InputItB bEnd,
        const LevenshteinCosts& costs = {}) noexcept {
    std::size_t sizeA = std::distance(aBegin, aEnd);
    std::size_t sizeB = std::distance(bBegin, bEnd);
    if (sizeA == 0) return sizeB*costs.ins;
    if (sizeB == 0) return sizeA*costs.del;

    std::vector<std::size_t> row(sizeB + 1);
    for (std::size_t j = 0; j < sizeB; ++j) row[j] = j*costs.ins;

    for (InputItA aIt = aBegin; aIt != aEnd; ++aIt) {
        std::size_t aboveLeft = row[0];
        row[0] += costs.del;

        std::size_t i = 0;
        for (InputItB bIt = bBegin; bIt != bEnd; ++bIt) {
            auto subCost = aboveLeft + (*aIt == *bIt ? 0 : costs.sub);
            auto insCost = row[i] + costs.ins;
            auto delCost = row[i+1] + costs.del;
            aboveLeft = row[i+1];
            row[i+1] = std::min({subCost, insCost, delCost});
            i += 1;
        }
    }

    return row.back();
}

template<class Container>
auto get_iterators(const Container& a) noexcept 
-> std::tuple<decltype(Detail::begin(a)), decltype(Detail::end(a))> {
    // Using to allow ADL for custom begin and end.
    using Detail::begin;
    using Detail::end;
    return std::make_tuple(begin(a), end(a));
}

// A specialization for character arrays that ignores the final '\0' because
// std::string and std::string_view don't consider this to be part of the data.
// Note that this only picks up char[n]; the above template handles const char*.
template<std::size_t n>
auto get_iterators(const char (&a)[n]) noexcept {
    return std::make_tuple((const char*)a, (const char*)a + n - 1);
}

template<class ContainerA, class InputItB,
         std::enable_if_t<is_iterable<ContainerA>::value, bool> = true>
std::size_t levenshtein_distance(
        const ContainerA& a, InputItB bBegin, InputItB bEnd,
        const LevenshteinCosts& costs = {}) noexcept {
    auto [aBegin, aEnd] = get_iterators(a);
    return levenshtein_distance(aBegin, aEnd, bBegin, bEnd, costs);
}

template<class InputItA, class ContainerB,
         std::enable_if_t<is_iterable<ContainerB>::value, bool> = true>
std::size_t levenshtein_distance(
        InputItA aBegin, InputItA aEnd, const ContainerB& b,
        const LevenshteinCosts& costs = {}) noexcept {
    auto [bBegin, bEnd] = get_iterators(b);
    return levenshtein_distance(aBegin, aEnd, bBegin, bEnd, costs);
}

template<class ContainerA, class ContainerB,
         std::enable_if_t<is_iterable<ContainerA>::value, bool> = true,
         std::enable_if_t<is_iterable<ContainerB>::value, bool> = true>
std::size_t levenshtein_distance(
        const ContainerA& a, const ContainerB& b, 
        const LevenshteinCosts& costs = {}) noexcept {
    auto [aBegin, aEnd] = get_iterators(a);
    auto [bBegin, bEnd] = get_iterators(b);
    return levenshtein_distance(aBegin, aEnd, bBegin, bEnd, costs);
}

} // namespace LCH
