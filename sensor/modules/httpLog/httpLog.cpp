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

#include <string>
#include <vector>
#include <tr1/memory>

#include <include/configuration.h>
#include <include/httpSession.h>
#include <include/logger.h>
#include <include/writer.hpp>

using namespace std;
using namespace tr1;

static uint32_t timeout;
static Writer <HTTPSession> writer;
static uint8_t version = 1;

/* Converts a session in memory to on-disk format. */
static void makeRecord(Writer <HTTPSession>::Record &record,
                       const HTTPSession &session) {
  /* Record version. */
  record += version;
  /* Client Ethernet address. */
  record.append(session.clientMAC, ETHER_ADDR_LEN);
  /* Server Ethernet address. */
  record.append(session.serverMAC, ETHER_ADDR_LEN);
  /* Client IP. */
  record += session.clientIP;
  /* Server IP. */
  record += session.serverIP;
  /* Client port. */
  record += session.clientPort;
  /* Server port. */
  record += session.serverPort;
  /* Compression (off for now). */
  record += (uint8_t)0;
  /* Number of messages. */
  record += htonl((uint32_t)(session.requests.size() + session.responses.size()));
  /* Requests. */
  for (size_t i = 0; i < session.requests.size(); ++i) {
    /* Message type. */
    record += (uint8_t)session.requests[i].type;
    /* Time (seconds). */
    record += htonl(session.requests[i].time.seconds());
    /* Time (microseconds). */
    record += htonl(session.requests[i].time.microseconds());
    /* Number of message components. */
    record += htonl((uint32_t)session.requests[i].message.size());
    for (size_t j = 0; j < session.requests[i].message.size(); ++j) {
      /* Message component size. */
      record += htonl((uint32_t)session.requests[i].message[j].size());
      /* Message compnent. */
      record += session.requests[i].message[j];
    }
    /* Number of headers. */
    record += htonl((uint32_t)session.requests[i].headers.size());
    for (size_t j = 0; j < session.requests[i].headers.size(); ++j) {
      /* Header field size. */
      record += htonl((uint32_t)session.requests[i].headers[j].first.size());
      /* Header field. */
      record += session.requests[i].headers[j].first;
      /* Header value size. */
      record += htonl((uint32_t)session.requests[i].headers[j].second.size());
      /* Header value. */
      record += session.requests[i].headers[j].second;
    }
  }
  /* Responses. */
  for (size_t i = 0; i < session.responses.size(); ++i) {
    /* Message type. */
    record += (uint8_t)session.responses[i].type;
    /* Time (seconds). */
    record += htonl(session.responses[i].time.seconds());
    /* Time (microseconds). */
    record += htonl(session.responses[i].time.microseconds());
    /* Number of message components. */
    record += htonl((uint32_t)session.responses[i].message.size());
    for (size_t j = 0; j < session.responses[i].message.size(); ++j) {
      /* Message component size. */
      record += htonl((uint32_t)session.responses[i].message[j].size());
      /* Message compnent. */
      record += session.responses[i].message[j];
    }
    /* Number of headers. */
    record += htonl((uint32_t)session.responses[i].headers.size());
    for (size_t j = 0; j < session.responses[i].headers.size(); ++j) {
      /* Header field size. */
      record += htonl((uint32_t)session.responses[i].headers[j].first.size());
      /* Header field. */
      record += session.responses[i].headers[j].first;
      /* Header value size. */
      record += htonl((uint32_t)session.responses[i].headers[j].second.size());
      /* Header value. */
      record += session.responses[i].headers[j].second;
    }
  }
}

extern "C" {
  int initialize(const Configuration &conf,
                 Logger &logger __attribute__((unused)), string &error) {
    timeout = conf.getNumber("timeout");
    if (!writer.initialize(conf.getString("data"), "http", timeout,
                           &makeRecord)) {
      error = writer.error();
      return 1;
    }
    return 0;
  }

  int processHTTP(const shared_ptr <HTTPSession> session) {
    writer.write(session, session -> time.seconds());
    return 0;
  }

  int flush() {
    /*
     * Write everything in Berkeley DB's cache to disk so that we don't lose
     * too much data in the event of a crash or power failure.
     */
    writer.flush();
    return 0;
  }

  int finish() {
    return 0;
  }
}
