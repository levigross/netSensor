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
 * This work was sponsored in part by Polytechnic Institute of NYU's business
 * incubator at 160 Varick Street, New York, New York.
 */

#include <cerrno>
#include <cstring>
#include <ctime>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <tr1/memory>
#include <tr1/unordered_map>

#include <include/address.h>
#include <include/configuration.h>
#include <include/endian.h>
#include <include/flowID.h>
#include <include/httpSession.h>
#include <include/logger.h>
#include <include/memory.hpp>
#include <include/packet.h>
#include <include/smtp.h>

#include "udpTrackerSession.h"

using namespace std;
using namespace tr1;

/*
 * The session table is a hash table of shared pointers to UDPTrackerSession
 * classes with flow IDs as keys.
 */
static unordered_map <string, shared_ptr <UDPTrackerSession> > sessions;
static unordered_map <string, shared_ptr <UDPTrackerSession> >::iterator sessionItr;
static shared_ptr <UDPTrackerSession> session;
/* Session memory allocator. */
static Memory <UDPTrackerSession> memory;
/* Locks for the session table. */
static pthread_mutex_t *locks;
static bool warning = true;
static uint32_t timeout;
static Logger *logger;

static SMTP smtp;

enum MessageType { REQUEST, RESPONSE };
enum Action { CONNECT, ANNOUNCE, SCRAPE, ERROR };

static const string pad(const string _string, size_t length) {
  if (_string.length() < length * 8) {
    return _string + string(length - _string.length() / 8, '\t');
  }
  return _string;
}

/* Converts HTTP tracker session to human-readable form for e-mail. */
static void printHTTP(const HTTPSession &session, ostringstream &message) {
  message << "HTTP tracker session:" << endl;
  for (size_t i = 0; i < session.requests.size(); ++i) {
    message << endl << "Message type:\t\t\trequest" << endl;
    message << "Time:\t\t\t\t" << session.requests[i].time.string() << endl;
    message << "Client Ethernet address:\t" << textMAC(session.clientMAC) << endl;
    message << "Client IPv4 address:\t\t" << textIP(session.clientIP) << endl;
    message << "Client port:\t\t\t" << ntohs(session.clientPort) << endl;
    message << "Server Ethernet address:\t" << textMAC(session.serverMAC) << endl;
    message << "Server IPv4 address:\t\t" << textIP(session.serverIP) << endl;
    message << "Server port:\t\t\t" << ntohs(session.serverPort) << endl;
    message << "Request method:\t\t\t" << session.requests[i].message[0] << endl;
    message << "Path:\t\t\t\t" << session.requests[i].message[1] << endl;
    if (session.requests[i].message[2].length() > 0) {
      message << "Query string:\t\t\t" << session.requests[i].message[2] << endl;
    }
    if (session.requests[i].message[3].length() > 0) {
      message << "Fragment:\t\t\t" << session.requests[i].message[3] << endl;
    }
    message << "Protocol version:\t\tHTTP/" << session.requests[i].message[4] << endl;
    if (session.requests[i].headers.size() > 0) {
      for (size_t j = 0; j < session.requests[i].headers.size(); ++j) {
        message << pad("Header/" + session.requests[i].headers[j].first + ':', 4)
                << session.requests[i].headers[j].second << endl;
      }
    }
  }
  for (size_t i = 0; i < session.responses.size(); ++i) {
    message << endl << "Message type:\t\t\tresponse" << endl;
    message << "Time:\t\t\t\t" << session.responses[i].time.string() << endl;
    message << "Client Ethernet address:\t" << textMAC(session.clientMAC) << endl;
    message << "Client IPv4 address:\t\t" << textIP(session.clientIP) << endl;
    message << "Client port:\t\t\t" << ntohs(session.clientPort) << endl;
    message << "Server Ethernet address:\t" << textMAC(session.serverMAC) << endl;
    message << "Server IPv4 address:\t\t" << textIP(session.serverIP) << endl;
    message << "Server port:\t\t\t" << ntohs(session.serverPort) << endl;
    message << "Protocol version:\t\tHTTP/" << session.responses[i].message[0] << endl;
    message << "Response code:\t\t\t" << session.responses[i].message[1] << endl;
    if (session.responses[i].headers.size() > 0) {
      for (size_t j = 0; j < session.responses[i].headers.size(); ++j) {
        message << pad("Header/" + session.responses[i].headers[j].first + ':', 4)
                << session.responses[i].headers[j].second << endl;
      }
    }
  }
}

