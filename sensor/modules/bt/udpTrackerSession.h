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

#ifndef UDP_TRACKER_SESSION_H
#define UDP_TRACKER_SESSION_H

#include <string>
#include <vector>

#include <sys/socket.h>

#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <netinet/if_ether.h>
#endif
#ifdef __linux__
#include <netinet/ether.h>
#endif
#include <stdint.h>

#include <include/endian.h>
#include <include/packet.h>
#include <include/timeStamp.h>

class UDPTrackerSession {
  public:
    UDPTrackerSession();
    void initialize(const Packet &packet);
    const TimeStamp &time() const;
    void time(const TimeStamp &time);
    const char *clientMAC() const;
    const char *serverMAC() const;
    const uint32_t &clientIP() const;
    const uint16_t &clientPort() const;
    const uint32_t &serverIP() const;
    const uint16_t &serverPort() const;
    const uint32_t &transactionID() const;
    void transactionID(const uint32_t &transactionID);
    const uint64_t &connectionID() const;
    void connectionID(const uint64_t &connectionID);
    void addAnnounceRequest(const Packet &packet);
    /* Announce request class. */
    class AnnounceRequest {
      public:
        AnnounceRequest(const Packet &packet);
        const TimeStamp &time() const;
        const char *infoHash() const;
        const char *peerID() const;
        const uint64_t &downloaded() const;
        const uint64_t &left() const;
        const uint64_t &uploaded() const;
        const uint32_t &event() const;
        const uint32_t &ip() const;
        const uint32_t &key() const;
        const uint32_t &peers() const;
        const uint16_t &port() const;
      private:
        TimeStamp _time;
        char _infoHash[20];
        char _peerID[20];
        uint64_t _downloaded;
        uint64_t _left;
        uint64_t _uploaded;
        uint32_t _event;
        uint32_t _ip;
        uint32_t _key;
        uint32_t _peers;
        uint16_t _port;
    };
    const std::vector <AnnounceRequest> &announceRequests() const;
    void addAnnounceResponse(const Packet &packet);
    /* Announce response class. */
    class AnnounceResponse {
      public:
        AnnounceResponse(const Packet &packet);
        const TimeStamp &time() const;
        const uint32_t &interval() const;
        const uint32_t &leechers() const;
        const uint32_t &seeders() const;
        /* Peer class. */
        class Peer {
          public:
            Peer(const uint32_t &ip, const uint16_t &port);
            const uint32_t &ip() const;
            const uint16_t &port() const;
          private:
            uint32_t _ip;
            uint16_t _port;
        };
        const std::vector <Peer> &peers() const;
      private:
        TimeStamp _time;
        uint32_t _interval;
        uint32_t _leechers;
        uint32_t _seeders;
        std::vector <Peer> _peers;
    };
    const std::vector <AnnounceResponse> &announceResponses() const;
    void addScrapeRequest(const Packet &packet);
    /* Scrape request class. */
    class ScrapeRequest {
      public:
        ScrapeRequest(const Packet &packet);
      private:
        TimeStamp time;
        std::vector <std::string> infoHashes;
    };
    const std::vector <ScrapeRequest> &scrapeRequests() const;
    void addScrapeResponse(const Packet &packet);
    /* Scrape response class. */
    class ScrapeResponse {
      public:
        ScrapeResponse(const Packet &packet);
      private:
        TimeStamp time;
        /* Info hash class. */
        class InfoHash {
          public:
            InfoHash(const uint32_t &seeders, const uint32_t &completed,
                     const uint32_t &leechers);
          private:
            uint32_t _seeders;
            uint32_t _completed;
            uint32_t _leechers;
        };
        std::vector <InfoHash> infoHashes;
    };
    const std::vector <ScrapeResponse> scrapeResponses() const;
    void addError(const Packet &packet);
    /* Error class. */
    class Error {
      public:
        Error(const Packet &packet);
      private:
        TimeStamp time;
        std::string error;
    };
    const std::vector <Error> errors() const;
  private:
    TimeStamp _time;
    char _clientMAC[ETHER_ADDR_LEN];
    char _serverMAC[ETHER_ADDR_LEN];
    uint32_t _clientIP;
    uint16_t _clientPort;
    uint32_t _serverIP;
    uint16_t _serverPort;
    uint32_t _transactionID;
    uint64_t _connectionID;
    std::vector <AnnounceRequest> _announceRequests;
    std::vector <AnnounceResponse> _announceResponses;
    std::vector <ScrapeRequest> _scrapeRequests;
    std::vector <ScrapeResponse> _scrapeResponses;
    std::vector <Error> _errors;
};

#endif
