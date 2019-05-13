///////////////////////////////////////////////////////////////////////////////
// file.hpp: a smart interchange between std::filesystem and boost::filesystem
// depending on the compiler's support for C++17.
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

#ifndef LCH_FILE_HPP
#define LCH_FILE_HPP

#if __cplusplus >= 201703L

#include <filesystem>
namespace LCH { namespace File = std::filesystem; }

#else

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
namespace LCH { namespace File = boost::filesystem; }

inline std::ostream& operator<<(std::ostream& os, const LCH::File::path& path) {
    return os << path.string();
}

#endif

#include <fstream>
#include <iterator>

namespace LCH {

inline std::size_t CountLines(const LCH::File::path& inputPath) {
    std::ifstream inStream(inputPath.native());

    std::size_t lineCount = 0;
    std::string line;
    while (std::getline(inStream,line)) ++lineCount;

    return lineCount;
}

inline bool BytesAreIdentical(const LCH::File::path& pathA, 
                              const LCH::File::path& pathB) {
    std::ifstream fileA(pathA.native(), 
                        std::ifstream::binary | std::ifstream::ate);
    std::ifstream fileB(pathB.native(), 
                        std::ifstream::binary | std::ifstream::ate);

    if (fileA.fail()  || fileB.fail()) return false;
    if (fileA.tellg() != fileB.tellg()) return false;

    fileA.seekg(0, std::ifstream::beg);
    fileB.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(fileA.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(fileB.rdbuf()));
}

} // namespace LCH

#endif // LCH_FILE_HPP
