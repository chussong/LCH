///////////////////////////////////////////////////////////////////////////////
// generator.hpp: a templated structure for iterating through some abstract data
// source, containing internal structure to keep track of how far through it is.
// Designed to resemble Python generators defined using "yield".
//
// The generator does not advance itself when checked for validity, so usage is:
//
// for (Generator<T> generator(getNextThing); generator; ++generator) {
//     <do something>
// }
//
// or perhaps something like
//
// Generator<T> generator(getNextThing);
// while (generator) {
//     <do something>
//     ++generator;
// }
//
// where getNextThing in both examples is a class inheriting from 
// Generator<T>::GetNext; it must have an operator()() and should manage its own
// internal state. Instances of this class need not be copyable or movable.
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

#ifndef LCH_GENERATOR_HPP
#define LCH_GENERATOR_HPP

#include <cstddef> // std::ptrdiff_t
#include <utility> // std::pair
#include <functional>
#include <iterator>
#include <memory>

namespace LCH {

// TODO: change this from requiring explicit inheritance to having a constructor
// with a templated derived GetNext<U> : public GetNextAbstract class like the
// one used in ThreadPool; then hold getNext in a unique_ptr<GetNextAbstract>.
// This allows construction of a generator with any callable thing instead of
// needing to derive your own Generator<T>::GetNext class.

// T is the type yielded by this generator
template <typename T>
class Generator : public std::iterator<std::input_iterator_tag, T> {
  public:
    // When instantiating a generator, define a class which inherits from this
    // and implements operator()(); there are no other particular requirements,
    // and users of the generator do not need to know how the class works.
    class GetNext {
      public:
          virtual std::pair<T, bool> operator()() = 0;
          virtual ~GetNext() = default;
    };

    // set up the various iterator traits expected by the compiler
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using reference = const T&;
    using pointer = const T*;
    using difference_type = std::ptrdiff_t;

    Generator(std::unique_ptr<GetNext>&& getNext,
              T&& initialValue,
              const bool alreadyInvalid = false): 
                    getNext(std::forward<std::unique_ptr<GetNext>>(getNext)), 
                    value(std::forward<T>(initialValue)), 
                    done(alreadyInvalid)  {}

    // returns a generator which starts off invalid and returns nothing
    static Generator NullGenerator() {
        return Generator(nullptr, T(), true);
    }

    // because getNext maintains its state internally, it is not copyable,
    // and therefore an instance of this class is not copyable either
    Generator(const Generator& toCopy) = delete;
    Generator(Generator&& toMove) = default;
    Generator& operator=(const Generator& toCopy) = delete;
    Generator& operator=(Generator&& toMove) = default;
    ~Generator() = default;

    explicit operator bool() const { return !done; }
    const T& operator*() const { 
        if (done) {
            throw std::out_of_range("LCH::Generator dereferenced after "
                                    "exhaustion.");
        }
        return value; 
    }
    const T* operator->() const { 
        if (done) {
            throw std::out_of_range("LCH::Generator pointer function called "
                                    "after exhaustion.");
        }
        return &value; 
    }

    // this is ++generator, not generator++ (which would require a copy)
    Generator& operator++() {
        if (done) {
            throw std::out_of_range("LCH::Generator incremented while already "
                                    "exhausted.");
        }
        if (getNext == nullptr) {
            throw std::logic_error("LCH::Generator incremented while getNext "
                                   "was a nullptr.");
        }

        std::pair<T,bool> nextValue = (*getNext)();
        if (nextValue.second) {
            value = nextValue.first;
        } else {
            done = true;
        }
        return *this;
    }

  private:
    std::unique_ptr<GetNext> getNext;
    T value;
    bool done;
};

} // namespace LCH

#endif // LCH_GENERATOR_HPP
