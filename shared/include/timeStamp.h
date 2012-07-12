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

#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <string>

#include <sys/time.h>

#include <stdint.h>

class TimeStamp {
  public:
    TimeStamp();
    TimeStamp(const TimeStamp&);
    TimeStamp(const timeval&);
    TimeStamp(const uint32_t &seconds, const uint32_t &microseconds);
    TimeStamp &operator =(const TimeStamp&);
    TimeStamp &operator =(const timeval&);
    void set(uint32_t, uint32_t);
    const uint32_t &seconds() const;
    const uint32_t &microseconds() const;
    std::string string() const;
    bool operator ==(const TimeStamp&) const;
    bool operator !=(const TimeStamp&) const;
    bool operator <(const TimeStamp&) const;
    bool operator >(const TimeStamp&) const;
    bool operator <=(const TimeStamp&) const;
    bool operator >=(const TimeStamp&) const;
    const TimeStamp operator -(const TimeStamp&) const;
    TimeStamp &operator -=(const TimeStamp &right);
    const TimeStamp operator +(const TimeStamp&) const;
    TimeStamp &operator +=(const TimeStamp &right);
  private:
    uint32_t _seconds;
    uint32_t _microseconds;
};

#endif
