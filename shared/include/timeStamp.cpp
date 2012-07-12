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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <ctime>

#include <iomanip>

#include "timeStamp.h"

TimeStamp::TimeStamp() {
  _seconds = 0;
  _microseconds = 0;
}

TimeStamp::TimeStamp(const TimeStamp &right) {
  *this = right;
}

TimeStamp::TimeStamp(const timeval &_timeval) {
  *this = _timeval;
}

TimeStamp::TimeStamp(const uint32_t &seconds, const uint32_t &microseconds) {
  _seconds = seconds;
  _microseconds = microseconds;
}

std::string TimeStamp::string() const {
  time_t time = _seconds;
  struct tm _time;
  char buffer[128];
  size_t offset;
  localtime_r(&time, &_time);
  offset = strftime(buffer, sizeof(buffer), "%F %T", &_time);
  sprintf(buffer + offset, ".%06u", _microseconds);
  return buffer;
}

TimeStamp &TimeStamp::operator =(const TimeStamp &right) {
  _seconds = right.seconds();
  _microseconds = right.microseconds();
  return *this;
}

TimeStamp &TimeStamp::operator =(const timeval &right) {
  _seconds = right.tv_sec;
  _microseconds = right.tv_usec;
  return *this;
}

void TimeStamp::set(uint32_t seconds, uint32_t microseconds) {
  _seconds = seconds;
  _microseconds = microseconds;
}

const uint32_t &TimeStamp::seconds() const {
  return _seconds;
}

const uint32_t &TimeStamp::microseconds() const {
  return _microseconds;
}

bool TimeStamp::operator ==(const TimeStamp &right) const {
  return (_seconds == right.seconds() && _microseconds == right.microseconds());
}

bool TimeStamp::operator !=(const TimeStamp &right) const {
  return !(*this == right);
}

bool TimeStamp::operator <(const TimeStamp &right) const {
  if (_seconds != right.seconds()) {
    return (_seconds < right.seconds());
  }
  return (_microseconds < right.microseconds());
}

bool TimeStamp::operator >(const TimeStamp &right) const {
  if (_seconds != right.seconds()) {
    return (_seconds > right.seconds());
  }
  return (_microseconds > right.microseconds());
}

bool TimeStamp::operator <=(const TimeStamp &right) const {
  return !(*this > right);
}

bool TimeStamp::operator >=(const TimeStamp &right) const {
  return !(*this < right);
}

TimeStamp &TimeStamp::operator -=(const TimeStamp &right) {
  if (*this <= right) {
    _seconds = 0;
    _microseconds = 0;
    return *this;
  }
  if (_microseconds < right.microseconds()) {
    _microseconds += 1000000;
    _seconds -= 1;
  }
  _microseconds -= right.microseconds();
  _seconds -= right.seconds();
  return *this;
}

const TimeStamp TimeStamp::operator -(const TimeStamp &right) const {
  return TimeStamp(*this) -= right;
}

TimeStamp &TimeStamp::operator +=(const TimeStamp &right) {
  _microseconds += right.microseconds();
  _seconds += right.seconds();
  if (_microseconds >= 1000000) {
    ++_seconds;
    _microseconds -= 1000000;
  }
  return *this;
}

const TimeStamp TimeStamp::operator +(const TimeStamp &right) const {
  return TimeStamp(*this) += right;
}
