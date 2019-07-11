// manifest.hpp: class for representing and parsing a manifest file, containing
// a sequence of paths with optional annotations.
//
// Each path in the manifest is considered to be relative to the parent of the
// manifest's path (as given to the constructor) and is saved in this way.

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

#ifndef LCH_MANIFEST_HPP
#define LCH_MANIFEST_HPP

#include "LCH/file.hpp"

#include <string>
#include <fstream>

namespace LCH {

class Manifest {
  public:
    struct Entry {
        LCH::File::path path;
        std::string annotation;
    };

    explicit Manifest(const LCH::File::path& path): Manifest(path, ' ') {}

    Manifest(const LCH::File::path& path, char annotationSeparator):
            Manifest(path, annotationSeparator, '\n') {}

    Manifest(const LCH::File::path& path, 
             char annotationSeparator, char entrySeparator) {
        LCH::File::path basePath = path.parent_path();
        std::ifstream inStream(path.native());
        std::string line;
        while (std::getline(inStream, line, entrySeparator)) {
            Entry newEntry;
            auto splitLoc = line.find(annotationSeparator);
            newEntry.path = basePath / line.substr(0, splitLoc);
            if (splitLoc != std::string::npos && splitLoc+1 < line.size()) {
                newEntry.annotation = line.substr(splitLoc+1);
            }
            entries.push_back(std::move(newEntry));
        }
    }

    const std::vector<Entry>& Entries() const {
        return entries;
    }

  private:
    std::vector<Entry> entries;
};

} // namespace LCH

#endif // LCH_MANIFEST_HPP
