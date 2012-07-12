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
 * This work was sponsored by Ecological, LLC.
 */

#include <cstring>

#include <string>
#include <vector>
#include <tr1/memory>

#include <include/configuration.h>
#include <include/flowID.h>
#include <include/logger.h>
#include <include/memory.hpp>
#include <include/module.h>
#include <include/writer.hpp>

#include "pjlSession.h"

using namespace std;
using namespace tr1;

/*
 * The session table is a hash table of shared pointers to PJLSession
 * structures with flow IDs as keys.
 */
static unordered_map <string, shared_ptr <PJLSession> > sessions;
static unordered_map <string, shared_ptr <PJLSession> >::iterator itr;
static shared_ptr <PJLSession> session;
/* Session memory allocator. */
static Memory <PJLSession> memory;
/* Locks for the session table. */
static pthread_mutex_t *locks;
//static uint64_t bufferSize;
static uint32_t timeout;
static Logger *logger;

static bool sessionWarning = true, bufferWarning = true;

static Writer <PJLSession> writer;
static uint8_t version = 1;

/* Converts a session in memory to on-disk format. */
static void makeRecord(Writer <PJLSession>::Record &record,
                       const PJLSession &session) {
  /* Record version. */
  record += version;
  /* Time (seconds). */
  record += htonl(session.startTime.seconds());
  /* Time (microseconds). */
  record += htonl(session.startTime.microseconds());
  /* Client Ethernet address. */
  record.append(session.clientMAC, ETHER_ADDR_LEN);
  /* Server Ethernet address. */
  record.append(session.serverMAC, ETHER_ADDR_LEN);
  /* Client IP address. */
  record += session.clientIP;
  /* Server IP address. */
  record += session.serverIP;
  /* Client port. */
  record += session.clientPort;
  /* Server port. */
  record += session.serverPort;
  /* Computer name size. */
  record += htons((uint16_t)session.computer.length());
  /* Computer name. */
  record += session.computer;
  /* Username size. */
  record += htons((uint16_t)session.user.length());
  /* Username. */
  record += session.user;
  /* Title size. */
  record += htons((uint16_t)session.title.length());
  /* Title. */
  record += session.title;
  /* Number of bytes */
  record += htonl(session.size);
  /* Number of pages. */
  record += htons(session.pages);
  /* Whether we ran out of memory for this session. */
  record += session.outOfMemory;
  /*pthread_mutex_lock(&bufferSizeLock);
  bufferSize += session.buffer.size();
  pthread_mutex_unlock(&bufferSizeLock);*/
}

static string variable(const string &line, const string variable) {
  size_t end;
  string search = "@PJL SET " + variable + "=\"";
  if (line.find(search) != 0) {
    return "";
  }
  end = line.rfind('"');
  if (end == string::npos) {
    return "";
  }
  return line.substr(search.length(),
                     line.length() - search.length() - (line.length() - end));
}

static void parse(PJLSession &session) {
  if (session.computer == "" &&
      (session.computer = variable(session.line, "PCNAME")) != "") {
    return;
  }
  if (session.user == "" &&
      (session.user = variable(session.line, "USERNAME")) != "") {
    return;
  }
  if (session.title == "" && session.line.substr(0, 8) == "%%Title:") {
    session.title = session.line.substr(9, string::npos);
    return;
  }
  if (session.line.substr(0, 7) == "%%Page:") {
    ++session.pages;
  }
}