/* Converts SHA-1 info hash to human-readable form. */
static string infoHash(const char *infoHash) {
  static ostringstream _infoHash;
  _infoHash.str("");
  for (size_t i = 0; i < 20; ++i) {
    _infoHash << setfill('0') << setw(2) << hex << (int)(uint8_t)(infoHash[i]);
  }
  return _infoHash.str();
}

/*
 * Converts peer ID to human-readable form. Peer ID format:
 *
 * http://www.bittorrent.org/beps/bep_0020.html
 */
static string peerID(const char *peerID) {
  static ostringstream _peerID;
  _peerID.str("");
  _peerID << string(peerID, 8);
  for (size_t i = 8; i < 20; ++i) {
    _peerID << setfill('0') << setw(2) << hex << (int)(uint8_t)(peerID[i]);
  }
  return _peerID.str();
}

/* Converts announce event to human-readable form. */
static string event(const uint32_t &event) {
  ostringstream _event;
  switch (event) {
    case 0:
      return "none";
      break;
    case 1:
      return "completed";
      break;
    case 2:
      return "started";
      break;
    case 3:
      return "stopped";
      break;
  }
  _event.str("");
  _event << "unknown (" << event << ')';
  return _event.str();
}

/* Converts number of bytes to human-readable form. */
static string size(double bytes) {
  ostringstream size;
  if (bytes == 0) {
    return "0 bytes";
  }
  if (bytes < 1024) {
    size << bytes << " bytes";
    return size.str();
  }
  else {
    size.precision(2);
    size.setf(ios::fixed);
    bytes /= 1024;
    if (bytes < 1024) {
      size << bytes << " KiB";
      return size.str();
    }
    else {
      bytes /= 1024;
      if (bytes < 1024) {
        size << bytes << " MiB";
        return size.str();
      }
      else {
        bytes /= 1024;
        if (bytes < 1024) {
          size << bytes << " GiB";
          return size.str();
        }
        else {
          bytes /= 1024;
          if (bytes < 1024) {
            size << bytes << " TiB";
            return size.str();
          }
        }
      }
    }
  }
  bytes /= 1024;
  size << bytes << "PiB";
  return size.str();
}

/* Converts announce IPv4 to human-readable form. */
static string ip(const uint32_t &ip) {
  if (ip == 0) {
    return "0 (use client IP address)";
  }
  return textIP(htonl(ip));
}

