// multi_new.hpp: variadic templates for using operator new to allocate multiple
// objects in an exception safe way. If any of the news fail, all previous ones
// will be deleted using the correct deletion operators, then the offending
// exception will be re-thrown.
//
// MultiNew has a "strong exception guarantee", meaning that in the event of an
// exception none of the arguments are changed (and no memory is leaked).
//
// Note that this does not perform any memory management. You should generally
// use std::unique_ptr<T> or std::shared_ptr<T> instead whenever possible, but
// sometimes (e.g. when making APIs for other languages) this automatic 
// management is not possible, so you will have to manage it yourself. Because
// this just uses ordinary operator new, you will have to delete everything
// manually when the time comes; this just protects against failures at the
// time of allocation.
//
// Call like this:
//
// Type* a;
// Type* b;
// Type* c;
// LCH::MultiNew(a, b, c);
//
// or for arrays:
//
// std::size_t bSize = 500;
// LCH::MultiNew(a, 10, b, bSize, c, 5);
//
// or a mix of both:
//
// LCH::MultiNew(a, b, bSize, c);
//
// Arguments are allocated in order from left to right; however, you can not
// use this to allocate a struct and its members at once, since the member
// pointers will be passed relative to the original, uninitialized address of
// the struct.
//
// Currently this can only call the default constructor for each argument. It
// would probably be possible to implement more complicated behavior using
// parameter packs, but if you're building complicated objects you should
// probably be managing their lifetime with something more sophisticated than
// plain old operator new.

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

#ifndef LCH_MULTI_NEW_HPP
#define LCH_MULTI_NEW_HPP

#include <type_traits> // std::enable_if, std::is_pointer
#include <utility> // std::forward

namespace LCH {

template<typename T>
using EnableIfPointer = typename std::enable_if<std::is_pointer<T>::value>::type;

template<typename T, typename = EnableIfPointer<T>>
void MultiNew(T& allocateNow) {
    using Underlying = typename std::remove_pointer<T>::type;
    allocateNow = new Underlying;
}

template<typename T, typename = EnableIfPointer<T>>
void MultiNew(T& allocateNow, std::size_t count) {
    using Underlying = typename std::remove_pointer<T>::type;
    allocateNow = new Underlying[count];
}

// Have to forward declare this so the next function can see it when recursing.
template<typename T, typename... Args, typename = EnableIfPointer<T>>
void MultiNew(T& allocateNow, std::size_t count, Args&&... allocateLater);

template<typename T, typename... Args, typename = EnableIfPointer<T>>
void MultiNew(T& allocateNow, Args&&... allocateLater) {
    using Underlying = typename std::remove_pointer<T>::type;
    Underlying* temp = new Underlying;
    try {
        MultiNew(std::forward<Args>(allocateLater)...);
        allocateNow = temp;
    } catch (...) {
        delete temp;
        throw;
    }
}

template<typename T, typename... Args, typename = EnableIfPointer<T>>
void MultiNew(T& allocateNow, std::size_t count, Args&&... allocateLater) {
    using Underlying = typename std::remove_pointer<T>::type;
    Underlying* temp = new Underlying[count];
    try {
        MultiNew(std::forward<Args>(allocateLater)...);
        allocateNow = temp;
    } catch (...) {
        delete[] temp;
        throw;
    }
}

} // namespace LCH

#endif // LCH_MULTI_NEW_HPP
