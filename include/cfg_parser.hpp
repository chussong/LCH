///////////////////////////////////////////////////////////////////////////////
// cfg_parser.hpp: a class to parse a text configuration file into a hash map of
// values -> variables.  
//
// a.k.a. "how to export all your shitty macros to a text file"
///////////////////////////////////////////////////////////////////////////////

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

#ifndef LCH_CFG_PARSER_HPP
#define LCH_CFG_PARSER_HPP

#include "LCH/file.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

#include "options.hpp"

namespace LCH {

// reads the given configuration text file, parsing it into a list of 
// option-value pairs
//
// @path should be the path to a plain text file in the format:
// <optionName1> <optionValue1>
// <optionName2> <optionValue2>
// ...           ...
// where <optionValue_i> is the value of <optionName_i>; if no value is given, 
// the value will be set to the string "true"
//
// TODO: support for aliases for options
//       define custom exception types
class CfgParser {
  public:
    explicit CfgParser(const File::path& path): 
                                                    config(ReadConfig(path)) {}
    explicit CfgParser(const std::string& filename): 
                                                config(ReadConfig(filename)) {}
    explicit CfgParser(const char* filename):   config(ReadConfig(filename)) {}

    const Options& GetOptions() const { return config; }
    const std::vector<std::string>& 
            OptionVector(const std::string& optionName) const {
        try {
            return config.ValueVector(optionName);
        }
        catch (const std::out_of_range&) {
            throw std::logic_error(__FILE__ ": requested option "
                                     + optionName + " was not detected by the "
                                     "parser");
        }
    }
    const std::string& OptionValue(const std::string& optionName) const {
        const std::vector<std::string>& optVec = OptionVector(optionName);
        if (optVec.size() == 0) {
            throw std::logic_error(__FILE__ ": requested option "
                                   + optionName + " was found but was empty");
        } else if (optVec.size() >= 2) {
            throw std::logic_error(__FILE__ ": requested option "
                                   + optionName + " has multiple values. Use "
                                   "OptionVector to get them all");
        }
        return optVec[0];
    }

  private:
    Options config;

    static Options ReadConfig(const File::path& path) {
        if (!File::exists(path)) {
            throw std::runtime_error(__FILE__ ": no file found at "
                                     "provided path " + path.native());
        }
        std::ifstream inStream(path.native());
        if (inStream.fail()) {
            throw std::runtime_error(__FILE__ ": could not open "
                                     "configuration file at " + path.native());
        }
        Options config;
        std::string line;
        while (std::getline(inStream, line)) {
            std::istringstream iss(line);
            std::string optionName = "";
            std::vector<std::string> values;
            std::string currentValue;
            while (iss >> currentValue) {
                if (optionName.empty()) {
                    optionName = std::move(currentValue);
                } else {
                    values.push_back(std::move(currentValue));
                }
            }
            if (optionName.empty()) continue;
            if (values.empty()) {
                // if an option is present without a value, it means "true"
                config.SetTrue(std::move(optionName));
            } else if (values.size() == 1) {
                if (values[0] == "yes" || values[0] == "true") {
                    // if an option's value is yes/true, it's a positive bool
                    config.SetTrue(std::move(optionName));
                } else if (values[0] == "no" || values[0] == "false") {
                    // if an option's value is no/false, it's a negative bool
                    config.SetFalse(std::move(optionName));
                } else {
                    // if an option's value is anything else, store it
                    config.Insert(std::move(optionName), std::move(values));
                }
            } else {
                // if the option has multiple values, definitely store them
                config.Insert(std::move(optionName), std::move(values));
            }
        }
        return config;
    }
    static Options ReadConfig(const std::string& filename) {
        File::path path = filename;
        return ReadConfig(path);
    }
    static Options ReadConfig(const char* filename) {
        File::path path = filename;
        return ReadConfig(path);
    }
};

} // namespace LCH

#endif // LCH_CFG_PARSER_HPP
