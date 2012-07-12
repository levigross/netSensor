/*
 * Copyright 2007-2011 Boris Kochergin. All rights reserved.
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

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <iomanip>
#include <sstream>

#include "address.h"

/*
 * Mac OS X 10.6.6 doesn't have ether_aton_r() or ether_ntoa_r(), and glibc
 * 2.11.1's ether_ntoa_r() doesn't zero-pad the Ethernet addresses it returns,
 * so we'll just roll our own versions.
 */
std::string binaryMAC(const std::string &text) {
  unsigned int binary[ETHER_ADDR_LEN];
  u_char _binary[ETHER_ADDR_LEN];
  sscanf(text.c_str(), "%x:%x:%x:%x:%x:%x", &binary[0], &binary[1], &binary[2],
         &binary[3], &binary[4], &binary[5]);
  for (size_t i = 0; i < ETHER_ADDR_LEN; ++i) {
    _binary[i] = binary[i];
  }
  return std::string((const char*)_binary, sizeof(_binary));
}

std::string textMAC(const u_char *binary) {
  char text[17];
  sprintf(text, "%02x:%02x:%02x:%02x:%02x:%02x", binary[0],
          binary[1], binary[2], binary[3], binary[4], binary[5]);
  return std::string(text, sizeof(text));
}

std::string textMAC(const char *binary) {
  return textMAC((const u_char*)binary);
}

uint32_t binaryIP(const std::string &text) {
  in_addr binary;
  inet_pton(AF_INET, text.c_str(), &binary);
  return binary.s_addr;
}

std::pair <uint32_t, uint32_t> cidrToIPs(const std::string &network) {
  std::pair <uint32_t, uint32_t> range;
  size_t slash = network.rfind('/');
#if ENDIAN == BIG_ENDIAN
  uint32_t ip = binaryIP(network.substr(0, slash));
#else
  uint32_t ip = ntohl(binaryIP(network.substr(0, slash)));
#endif
  if (slash == std::string::npos) {
    return std::make_pair(ip, ip);
  }
  return std::make_pair(ip,
                        ip + pow((double)2,
                                 (double)(32 - strtoul(network.substr(slash + 1).c_str(),
                                                       NULL, 10))) - 1);
}

std::string textIP(const uint32_t &binary) {
  in_addr _binary;
  char text[INET_ADDRSTRLEN];
  _binary.s_addr = binary;
  inet_ntop(AF_INET, &_binary.s_addr, text, INET_ADDRSTRLEN);
  return text;
}
