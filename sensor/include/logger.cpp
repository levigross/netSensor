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

#include <cerrno>
#include <cstring>

#include "logger.h"

Logger::Logger() {
  _error = true;
  errorMessage = "Logger::Logger(): class not initialized";
}

Logger::Logger(const std::string fileName, const std::string timeFormat) {
  initialize(fileName, timeFormat);
}

bool Logger::initialize(const std::string fileName,
                        const std::string timeFormat) {
  _error = true;
  _timeFormat = timeFormat;
  int ret = pthread_mutex_init(&_lock, NULL);
  if (ret != 0) {
    _error = true;
    errorMessage = "Logger::initialize(): pthread_mutex_init(): ";
    errorMessage += strerror(ret);
    return false;
  }
  file.open(fileName.c_str(), std::ios::app);
  if (!file) {
    _error = true;
    errorMessage = "Logger::initialize(): open(): " + fileName + ": " +
                   strerror(errno);
    return false;
  }
  _error = false;
  errorMessage.clear();
  return true;
}

Logger::operator bool() const {
  return !_error;
} 

const std::string &Logger::error() {
  return errorMessage;
}

int Logger::lock() {
  return pthread_mutex_lock(&_lock);
}

int Logger::unlock() {
  return pthread_mutex_unlock(&_lock);
}

Logger &Logger::operator <<(std::ostream& (*manipulator)(std::ostream&)) {
  file << manipulator;
  return *this;
}

const char *Logger::time() {
  _time = ::time(NULL);
  localtime_r(&_time, &__time);
  strftime(timeBuffer, sizeof(timeBuffer), _timeFormat.c_str(), &__time);
  return timeBuffer;
}

Logger::~Logger() {
  if (_error == false) {
    pthread_mutex_destroy(&_lock);
    file.close();
  }
}
