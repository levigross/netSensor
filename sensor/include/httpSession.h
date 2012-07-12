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

#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <string>
#include <utility>
#include <vector>

#include <sys/socket.h>

#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <netinet/if_ether.h>
#endif
#ifdef __linux__
#include <netinet/ether.h>
#endif

#include <include/httpParser.h>
#include <include/timeStamp.h>

enum HTTPMessageState { NO_STATE, PATH_STATE, URL_STATE, HEADER_FIELD_STATE,
                        HEADER_VALUE_STATE, COMPLETE_STATE };

struct HTTPMessage {
  HTTPMessage(const http_parser_type &_type, const TimeStamp &_time);
  http_parser_type type;
  TimeStamp time;
  std::vector <std::string> message;
  std::vector <std::pair <std::string, std::string> > headers;
};

struct HTTPSession {
  HTTPSession();
  TimeStamp time;
  char clientMAC[ETHER_ADDR_LEN];
  char serverMAC[ETHER_ADDR_LEN];
  uint32_t clientIP;
  uint32_t serverIP;
  uint16_t clientPort;
  uint16_t serverPort;
  http_parser parsers[2];
  HTTPMessageState requestState;
  HTTPMessageState responseState;
  std::vector <HTTPMessage> requests;
  std::vector <HTTPMessage> responses;
};

#endif
