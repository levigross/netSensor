/*
 * Copyright 2011 Boris Kochergin. All rights reserved.
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
 *
 * This work was sponsored by the New York Internet Company.
 */

#include <stdint.h>

class PPSMail {
  public:
    PPSMail(const uint32_t &ip, const uint64_t &incomingPPS,
            const uint64_t &outgoingPPS, const uint64_t &incomingBytes,
            const uint64_t &outgoingBytes);
    const uint32_t &ip();
    const uint64_t &incomingPPS();
    const uint64_t &outgoingPPS();
    const uint64_t &incomingBytes();
    const uint64_t &outgoingBytes();
  private:
    uint32_t _ip;
    uint64_t _incomingPPS;
    uint64_t _outgoingPPS;
    uint64_t _incomingBytes;
    uint64_t _outgoingBytes;
};

PPSMail::PPSMail(const uint32_t &ip, const uint64_t &incomingPPS,
                 const uint64_t &outgoingPPS, const uint64_t &incomingBytes,
                 const uint64_t &outgoingBytes) {
  _ip = ip;
  _incomingPPS = incomingPPS;
  _outgoingPPS = outgoingPPS;
  _incomingBytes = incomingBytes;
  _outgoingBytes = outgoingBytes;
}

const uint32_t &PPSMail::ip() {
  return _ip;
}

const uint64_t &PPSMail::incomingPPS() {
  return _incomingPPS;
}

const uint64_t &PPSMail::outgoingPPS() {
  return _outgoingPPS;
}

const uint64_t &PPSMail::incomingBytes() {
  return _incomingBytes;
}

const uint64_t &PPSMail::outgoingBytes() {
  return _outgoingBytes;
}
