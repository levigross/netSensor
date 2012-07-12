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

#include <cerrno>
#include <cstring>
#include <ctime>

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <tr1/memory>

#include <include/consumers.hpp>
#include <include/flowID.h>
#include <include/httpParser.h>
#include <include/httpSession.h>
#include <include/logger.h>
#include <include/memory.hpp>
#include <include/module.h>

using namespace std;
using namespace tr1;

/* Consumers of HTTP data must have a "processHTTP" function defined. */
const char *callback = "processHTTP";
static Consumers <HTTPSession> consumers;

static http_parser_settings settings;
static const Packet *_packet;
/*
 * The session table is a hash table of shared pointers to HTTPSession
 * structures with flow IDs as keys.
 */
static unordered_map <string, shared_ptr <HTTPSession> > sessions;
static unordered_map <string, shared_ptr <HTTPSession> >::iterator sessionItr;
static shared_ptr <HTTPSession> session;
/* Session memory allocator. */
static Memory <HTTPSession> memory;
/* Locks for the session table. */
static pthread_mutex_t *locks;
static bool warning = true;
static uint32_t timeout;
static Logger *logger;

static int url(http_parser *parser, const char *url __attribute__((unused)),
               size_t length __attribute__((unused))) {
  /* Fill in session's addressing information. */
  memcpy(session -> clientMAC, _packet -> sourceMAC(), ETHER_ADDR_LEN);
  memcpy(session -> serverMAC, _packet -> destinationMAC(), ETHER_ADDR_LEN);
  session -> clientIP = _packet -> sourceIP();
  session -> serverIP = _packet -> destinationIP();
  session -> clientPort = _packet -> sourcePort();
  session -> serverPort = _packet -> destinationPort();
  /*
   * Add request to session and record its time if the path callback hasn't
   * been called prior to this.
   */
  if (session -> requestState != PATH_STATE) {
    session -> requests.push_back(HTTPMessage(HTTP_REQUEST,
                                              _packet -> time()));
  }
  /* Record request method. */
  session -> requests.rbegin() -> message[0] = http_method_str((http_method)(parser -> method));
  session -> requestState = URL_STATE;
  return 0;
}

static int path(http_parser *parser __attribute__((unused)), const char *path,
                size_t length) {
  /*
   * Add request to session and record its time if the URL callback hasn't been
   * called prior to this.
   */
  if (session -> requestState != URL_STATE) {
    session -> requests.push_back(HTTPMessage(HTTP_REQUEST,
                                              _packet -> time()));
  }
  /* Record request path. */
  session -> requests.rbegin() -> message[1] = string(path, length);
  session -> requestState = PATH_STATE;
  return 0;
}

static int queryString(http_parser *parser __attribute__((unused)),
                       const char *queryString, size_t length) {
  session -> requests.rbegin() -> message[2] = string(queryString, length);
  return 0;
}

static int fragment(http_parser *parser __attribute__((unused)),
                    const char *fragment, size_t length) {
  session -> requests.rbegin() -> message[3] = string(fragment, length);
  return 0;
}

static int headerField(http_parser *parser, const char *field,
                       size_t length) {
  static HTTPMessageState *state;
  static vector <pair <string, string> > *headers;
  switch (parser -> type) {
    case HTTP_REQUEST:
      state = &(session -> requestState);
      headers = &(session -> requests.rbegin() -> headers);
      break;
    case HTTP_RESPONSE:
      if (session -> requests.size() == 0 &&
          session -> responses.size() == 0) {
        memcpy(session -> clientMAC, _packet -> destinationMAC(),
               ETHER_ADDR_LEN);
        memcpy(session -> serverMAC, _packet -> sourceMAC(), ETHER_ADDR_LEN);
        session -> clientIP = _packet -> destinationIP();
        session -> serverIP = _packet -> sourceIP();
        session -> clientPort = _packet -> destinationPort();
        session -> serverPort = _packet -> sourcePort();
      }
      state = &(session -> responseState);
      if (*state != HEADER_FIELD_STATE && *state != HEADER_VALUE_STATE) {
        session -> responses.push_back(HTTPMessage(HTTP_RESPONSE,
                                                   _packet -> time()));
      }
      headers = &(session -> responses.rbegin() -> headers);
      break;
  }
  switch (*state) {
    case NO_STATE:
    case PATH_STATE:
    case URL_STATE:
    case COMPLETE_STATE:
      headers -> push_back(make_pair(string(field, length), ""));
      break;
    case HEADER_FIELD_STATE:
      headers -> rbegin() -> first.append(field, length);
      break;
    case HEADER_VALUE_STATE:
      headers -> push_back(make_pair(string(field, length), ""));
      break;
  }
  *state = HEADER_FIELD_STATE;
  return 0;
}

