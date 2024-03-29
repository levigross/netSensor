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

#ifndef ETHERNET_INFO_H
#define ETHERNET_INFO_H

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <string>
#include <tr1/unordered_map>

#include <arpa/inet.h>

class EthernetInfo {
  public:
    EthernetInfo();
    EthernetInfo(const std::string fileName);
    bool initialize(const std::string fileName);
    operator bool() const;
    const std::string &error();
    const std::string &find(const std::string oui);
    const std::string &find(const char *oui);
  private:
    std::tr1::unordered_map <std::string, std::string> ouiMap;
    std::tr1::unordered_map <std::string, std::string>::const_iterator ouiItr;
    std::string _oui;
    std::string empty;
    std::string multicast;
    bool _error;
    std::string errorMessage;
};

#endif
