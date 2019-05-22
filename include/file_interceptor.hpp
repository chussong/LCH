///////////////////////////////////////////////////////////////////////////////
// file_interceptor.hpp: a class which intercepts the given FILE* stream for
// as long as it lives, making it accessible as a string; when destroyed, this
// restores the original stream.
//
// This class creates a pipe which can then be read from; since it doesn't 
// create a separate thread to monitor and clear the pipe, it can become full
// and block. It is thus not suitable for large amounts of text and/or keeping
// interception going for long periods of time.
//
// THIS USES A POSIX HEADER SO IT IS EFFECTIVELY UNIX ONLY; THERE IS PRESUMABLY 
// A WAY TO DO THIS ON WINDOWS AS WELL BUT IT WILL NEED SYSTEM CALLS.
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

#ifndef LCH_FILE_INTERCEPTOR_HPP
#define LCH_FILE_INTERCEPTOR_HPP

#include <unistd.h>
#include <string>
#include <stdexcept>

namespace LCH {

class FileInterceptor {
  private:
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr const char* LINE_ENDING = "\n";
    enum PIPES { READ = 0, WRITE = 1 };

  public:
    explicit FileInterceptor(FILE* stream): stream(stream), oldFileno(-1), 
            pipes({-1, -1}), capturing(false) {
        int pipeStatus = pipe(pipes.data());
        if (pipeStatus == -1) {
            throw std::logic_error(__FILE__ ": could not create pipes");
        }
        oldFileno = dup(fileno(stream));
        if (oldFileno == -1) {
            throw std::logic_error(__FILE__ ": could not duplicate old stream");
        }
        RestartCapture();
    }

    // can move a FileInterceptor but not copy it
    FileInterceptor(const FileInterceptor& toCopy) = delete;
    FileInterceptor(FileInterceptor&& toMove) = default;
    FileInterceptor& operator=(const FileInterceptor& toCopy) = delete;
    FileInterceptor& operator=(FileInterceptor&& toMove) = default;

    ~FileInterceptor() {
        if (capturing) EndCapture();
        if (oldFileno >= 0) close(oldFileno);
        if (pipes[READ] >= 0) close(pipes[READ]);
        if (pipes[WRITE] >= 0) close(pipes[WRITE]);
    }

    void RestartCapture() {
        if (capturing) EndCapture();
        fflush(stream);
        dup2(pipes[WRITE], fileno(stream));
        capturing = true;
    }

    void EndCapture() {
        if (!capturing) {
            throw std::logic_error(__FILE__ ": tried to end capture on an "
                                   "interceptor which was not capturing");
        }
        fflush(stream);
        dup2(oldFileno, fileno(stream));
        interceptedString.clear();

        std::string buf(BUFFER_SIZE, '\0');
        while (true) {
            auto readResult = read(pipes[READ], (void*)buf.data(), buf.size());
            int errorNumber = errno;
            if (readResult < 0) {
                throw std::runtime_error(__FILE__ ": got an error from reading "
                                         "the pipe: " 
                                         + std::string(strerror(errorNumber)));
            }
            auto bytesRead = static_cast<std::string::size_type>(readResult);
            interceptedString.append(buf.begin(), buf.begin() + bytesRead);
            if (bytesRead < buf.size()) break;
        };
        capturing = false;
    }

    std::string GetCachedString() const {
        // find_last_not_of would not actually work with CRLF...
        auto lastIndex = interceptedString.find_last_not_of(LINE_ENDING);
        if (lastIndex == std::string::npos) {
            return interceptedString;
        } else {
            return interceptedString.substr(0, lastIndex+1);
        }
    }

    std::string GetString() {
        if (capturing) EndCapture();
        return GetCachedString();
    }

  private:
    FILE* stream;
    int oldFileno;
    std::array<int, 2> pipes;
    bool capturing;
    std::string interceptedString;
};

} // namespace LCH

#endif // LCH_FILE_INTERCEPTOR_HPP