static int headerValue(http_parser *parser, const char *value,
                       size_t length) {
  static HTTPMessageState *state;
  static vector <pair <string, string> > *headers;
  switch (parser -> type) {
    case HTTP_REQUEST:
      state = &(session -> requestState);
      headers = &(session -> requests.rbegin() -> headers);
      break;
    case HTTP_RESPONSE:
      state = &(session -> responseState);
      headers = &(session -> responses.rbegin() -> headers);
      break;
  }
  switch (*state) {
    case HEADER_FIELD_STATE:
      headers -> rbegin() -> second = string(value, length);
      break;
    case HEADER_VALUE_STATE:
      headers -> rbegin() -> second.append(value, length);
      break;
    /*
     * "state" can only be "HEADER_FIELD_STATE" or "HEADER_VALUE_STATE" during
     * this function call, but we have to do this to keep compilers happy.
     */
    default:
      break;
  }
  *state = HEADER_VALUE_STATE;
  return 0;
}

static int headersComplete(http_parser *parser) {
  static ostringstream message;
  switch (parser -> type) {
    case HTTP_REQUEST:
      /* Record request HTTP version. */
      message << parser -> http_major << '.' << parser -> http_major;
      session -> requests.rbegin() -> message[4] = message.str();
      message.str("");
      session -> requestState = COMPLETE_STATE;
      break;
    case HTTP_RESPONSE:
      /*
       * Most responses contain headers and result in a call to headerField(),
       * which adds an HTTPMessage structure to that response's HTTPSession
       * structure. However, a response is not required to contain headers, so
       * we handle that possibility here.
       */
      if (session -> responseState != HEADER_VALUE_STATE) {
        session -> responses.push_back(HTTPMessage(HTTP_RESPONSE,
                                                   _packet -> time()));
      }
      /* Record response HTTP version. */
      message << parser -> http_major << '.' << parser -> http_minor;
      session -> responses.rbegin() -> message.push_back(message.str());
      message.str("");
      /* Record response HTTP status code. */
      message << parser -> status_code;
      session -> responses.rbegin() -> message.push_back(message.str());
      message.str("");
      /* Remove a session from memory if this is its last message. */
      if (http_should_keep_alive(parser) == 0) {
        /*
         * The appropriate bucket in the session table has been locked in
         * processPacket() before this function call, so there is no need to
         * lock it here.
         */
        /*if (sessionItr != sessions.end()) {
          sessions.erase(sessionItr);
          sessionItr = sessions.end();
        }
        consumers.consume(session);*/
      }
      else {
        session -> responseState = COMPLETE_STATE;
      }
      break;
  }
  return 0;
}

