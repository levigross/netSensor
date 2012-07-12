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
 */

#include <algorithm>

#include "dns.h"

void getPTRRecords(std::vector <std::string> &ptrRecords, const uint32_t &ip) {
  in_addr internetAddress;
  internetAddress.s_addr = ip;
  hostent *hostEntry = gethostbyaddr((const void*)&internetAddress,
                                     sizeof(internetAddress), AF_INET);
  if (hostEntry != NULL) {
    if (strlen(hostEntry -> h_name) == 0) {
      ptrRecords.push_back(".");
    }
    else {
      ptrRecords.push_back(hostEntry -> h_name);
    }
    while (*(hostEntry -> h_aliases) != NULL) {
      if (strlen(*(hostEntry -> h_aliases)) == 0) {
        ptrRecords.push_back(".");
      }
      else {
        ptrRecords.push_back(*(hostEntry -> h_aliases));
      }
      ++(hostEntry -> h_aliases);
    }
  }
  sort(ptrRecords.begin(), ptrRecords.end());
}

std::vector <std::string> getPTRRecords(const uint32_t &ip) {
  std::vector <std::string> ptrRecords;
  getPTRRecords(ptrRecords, ip);
  return ptrRecords;
}