/* Converts UDP tracker session to human-readable form for e-mail. */
static void printUDP(const UDPTrackerSession &session, ostringstream &message) {
  message << "UDP tracker session:\t\t" << endl << endl;
  message << "Client Ethernet address:\t" << textMAC(session.clientMAC()) << endl;
  message << "Client IPv4 address:\t\t" << textIP(session.clientIP()) << endl;
  message << "Client port:\t\t\t" << ntohs(session.clientPort()) << endl;
  message << "Tracker Ethernet address:\t" << textMAC(session.serverMAC()) << endl;
  message << "Tracker IPv4 Address:\t\t" << textIP(session.serverIP()) << endl;
  message << "Tracker port:\t\t\t" << ntohs(session.serverPort()) << endl;
  for (size_t i = 0; i < session.announceRequests().size(); ++i) {
    message << endl;
    message << "Message type:\t\t\tannounce request" << endl;
    message << "Time:\t\t\t\t" << session.announceRequests()[i].time().string() << endl;
    message << "Info hash:\t\t\t" << infoHash(session.announceRequests()[i].infoHash()) << endl;
    message << "Peer ID:\t\t\t" << peerID(session.announceRequests()[i].peerID()) << endl;
    message << "Downloaded:\t\t\t" << size(ntohq(session.announceRequests()[i].downloaded())) << endl;
    message << "Left:\t\t\t\t" << size(ntohq(session.announceRequests()[i].left())) << endl;
    message << "Uploaded:\t\t\t" << size(ntohq(session.announceRequests()[i].uploaded())) << endl;
    message << "Event:\t\t\t\t" << event(ntohl(session.announceRequests()[i].event())) << endl;
    message << "IP:\t\t\t\t" << ip(ntohl(session.announceRequests()[i].ip())) << endl;
    message << "Key:\t\t\t\t" << hex << setw(4) << setfill('0') << session.announceRequests()[i].key() << endl;
    message << "Peers wanted:\t\t\t" << dec << ntohl(session.announceRequests()[i].peers()) << endl;
    message << "Port:\t\t\t\t" << ntohs(session.announceRequests()[i].port()) << endl;
  }
  for (size_t i = 0; i < session.announceResponses().size(); ++i) {
    message << endl;
    message << "Message type:\t\t\tannounce response" << endl;
    message << "Time:\t\t\t\t" << session.announceResponses()[i].time().string() << endl;
    message << "Announce interval:\t\t" << ntohl(session.announceResponses()[i].interval()) << " seconds" << endl;
    message << "Leechers:\t\t\t" << ntohl(session.announceResponses()[i].leechers()) << endl;
    message << "Seeders:\t\t\t" << ntohl(session.announceResponses()[i].seeders()) << endl;
    for (size_t j = 0; j < session.announceResponses()[i].peers().size(); ++j) {
      if (j < 9) {
        message << "Peer " << j + 1 << ":\t\t\t\t";
      }
      else {
        message << "Peer " << j + 1 << ":\t\t\t";
      }
      message << textIP(session.announceResponses()[i].peers()[j].ip()) << ':'
              << ntohs(session.announceResponses()[i].peers()[j].port())
              << endl;
    }
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
    /*
     * Because UDPTrackerSession classes are fairly small, we will
     * pre-allocate as many of them as we may need so that we can later hand
     * them out in constant time.
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
    if (!smtp.initialize(conf.getString("smtpServer"),
                         conf.getNumber("smtpAuth"),
                         conf.getString("smtpUser"),
                         conf.getString("smtpPassword"),
                         conf.getString("senderName"),
                         conf.getString("senderAddress"),
                         conf.getStrings("recipient"))) {
      error = smtp.error();
      return 1;
    }
    return 0;
  }

  int processPacket(const Packet &packet) {
    static FlowID flowID;
    static size_t bucket;
    static MessageType messageType;
    /*
     * Initial connection ID for the UDP tracker protocol. UDP tracker protocol
     * specification:
     *
     * http://www.bittorrent.org/beps/bep_0015.html
     */
    static const uint64_t initialConnectionID = ntohq(0x0000041727101980LLU);
    /*
     * Avoid spending time on packets with less than 16 bytes of payload data,
     * as that is the minimum size of a valid message between a client and a
     * UDP tracker.
     *
     * Also avoid spending time on fragmented packets. It seems that the sensor,
     * as opposed to a module, is the ideal place for a fragment-reassembly
     * engine, but none has been written yet.
     */
    if (packet.payloadSize() < 16 || packet.fragmented() == true) {
      return 0;
    }
    messageType = REQUEST;
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
     * saw earlier. The session created by that request would have been UDP
     * traffic in the opposite direction of this packet and would have
     * therefore had a flow ID with this packet's source IPs and ports swapped
     * with this its destination IPs and ports, respectively, so we will craft
     * that flow ID and look for it in the session table.
     */
    if (sessionItr == sessions.end()) {
      messageType = RESPONSE;
      pthread_mutex_unlock(&(locks[bucket]));
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
      switch (messageType) {
        case REQUEST:
          /* Store transaction ID. */
          sessionItr -> second -> transactionID(*(uint32_t*)(packet.payload() + 12));
          /* Check action. */
          switch (ntohl(*(uint32_t*)(packet.payload() + 8))) {
            case ANNOUNCE:
              if (packet.payloadSize() >= 98 &&
                  *(uint64_t*)packet.payload() == sessionItr -> second -> connectionID()) {
                sessionItr -> second -> addAnnounceRequest(packet);
              }
              break;
            case SCRAPE:
              if (packet.payloadSize() >= 36 &&
                  *(uint64_t*)packet.payload() == sessionItr -> second -> connectionID()) {
                sessionItr -> second -> addScrapeRequest(packet);
              }
              break;
          }
          break;
        case RESPONSE:
          /* Check transaction ID. */
          if (*(uint32_t*)(packet.payload() + 4) == sessionItr -> second -> transactionID()) {
            /* Check action. */
            switch (ntohl(*(uint32_t*)packet.payload())) {
              case CONNECT:
                sessionItr -> second -> time(packet.time());
                sessionItr -> second -> connectionID(*(uint64_t*)(packet.payload() + 8));
                break;
              case ANNOUNCE:
                if (packet.payloadSize() >= 20) {
                  sessionItr -> second -> addAnnounceResponse(packet);
                }
                break;
              case SCRAPE:
                if (packet.payloadSize() >= 20) {
                  sessionItr -> second -> addScrapeResponse(packet);
                }
                break;
            }
        }
      }
      pthread_mutex_unlock(&(locks[bucket]));
    }
    /*
     * Otherwise, this packet potentially contains data belonging to a new
     * session, so we will check for that, and, if it is the case, we will
     * allocate a UDPTrackerSession class and insert it into the session table
     * with the originally-computed flow ID as its key.
     */
    else {
      pthread_mutex_unlock(&(locks[bucket]));
      if (*(uint64_t*)packet.payload() == initialConnectionID &&
          *(uint32_t*)(packet.payload() + 8) == CONNECT) {
        /* Lock the memory allocator to prevent a race with flush(). */
        session = memory.allocate();
        if (session == shared_ptr <UDPTrackerSession>()) {
          if (warning == true) {
            logger -> lock();
            (*logger) << logger -> time()
                      << "BitTorrent module: session table is full." << endl;
            logger -> unlock();
            warning = false;
          }
          return 0;
        } 
        session -> initialize(packet);
        flowID.set(packet.protocol(), packet.sourceIP(),
                   packet.destinationIP(), packet.sourcePort(),
                   packet.destinationPort());
        bucket = sessions.bucket(flowID.data());
        /*
         * Lock the bucket this flow ID belongs to to prevent a race with
         * flush().
         */
        pthread_mutex_lock(&(locks[bucket]));
        sessions.insert(make_pair(flowID.data(), session));
        pthread_mutex_unlock(&(locks[bucket]));
      }
    }
    return 0;
  }

  int processHTTP(const shared_ptr <HTTPSession> session) {
    static string path;
    for (size_t i = 0; i < session -> requests.size(); ++i) {
      path = session -> requests[i].message[1];
      /* Detect HTTP torrent file downloads. */
      if (path.length() >= 8 && strcasecmp(path.substr(path.length() - 8).c_str(),
                                                       ".torrent") == 0) {
        /* Lock the SMTP client to prevent a race with flush(). */
        smtp.lock();
        smtp.subject() << "Torrent file download by "
                       << textIP(session -> clientIP) << " ("
                       << textMAC(session -> clientMAC) << ") detected";
        printHTTP(*session, smtp.message());
        if (!smtp.send()) {
          logger -> lock();
          (*logger) << logger -> time() << "BitTorrent module: smtp::send(): "
                    << smtp.error() << endl;
          logger -> unlock();
        }
        smtp.subject().str("");
        smtp.message().str("");
        smtp.unlock();
      }
      /* Detect HTTP tracker communication. */
      if (session -> requests[i].message[2].find("info_hash") != string::npos) {
        /* Lock the SMTP client to prevent a race with flush(). */
        smtp.lock();
        smtp.subject() << "HTTP tracker communication by "
                       << textIP(session -> clientIP) << " ("
                       << textMAC(session -> clientMAC) << ") detected";
        printHTTP(*session, smtp.message());
        if (!smtp.send()) {
          logger -> lock();
          (*logger) << logger -> time() << "BitTorrent module: smtp::send(): "
                    << smtp.error() << endl;
          logger -> unlock();
        }
        smtp.subject().str("");
        smtp.message().str("");
        smtp.unlock();
      }
    }
    return 0;
  }

  int flush() {
    static time_t _time;
    static unordered_map <string, shared_ptr <UDPTrackerSession> >::local_iterator localItr;
    static vector <string> erase;
    _time = time(NULL);
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
           * Remove a session from memory if it has been idle for at least as
           * long as the configured idle timeout.
           */
          if (_time - localItr -> second -> time().seconds() >= timeout) { 
            /*
             * We're only interested in sessions with at least one announce
             * or scrape request or response.
             */
            if (localItr -> second -> announceRequests().size() > 0 ||
                localItr -> second -> announceResponses().size() > 0) {
              /* Lock the SMTP client to prevent a race with processHTTP(). */
              smtp.lock();
              smtp.subject() << "UDP tracker communication by "
                             << textIP(localItr -> second -> clientIP()) << " ("
                             << textMAC(localItr -> second -> clientMAC())
                             << ") detected";
              printUDP(*(localItr -> second), smtp.message());
              if (!smtp.send()) {
                logger -> lock();
                (*logger) << logger -> time() << "BitTorrent: smtp::send(): "
                          << smtp.error() << endl;
                logger -> unlock();
              }
              smtp.subject().str("");
              smtp.message().str("");
              smtp.unlock();
            }
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
    return 0;
  }

  int finish() {
    return 0;
  }
}
