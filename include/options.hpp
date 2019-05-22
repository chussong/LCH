///////////////////////////////////////////////////////////////////////////////
// options.hpp: a class that contains option:value pairs, generally parsed from
// the command line or an external file
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

#ifndef LCH_OPTIONS_HPP
#define LCH_OPTIONS_HPP

#include <vector>
#include <string>
#include <unordered_map>

namespace LCH {

// Represents a set of option:value(s) pairs; the values are kept as vectors of
// strings. If an option has only one value, it can be accessed with 
// Options::Value; you can always use Options::ValueVector to get all associated
// values in vector form.
//
// To populate, default-construct an instance and then use Options::Insert to
// add entries to the internal hash table. Once they're entered, they can't be
// modified without being deleted, so be careful. Multiple instances of Options 
// can be combined with operator+ into a single instance which shares all of 
// their entries; if any keys are duplicated and their values are not exactly 
// identical, operator+= will throw a logic_error.
class Options {
  public:
    // basic value insertion and extraction -----------------------------------

    void Insert(const std::string& key, const std::vector<std::string>& vals) {
        if (data.count(key) == 1) {
            throw std::logic_error("Options::Insert: key \"" + key 
                                   + "\" already exists.");
        } else {
            data.emplace(key, vals);
        }
    }
    void Insert(const std::string& key, const std::string& val) {
        Insert(key, std::vector<std::string>{val});
    }
    void Remove(const std::string& key) {
        data.erase(key);
    }
    void Overwrite(const std::string& key, const std::vector<std::string>& vals) {
        if (Exists(key)) Remove(key);
        Insert(key, vals);
    }
    void Overwrite(const std::string& key, const std::string& val) {
        Overwrite(key, std::vector<std::string>{val});
    }

    // Options::Value is for an option with exactly one value; for multi-valued
    // options, use Options::ValueVector
    const std::string& Value(const std::string& key) const {
        const std::vector<std::string>& valVec = ValueVector(key);
        if (valVec.size() == 0) {
            throw std::logic_error("Options::Value: option \"" + key 
                                   + "\" was found, but it was empty.");
        } else if (valVec.size() >= 2) {
            throw std::logic_error("Options::Value: option \"" + key 
                                   + "\" has multiple values. Use ValueVector "
                                   "to get them all.");
        }
        return valVec[0];
    }
    const std::vector<std::string>& ValueVector(const std::string& key) const {
        if (data.count(key) == 0) {
            throw std::logic_error("Options::ValueVector: option \"" + key 
                                   + "\" not found.");
        } else {
            return data.at(key);
        }
    }
    // Don't use this for handling boolean options; use the specialized boolean
    // methods below instead.
    bool Exists(const std::string& key) const {
        return data.count(key) == 1;
    }

    // more idiomatic handling of boolean options -----------------------------

    // Set a key to be a boolean option w/ value "true" -- if it doesn't exist, 
    // it's inserted with an empty vector; if it exists already, it's made 
    // boolean by deleting its values so it effectively contains only "true"
    void SetTrue(const std::string& key) {
        if (data.count(key) == 0) {
            data.emplace(key, std::vector<std::string>());
        } else {
            data.at(key).clear();
        }
    }

    // Set a key to be a boolean option w/ value "false" -- options are false by
    // default, so all we have to do is delete the option if it already exists
    void SetFalse(const std::string& key) {
        data.erase(key);
    }

    // Return [true] or [false] if option is a boolean set by Options::SetTrue 
    // or Options::SetFalse; if option is not a boolean, throw std::logic_error.
    bool IsTrue(const std::string& key) const {
        if (data.count(key) == 1) {
            if (data.at(key).size() == 0) {
                return true;
            } else {
                throw std::logic_error("Options::IsTrue called on an option key"
                                       " with a non-boolean value.");
            }
        } else {
            return false;
        }
    }

    // ways to combine multiple Options objects -------------------------------

    // Combine two Options objects, returning their union. If any option is 
    // present in both, it must have the same value, or a std::logic_error is
    // thrown; to avoid this, make sure options can't be entered in multiple
    // places, or call Options::Incorporate which gives precedence to *this.
    Options& operator+=(const Options& rhs) {
        for (const auto& entry : rhs.data) {
            if (this->data.count(entry.first) == 1) {
                if (this->data[entry.first] != entry.second) {
                    throw std::logic_error("Options::operator+=: same option "
                                           "was present in both containers with"
                                           " different values.");
                }
                // if the values are the same, do nothing
            } else {
                this->data.emplace(entry.first, entry.second);
            }
        }
        return *this;
    }

    // Incorporate another Options object into *this, leaving entries of *this
    // as-is whenever there is a conflict with @other. Note that @other is not
    // invalidated by doing this, unlike std::unordered_map::merge.
    void Incorporate(const Options& other) {
        for (const auto& entry : other.data) {
            if (this->data.count(entry.first) == 0) {
                this->data.emplace(entry.first, entry.second);
            }
        }
    }

  private:
    std::unordered_map<std::string, std::vector<std::string>> data;
};

inline Options operator+(Options A, const Options& B) {
    A += B;
    return A;
}

}

#endif // LCH_OPTIONS_HPP
