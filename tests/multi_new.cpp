#include "multi_new.hpp"

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

#include <stdexcept>
#include <array>
#include <iostream>

class ThrowAfterThree {
  public:
    ~ThrowAfterThree() {
        count -= 1;
    }

    static void* operator new(std::size_t size) {
        if (count < 3) {
            count += 1;
            return ::operator new(size);
        } else {
            throw std::exception();
        }
    }

    static int Count() {
        return count;
    }

  private:
    static int count;
};

int ThrowAfterThree::count = 0;

TEST_CASE("multi_new works and is properly exception safe", "[multi_new]") {
    ThrowAfterThree* a = nullptr;
    ThrowAfterThree* b = nullptr;
    ThrowAfterThree* c = nullptr;
    ThrowAfterThree* d = nullptr;

    SECTION("works when nothing throws") {
        REQUIRE(ThrowAfterThree::Count() == 0);
        LCH::MultiNew(a, b, c);
        REQUIRE(ThrowAfterThree::Count() == 3);
        REQUIRE(a != nullptr);
        REQUIRE(b != nullptr);
        REQUIRE(c != nullptr);
        delete a;
        delete b;
        delete c;
    }

    SECTION("nothing is leaked when an exception happens") {
        REQUIRE(ThrowAfterThree::Count() == 0);
        REQUIRE_THROWS(LCH::MultiNew(a, b, c, d));
        // if the count is 0 after this, they have been correctly destroyed
        REQUIRE(ThrowAfterThree::Count() == 0);
        REQUIRE(a == nullptr);
        REQUIRE(b == nullptr);
        REQUIRE(c == nullptr);
        REQUIRE(d == nullptr);
    }
}
