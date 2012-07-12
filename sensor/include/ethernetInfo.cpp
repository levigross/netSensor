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

#include "ethernetInfo.h"

EthernetInfo::EthernetInfo() {
  _error = true;
  errorMessage = "EthernetInfo::EthernetInfo(): class not initialized";
}

EthernetInfo::EthernetInfo(const std::string fileName) {
  initialize(fileName);
}

bool EthernetInfo::initialize(const std::string fileName) {
  std::ifstream file(fileName.c_str());
  std::string line;
  unsigned int oui;
  multicast = "multicast";
  if (!file) {
    _error = true;
    errorMessage = "EthernetInfo::initialize(): " + fileName + ": " +
                   strerror(errno);
    return false;
  }
  while (getline(file, line)) {
    if (line.length() >= 22 && line.substr(11, 9) == "(base 16)") {
      oui = htonl(strtoul(line.c_str(), NULL, 16));
      ouiMap.insert(std::make_pair(std::string((const char*)(&oui) + 1, 3),
                    line.substr(22)));
    }
  }
  file.close();
  _error = false;
  errorMessage.clear();
  return true;
}

EthernetInfo::operator bool() const {
  return !_error;
}

const std::string &EthernetInfo::error() {
  return errorMessage;
}

const std::string &EthernetInfo::find(const std::string oui) {
  if ((oui[0] & 0x01) == 1) {
    return multicast;
  }
  ouiItr = ouiMap.find(oui.substr(0, 3));
  if (ouiItr == ouiMap.end()) {
    return empty;
  }
  return ouiItr -> second;
}

const std::string &EthernetInfo::find(const char *oui) {
  return find(std::string(oui, 3));
}
