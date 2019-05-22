///////////////////////////////////////////////////////////////////////////////
// matrix.hpp: a general purpose matrix class with some basic access,
// arithmetic, and IO functions. If performance is important, use a real matrix
// library like Eigen!
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

#ifndef LCH_MATRIX_HPP
#define LCH_MATRIX_HPP

// general purpose matrix.hpp

#include "LCH/file.hpp"

#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>

namespace LCH {

template<typename T>
class Matrix {
  public:
    typedef std::size_t size_type;
    typedef std::array<size_type,2> Coords;
    static constexpr Coords NullCoords {{static_cast<size_type>(-1), 
                                         static_cast<size_type>(-1)}};

  private:
    const size_type rows;
    const size_type cols;
    std::vector<T> data;

  public:
    Matrix(size_type rows, size_type cols): rows(rows), cols(cols),
                                            data(rows*cols) {}

    T& operator()(const size_type row, const size_type col) {
            return data.at(row*cols + col);
    }
    T& operator()(const Coords& coords) {
            return (*this)(coords[0], coords[1]);
    }

    const T& operator()(const size_type row, const size_type col) const {
            return data.at(row*cols + col);
    }
    const T& operator()(const Coords& coords) const {
            return (*this)(coords[0], coords[1]);
    }

    size_type Rows() const { return rows; }
    size_type Cols() const { return cols; }

    static Matrix ReadFromFile(const File::path& path) {
        if (!File::exists(path)) {
            throw std::runtime_error(__FILE__ ": no file found at provided "
                                     "path " + path.native());
        }
        std::ifstream inStream(path.native());
        if (inStream.fail()) {
            throw std::runtime_error(__FILE__ ": could not open file at "
                                     "provided path " + path.native());
        }

        std::size_t rows = 0;
        std::size_t cols = 0;
        std::vector<T> data;
        std::string line;
        while (std::getline(inStream, line)) {
            std::istringstream iss(line);
            T value;
            std::size_t colsOnThisLine = 0;
            while (iss >> value) {
                data.push_back(value);
                ++colsOnThisLine;
            }
            if (cols == 0) cols = colsOnThisLine;
            if (cols != colsOnThisLine) {
                throw std::runtime_error("Matrix::ReadFromFile: matrix not "
                                         "rectangular (row lengths differ).");
            }
            ++rows;
        }

        Matrix output(rows, cols);
        output.data = data;
        return output;
    }
    static Matrix ReadFromFile(const std::string& filename) {
        File::path path = File::current_path() / filename;
        return ReadFromFile(path);
    }
    static Matrix ReadFromFile(const char* filename) {
        File::path path = File::current_path() / filename;
        return ReadFromFile(path);
    }

    // TODO: buffer into columns first, then check widths and format everything
    // to line up
    friend std::ostream& operator<<(std::ostream& os, const Matrix& mat) {
        size_type i = 0;
        while (i < mat.rows*mat.cols - 1) {
            os << mat.data[i];
            ++i;
            if (i%mat.cols >= 1) os << ' ';
            if (i%mat.cols == 0) os << '\n';
        }
        return os << mat.data[i];
    }
};

} // namespace LCH

#endif // LCH_MATRIX_HPP
