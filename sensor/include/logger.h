/*
 * Copyright 2010-2011 Boris Kochergin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <ctime>

#include <fstream>
#include <string>

#include <pthread.h>

class Logger {
  public:
    Logger();
    Logger(const std::string fileName, const std::string timeFormat);
    bool initialize(const std::string fileName, const std::string timeFormat);
    operator bool() const;
    const std::string &error();
    int lock();
    int unlock();
    template <class T>
    Logger &operator <<(const T &data);
    Logger &operator <<(std::ostream& (*manipulator)(std::ostream&));
    const char *time();
    ~Logger();
  private:
    std::string _timeFormat;
    time_t _time;
    tm __time;
    char timeBuffer[128];
    std::ofstream file;
    pthread_mutex_t _lock;
    bool _error;
    std::string errorMessage;
};

template <typename T>
Logger &Logger::operator <<(const T &data) {
  file << data;
  return *this;
}

#endif