extern "C" {
  int initialize(const Configuration &conf, Logger &logger, string &error) {
    int _error;
    timeout = conf.getNumber("timeout");
    ::logger = &logger;
    /*
     * Rehash the session table for as many sessions as we may need to hold in
     * it.
     */
    sessions.rehash(conf.getNumber("maxSessions"));
    if (!memory.initialize(conf.getNumber("maxSessions"))) {
      error = memory.error();
      return 1;
    }
    //bufferSize = conf.getNumber("maxBufferSize") * 1024 * 1024;
    /*
     * We will be locking the sessions hash table one bucket at a time, so
     * allocate one mutex per bucket.
     */
    locks = new(nothrow) pthread_mutex_t[sessions.bucket_count()];
    if (locks == NULL) {
      error = "malloc(): ";
      error += strerror(errno);
      return 1;
    }
    for (size_t i = 0; i < sessions.bucket_count(); ++i) {
      _error = pthread_mutex_init(&(locks[i]), NULL);
      if (_error != 0) {
        error = "pthread_mutex_init(): ";
        error += strerror(_error);
        return 1;
      }
    }
    if (!writer.initialize(conf.getString("data"), "pjl", timeout,
                           &makeRecord)) {
      error = writer.error();
      return 1;
    }
    return 0;
  }

  int processPacket(const Packet &packet) {
    /*
     * The flow ID uniquely identifies a session between a client and server.
     */
    static FlowID flowID;
    static size_t bucket;
    static const u_char *start, *end;
    /*
     * Avoid spending time on packets without payloads. The TCP handshake
     * consists of three such packets, for example.
     *
     * Also avoid spending time on fragmented packets. It seems that the
     * sensor, as opposed to a module, is the ideal place for a
     * fragment-reassembly engine, but none has been written yet.
     */
    if (packet.payloadSize() == 0 || packet.fragmented() == true) {
      return 0;
    }
    flowID.set(packet.protocol(), packet.sourceIP(), packet.destinationIP(),
               packet.sourcePort(), packet.destinationPort());
    bucket = sessions.bucket(flowID.data());
    /*
     * Lock the bucket this flow ID might belong to to prevent a race with
     * flush().
     */
    pthread_mutex_lock(&(locks[bucket]));
    /* Check the session table for a session with this flow ID. */
    itr = sessions.find(flowID.data());
    if (itr == sessions.end()) {
      session = memory.allocate();
      if (session == shared_ptr <PJLSession>()) {
        pthread_mutex_unlock(&(locks[bucket]));
        if (sessionWarning == true) {
          logger -> lock();
          (*logger) << logger -> time()
                    << "PJL module: session table is full." << endl;
          logger -> unlock();
          sessionWarning = false;
        }
        return 0;
      }
      itr = sessions.insert(make_pair(flowID.data(), session)).first;
      itr -> second -> startTime = packet.time();
      memcpy(itr -> second -> clientMAC, packet.sourceMAC(), ETHER_ADDR_LEN);
      memcpy(itr -> second -> serverMAC, packet.destinationMAC(),
             ETHER_ADDR_LEN);
      itr -> second -> clientIP = packet.sourceIP();
      itr -> second -> serverIP = packet.destinationIP();
      itr -> second -> clientPort = packet.sourcePort();
      itr -> second -> serverPort = packet.destinationPort();
      itr -> second -> size = 0;
      itr -> second -> pages = 0;
      itr -> second -> outOfMemory = 0;
    }
    itr -> second -> lastUpdate = packet.time().seconds();
    itr -> second -> size += packet.payloadSize();
    end = packet.payload();
    end = (const u_char*)memchr(packet.payload(), '\n', packet.payloadSize());
    if (end == NULL) {
      itr -> second -> line.append((const char*)packet.payload(),
                                   packet.payloadSize());
    }
    else {
      start = packet.payload();
      while (end != NULL) {
        itr -> second -> line.append((const char*)start, end - start);
        parse(*(itr -> second));
        itr -> second -> line.clear();
        if (end < packet.payload() + packet.payloadSize() - 1) {
          start = end + 1;
          end = (const u_char*)memchr(start, '\n',
                                      packet.payload() + packet.payloadSize() - start);
        }
        else {
          pthread_mutex_unlock(&(locks[bucket]));
          return 0;
        }
      }
      itr -> second -> line.append((const char*)start,
                                   packet.payload() + packet.payloadSize() - start);
    }
    pthread_mutex_unlock(&(locks[bucket]));
    return 0;
  }

  int flush() {
    static time_t _time;
    static unordered_map <string, shared_ptr <PJLSession> >::local_iterator localItr;
    static vector <string> erase;
    _time = time(NULL);
    /*
     * To avoid cluttering the log, only warn about the session table being
     * full, and not having any more job buffer memory, a maximum of once per
     * flush() call.
     */
    sessionWarning = true;
    bufferWarning = true;
    if (sessions.size() > 0) {
      for (size_t i = 0; i < sessions.bucket_count(); ++i) {
        /*
         * Lock the bucket in which we will be checking for timed-out sessions
         * to prevent a race with processPacket().
         */
        pthread_mutex_lock(&(locks[i]));
        for (localItr = sessions.begin(i); localItr != sessions.end(i);
             ++localItr) {
          /*
           * Remove a session from memory if it has been idle for at last as
           * long as the configured idle timeout.
           */
          if (_time - localItr -> second -> lastUpdate >= timeout) {
            parse(*(localItr -> second));
            localItr -> second -> line.clear();
            writer.write(localItr -> second,
                         localItr -> second -> startTime.seconds());
            erase.push_back(localItr -> first);
          }
        }
        for (size_t j = 0; j < erase.size(); ++j) {
          sessions.erase(sessions.find(erase[j]));
        }
        pthread_mutex_unlock(&(locks[i]));
        erase.clear();
      }
    }
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