extern "C" {
  int initialize(const Configuration &conf, Logger &logger,
                 const vector <void*> &callbacks, string &error) {
    int _error;
    timeout = conf.getNumber("timeout");
    ::logger = &logger;
    /*
     * Rehash the session table for as many sessions as we may need to hold in
     * it.
     */
    sessions.rehash(conf.getNumber("maxSessions"));
    /*
     * Because HTTPSession structures will be allocated very frequently (as
     * often as once per TCP packet with a payload, depending on this module's
     * filter string), we will pre-allocate as many of them as we may need so
     * that we can later hand them out in constant time.
     */
    if (!memory.initialize(conf.getNumber("maxSessions"))) {
      error = memory.error();
      return 1;
    }
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
    consumers.initialize(callbacks);
    /* Set HTTP parser callbacks. */
    settings.on_path = &path;
    settings.on_url = &url;
    settings.on_query_string = &queryString;
    settings.on_fragment = &fragment;
    settings.on_header_field = &headerField;
    settings.on_header_value = &headerValue;
    settings.on_headers_complete = &headersComplete;
    return 0;
  }

  int processPacket(const Packet &packet) {
    /*
     * The flow ID uniquely identifies a session between a client and server.
     */
    static FlowID flowID;
    static size_t parser, bucket;
    static int parsed;
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
    parser = 0;
    flowID.set(packet.protocol(), packet.sourceIP(), packet.destinationIP(),
               packet.sourcePort(), packet.destinationPort());
    bucket = sessions.bucket(flowID.data());
    /*
     * Lock the bucket this flow ID might belong to to prevent a race with
     * flush().
     */
    pthread_mutex_lock(&(locks[bucket]));
    /* Check the session table for a session with this flow ID. */
    sessionItr = sessions.find(flowID.data());
    /*
     * If none exists, we shouldn't insert a new session into the session table
     * just yet, as this packet might be part of the response to a request we
     * saw earlier. The session created by that request would have been TCP
     * traffic in the opposite direction of this packet and would have
     * therefore had a flow ID with this packet's source IPv4 addresses and
     * ports swapped with this its destination IPv4 addresses and ports,
     * respectively, so we will craft that flow ID and look for it in the
     * session table.
     */
    if (sessionItr == sessions.end()) {
      pthread_mutex_unlock(&(locks[bucket]));
      parser = 1;
      flowID.set(packet.protocol(), packet.destinationIP(), packet.sourceIP(),
                 packet.destinationPort(), packet.sourcePort());
      bucket = sessions.bucket(flowID.data());
      /*
       * Lock the bucket this flow ID might belong to to prevent a race with
       * flush().
       */
      pthread_mutex_lock(&(locks[bucket]));
      sessionItr = sessions.find(flowID.data());
    }
    /*
     * If "sessionItr" is valid at this point, there is some request or
     * response data to be parsed for an existing session.
     */
    if (sessionItr != sessions.end()) {
      session = sessionItr -> second;
      _packet = &packet;
      parsed = http_parser_execute(&(sessionItr -> second -> parsers[parser]),
                                   &settings, (const char*)packet.payload(),
                                   packet.payloadSize());
      if (parsed != packet.payloadSize()) {
        if (sessionItr != sessions.end()) {
          sessions.erase(sessionItr);
        }
        pthread_mutex_unlock(&(locks[bucket]));
        return 0;
      }
      pthread_mutex_unlock(&(locks[bucket]));
    }
    /*
     * Otherwise, this packet potentially contains data belonging to a new
     * session, so we will allocate an HTTPSession structure and attempt to
     * parse the packet's payload. If the parser determines that the packet
     * contains valid request or response data:
     *
     * - If the packet contains only complete responses, the session will be
     *   removed from memory by headersComplete() under the assumption that we
     *   we did not see the requests associated with this session, and never
     *   will.
     *
     * - Otherwise, the HTTPSession structure will be inserted into the session
     *   table with the originally-computed flow ID as its key.
     */
    else {
      pthread_mutex_unlock(&(locks[bucket]));
      /* Lock the memory allocator to prevent a race with flush(). */
      session = memory.allocate();
      if (session == shared_ptr <HTTPSession>()) {
        if (warning == true) {
          logger -> lock();
          (*logger) << logger -> time()
                    << "HTTP module: session table is full." << endl;
          logger -> unlock();
          warning = false;
        }
        return 0;
      }
      _packet = &packet;
      session -> time = packet.time();
      parsed = http_parser_execute(&(session -> parsers[0]), &settings,
                                   (const char*)packet.payload(),
                                   packet.payloadSize());
      if (parsed != packet.payloadSize()) {
        return 0;
      }
      flowID.set(packet.protocol(), packet.sourceIP(),
                 packet.destinationIP(), packet.sourcePort(),
                 packet.destinationPort());
      bucket = sessions.bucket(flowID.data());
      /*
       * Lock the bucket this flow ID belongs to to prevent a race with
       * flush().
       */
      pthread_mutex_lock(&(locks[bucket]));
      sessionItr = sessions.insert(make_pair(flowID.data(), session)).first;
      pthread_mutex_unlock(&(locks[bucket]));
    }
    return 0;
  }

  int flush() {
    static time_t _time;
    static unordered_map <string, shared_ptr <HTTPSession> >::local_iterator localItr;
    static vector <pair <string, shared_ptr <HTTPSession> > > erase;
    _time = time(NULL);
    /*
     * To avoid cluttering the log, only warn about the session table being
     * full a maximum of once per flush() call.
     */
    warning = true;
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
          if ((localItr -> second -> requests.size() > 0 &&
               localItr -> second -> requests.rbegin() -> time.seconds() != 0 &&
               _time - localItr -> second -> requests.rbegin() -> time.seconds() >= timeout) ||
              (localItr -> second -> responses.size() > 0 &&
               localItr -> second -> responses.rbegin() -> time.seconds() != 0 &&
               _time - localItr -> second -> responses.rbegin() -> time.seconds() >= timeout) ||
               _time - localItr -> second -> time.seconds() >= timeout) {
              erase.push_back(make_pair(localItr -> first, localItr -> second));
          }
        }
        for (size_t j = 0; j < erase.size(); ++j) { 
          /*
           * We're only interested in doing anything with sessions with at
           * least one request or response.
           */
          if (erase[j].second -> requests.size() > 0 ||
              erase[j].second -> responses.size() > 0) {
            consumers.consume(erase[j].second);
          }
          sessions.erase(sessions.find(erase[j].first));
        }
        pthread_mutex_unlock(&(locks[i]));
        erase.clear();
      }
    } 
    return 0;
  }

  int finish() {
    return 0;
  }
}
