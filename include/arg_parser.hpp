///////////////////////////////////////////////////////////////////////////////
// arg_parser.hpp: a class that parses argc and argv into C++ types and
// extracts the options from each
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

#ifndef LCH_ARG_PARSER_HPP
#define LCH_ARG_PARSER_HPP

#include "LCH/options.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <tuple>

namespace LCH {

typedef std::unordered_map<std::string, std::size_t> OptionSpec;

// reads the given c-style @argc and @argv, along with the option-specifying 
// file @optionFile, parsing them into a list of option-value pairs plus a list
// of the remaining non-option arguments; order preservation is guaranteed for
// arguments (both overall and arguments of options) but options are unordered
//
// @optionFile should be the name of a plain text file in the format:
// <optionName1> <#1>
// <optionName2> <#2>
// ...           ...
// where <#_i> is the number of arguments taken by <optionName_i> (0 is allowed)
// If @optionFile is empty or the file is not found, parsing will be attempted
// with no options
//
// throws std::runtime_error when encountering an unrecognized option
//
// TODO: support for aliases for options
//       support for options with unlimited arguments (i.e. until next option)
//       define custom exception types
class ArgParser {
  public:
    ArgParser(const int argc, const char* const* const argv,
                     const std::string& optionFile = ""):
            args(StringifyArgs(argc, argv)) {
        spec = ReadOptionSpec(optionFile);
        options = ExtractOptions(args, spec);
    }

    const Options& GetOptions() const { return options; }
    const std::vector<std::string>& 
            OptionVector(const std::string& optionName) const {
        try {
            return options.ValueVector(optionName);
        }
        catch (const std::logic_error&) {
            if (spec.count(optionName) == 0) {
                throw std::logic_error("ArgParser::OptionVector: option not "
                                       "found in spec.");
            } else {
                throw std::runtime_error("ArgParser::OptionVector: option not "
                                         "detected by parser.");
            }
        }
    }
    const std::string& OptionValue(const std::string& optionName) const {
        const std::vector<std::string>& optVec = OptionVector(optionName);
        if (optVec.size() != 1) {
            throw std::logic_error("ArgParser::OptionValue: requested option "
                                   "does not have exactly one value.");
        }
        return optVec[0];
    }
    const std::vector<std::string>& GetArgs() const { return args; }

  private:
    OptionSpec spec;
    Options options;
    std::vector<std::string> args;

    // Turn the C-style arg vector into a C++ vector of C++ strings
    static std::vector<std::string> 
            StringifyArgs(const int argc, 
                          const char* const* const argv) {
        std::vector<std::string> output(argc-1);
        for (int i = 0; i < argc-1; ++i) {
            output[i] = argv[i+1];
        }
        return output;
    }

    static OptionSpec ReadOptionSpec(const std::string& optionFile) {
        OptionSpec spec;
        if (optionFile.empty()) return spec;
        std::ifstream inStream(optionFile);
        if (inStream.fail()) {
            throw std::runtime_error("ArgParser::ReadOptionSpec: could not open"
                                     " specification file.");
        }
        std::string optName;
        std::size_t n_args;
        while (inStream >> optName >> n_args) {
            spec.emplace(optName, n_args);
        }
        return spec;
    }

    // Extract the options from @args, leaving behind non-option, non-empty els.
    // Return a vector of the extracted options
    static Options ExtractOptions(std::vector<std::string>& args,
                                         const OptionSpec& spec) {
        std::vector<std::string> newArgs;
        Options options;

        std::string currentOption = "";
        std::size_t currentArgc = 0;
        std::vector<std::string> optArgs;

        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i].empty()) continue;
            if (currentOption.empty() && args[i][0] == '-') {
                auto parsedOption = ParseNewOption(args[i], options, spec);
                currentOption = std::get<0>(parsedOption);
                currentArgc = std::get<1>(parsedOption);
            } else {
                if (currentOption.empty()) {
                    newArgs.push_back(std::move(args[i]));
                } else {
                    optArgs.push_back(std::move(args[i]));
                    if (optArgs.size() >= currentArgc) {
                        options.Insert(std::move(currentOption), 
                                        std::move(optArgs));
                        currentOption.clear();
                        currentArgc = 0;
                        optArgs.clear();
                    }
                }
            }
        }
        if (!currentOption.empty()) {
            throw std::runtime_error("ArgParser::ExtractOptions: parsing "
                                     "finished with an option still open.");
        }
        std::swap(args, newArgs);
        return options;
    }

    static std::tuple<std::string, std::size_t> 
            ParseNewOption(const std::string& arg, Options& options,
                           const OptionSpec& spec) {
        // TODO add support for abbreviated options with only 1 minus
        if (arg.size() < 2 || arg[1] != '-') {
            throw std::logic_error("ArgParser::ParseNewOptions: option \""
                                   + arg + "\" does not begin with \"--\"");
        }
        std::size_t spaceLoc = arg.find(' ');
        std::size_t equalsLoc = arg.find('=');
        std::string option = arg.substr(2, std::min(spaceLoc, equalsLoc)-2);
        if (spec.count(option) == 1) {
            std::size_t numberOfArgs = spec.at(option);
            if (numberOfArgs == 0) {
                options.SetTrue(option);
                return std::make_tuple("", 0);
            } else if (numberOfArgs == 1 && equalsLoc < std::string::npos) {
                options.Insert(option, {arg.substr(equalsLoc+1)});
                return std::make_tuple("", 0);
            } else {
                return std::make_tuple(option, numberOfArgs);
            }
        } else {
            throw std::runtime_error("ArgParser::ParseNewOption: option \""
                                     + option + "\" not in spec");
        }
    }
};

} // namespace LCH

#endif // LCH_ARG_PARSER_HPP
