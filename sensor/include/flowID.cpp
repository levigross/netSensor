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

#include <sstream>

#include <arpa/inet.h>

#include <include/address.h>

#include "flowID.h"

FlowID::FlowID() {
  flowID.assign(13, '\0');
}

void FlowID::set(const std::string &string) {
  flowID = string;
}

void FlowID::set(const uint8_t &protocol, const uint32_t &sourceIP,
                 const uint32_t &destinationIP, const uint16_t &sourcePort,
                 const uint16_t &destinationPort) {
  flowID.replace(0, sizeof(protocol), (char*)&protocol, sizeof(protocol));
  flowID.replace(1, sizeof(sourceIP), (char*)&sourceIP, sizeof(sourceIP));
  flowID.replace(5, sizeof(destinationIP), (char*)&destinationIP,
                 sizeof(destinationIP));
  flowID.replace(9, sizeof(sourcePort), (char*)&sourcePort, sizeof(sourcePort));
  flowID.replace(11, sizeof(destinationPort), (char*)&destinationPort,
                 sizeof(destinationPort));
}

const uint8_t &FlowID::protocol() const {
  return *(uint8_t*)flowID.data();
}

const uint32_t &FlowID::sourceIP() const {
  return *(uint32_t*)(flowID.data() + 1);
}

const uint32_t &FlowID::destinationIP() const {
  return *(uint32_t*)(flowID.data() + 5);
}

const uint16_t &FlowID::sourcePort() const {
  return *(uint16_t*)(flowID.data() + 9);
}

const uint16_t &FlowID::destinationPort() const {
  return *(uint16_t*)(flowID.data() + 11);
}

const std::string &FlowID::data() const {
  return flowID;
}

std::string FlowID::string() const {
  std::ostringstream string;
  string << textIP(sourceIP()) << ':' << ntohs(sourcePort()) << " -> "
         << textIP(destinationIP()) << ':' << ntohs(destinationPort());
  return string.str();
}
