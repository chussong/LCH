#include "levenshtein.hpp"

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

#include "Catch2/catch.hpp"

TEST_CASE("Levenshtein distance works", "[levenshtein]") {
    LCH::LevenshteinCosts costs {1, 5, 10};

    std::vector<int> intEmpty{};
    std::vector<int> intFour{1, 1, 2, 3};
    std::vector<int> intSix{1, 1, 2, 3, 5, 8};
    std::vector<int> intSkip{2, 3, 5, 8};
    std::vector<double> dblEmpty{};
    std::vector<double> dblFour{1.0, 1.0, 2.0, 3.0};
    std::vector<double> dblSix{1.0, 1.0, 2.0, 3.0, 5.0, 8.0};
    std::vector<double> dblSkip{2.0, 3.0, 5.0, 8.0};

    CHECK(LCH::levenshtein_distance(intEmpty, intEmpty) == 0);
    CHECK(LCH::levenshtein_distance(intEmpty, intFour) == 4);
    CHECK(LCH::levenshtein_distance(intFour, intEmpty) == 4);
    CHECK(LCH::levenshtein_distance(intFour, intFour) == 0);
    CHECK(LCH::levenshtein_distance(intFour, intSix) == 2);
    CHECK(LCH::levenshtein_distance(intSix, intFour) == 2);
    CHECK(LCH::levenshtein_distance(intSkip, intSix) == 2);
    CHECK(LCH::levenshtein_distance(intSix, intSkip) == 2);
    CHECK(LCH::levenshtein_distance(intSkip, intFour) == 4);
    CHECK(LCH::levenshtein_distance(intFour, intSkip) == 4);
    CHECK(LCH::levenshtein_distance(intEmpty, intEmpty, costs) == 0);
    CHECK(LCH::levenshtein_distance(intEmpty, intFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(intFour, intEmpty, costs) == 40);
    CHECK(LCH::levenshtein_distance(intFour, intFour, costs) == 0);
    CHECK(LCH::levenshtein_distance(intFour, intSix, costs) == 10);
    CHECK(LCH::levenshtein_distance(intSix, intFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(intSkip, intSix, costs) == 10);
    CHECK(LCH::levenshtein_distance(intSix, intSkip, costs) == 20);
    CHECK(LCH::levenshtein_distance(intSkip, intFour, costs) == 4);
    CHECK(LCH::levenshtein_distance(intFour, intSkip, costs) == 4);

    CHECK(LCH::levenshtein_distance(dblEmpty, dblEmpty, costs) == 0);
    CHECK(LCH::levenshtein_distance(
                dblEmpty.begin(), dblEmpty.end(), dblFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(dblFour, dblEmpty, costs) == 40);
    CHECK(LCH::levenshtein_distance(
                dblFour, dblFour.begin(), dblFour.end(), costs) == 0);
    CHECK(LCH::levenshtein_distance(dblFour, dblSix, costs) == 10);
    CHECK(LCH::levenshtein_distance(dblSix, dblFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(
                dblSkip.begin(), dblSkip.end(), dblSix.begin(), dblSix.end(), 
                costs) == 10);
    CHECK(LCH::levenshtein_distance(dblSix, dblSkip, costs) == 20);
    CHECK(LCH::levenshtein_distance(dblSkip, dblFour, costs) == 4);
    CHECK(LCH::levenshtein_distance(dblFour, dblSkip, costs) == 4);

    CHECK(LCH::levenshtein_distance(intEmpty, dblEmpty, costs) == 0);
    CHECK(LCH::levenshtein_distance(intEmpty, dblFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(intFour, dblEmpty, costs) == 40);
    CHECK(LCH::levenshtein_distance(intFour, dblFour, costs) == 0);
    CHECK(LCH::levenshtein_distance(intFour, dblSix, costs) == 10);
    CHECK(LCH::levenshtein_distance(intSix, dblFour, costs) == 20);
    CHECK(LCH::levenshtein_distance(intSkip, dblSix, costs) == 10);
    CHECK(LCH::levenshtein_distance(intSix, dblSkip, costs) == 20);
    CHECK(LCH::levenshtein_distance(intSkip, dblFour, costs) == 4);
    CHECK(LCH::levenshtein_distance(intFour, dblSkip, costs) == 4);

    using namespace std::literals;
    CHECK(LCH::levenshtein_distance("bead", "bean") == 1);
    CHECK(LCH::levenshtein_distance("kitten", "sitting") == 3);
    CHECK(LCH::levenshtein_distance("kitten", "sitting"s, costs) == 7);
    CHECK(LCH::levenshtein_distance("corporate"s, "cooperation") == 5);
    CHECK(LCH::levenshtein_distance("corporate"sv, "cooperation"s, costs) == 13);
    CHECK(LCH::levenshtein_distance((const char*)"123", "") == 3);
    CHECK(LCH::levenshtein_distance("123", ""sv, costs) == 30);
    CHECK(LCH::levenshtein_distance("", "") == 0);
    CHECK(LCH::levenshtein_distance("", "", costs) == 0);
}
