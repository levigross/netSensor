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

#include <cstring>

#include "udpTrackerSession.h"

UDPTrackerSession::UDPTrackerSession() {}

void UDPTrackerSession::initialize(const Packet &packet) {
  if (packet.payloadSize() >= 16) {
    _time = packet.time();
    memcpy(_clientMAC, packet.sourceMAC(), ETHER_ADDR_LEN);
    memcpy(_serverMAC, packet.destinationMAC(), ETHER_ADDR_LEN);
    _clientIP = packet.sourceIP();
    _clientPort = packet.sourcePort();
    _serverIP = packet.destinationIP();
    _serverPort = packet.destinationPort();
    _transactionID = *(uint32_t*)(packet.payload() + 12);
  }
}

const TimeStamp &UDPTrackerSession::time() const {
  return _time;
}

void UDPTrackerSession::time(const TimeStamp &time) {
  _time = time;
}

const char *UDPTrackerSession::clientMAC() const {
  return _clientMAC;
}

const char *UDPTrackerSession::serverMAC() const {
  return _serverMAC;
}

const uint32_t &UDPTrackerSession::clientIP() const {
  return _clientIP;
}

const uint16_t &UDPTrackerSession::clientPort() const {
  return _clientPort;
}

const uint32_t &UDPTrackerSession::serverIP() const {
  return _serverIP;
}

const uint16_t &UDPTrackerSession::serverPort() const {
  return _serverPort;
}

const uint32_t &UDPTrackerSession::transactionID() const {
  return _transactionID;
}

void UDPTrackerSession::transactionID(const uint32_t &transactionID) {
  _transactionID = transactionID;
}

const uint64_t &UDPTrackerSession::connectionID() const {
  return _connectionID;
}

void UDPTrackerSession::connectionID(const uint64_t &connectionID) {
  _connectionID = connectionID;
}

const std::vector <UDPTrackerSession::AnnounceRequest> &UDPTrackerSession::announceRequests() const {
  return _announceRequests;
}

void UDPTrackerSession::addAnnounceRequest(const Packet &packet) {
  if (packet.payloadSize() >= 98) {
    _time = packet.time();
    _announceRequests.push_back(AnnounceRequest(packet));
  }
}

UDPTrackerSession::AnnounceRequest::AnnounceRequest(const Packet &packet) {
  if (packet.payloadSize() >= 98) {
    _time = packet.time();
    memcpy(_infoHash, packet.payload() + 16, sizeof(_infoHash));
    memcpy(_peerID, packet.payload() + 36, sizeof(_peerID));
    _downloaded = *(const uint64_t*)(packet.payload() + 56);
    _left = *(const uint64_t*)(packet.payload() + 64);
    _uploaded = *(const uint64_t*)(packet.payload() + 72);
    _event = *(const uint32_t*)(packet.payload() + 80);
    _ip = *(const uint32_t*)(packet.payload() + 84);
    _key = *(const uint32_t*)(packet.payload() + 88);
    _peers = *(const uint32_t*)(packet.payload() + 92);
    _port = *(const uint16_t*)(packet.payload() + 96);
  }
}

const TimeStamp &UDPTrackerSession::AnnounceRequest::time() const {
  return _time;
}

const char *UDPTrackerSession::AnnounceRequest::infoHash() const {
  return _infoHash;
}

const char *UDPTrackerSession::AnnounceRequest::peerID() const {
  return _peerID;
}

const uint64_t &UDPTrackerSession::AnnounceRequest::downloaded() const {
  return _downloaded;
}

const uint64_t &UDPTrackerSession::AnnounceRequest::left() const {
  return _left;
}

const uint64_t &UDPTrackerSession::AnnounceRequest::uploaded() const {
  return _uploaded;
}

const uint32_t &UDPTrackerSession::AnnounceRequest::event() const {
  return _event;
}

const uint32_t &UDPTrackerSession::AnnounceRequest::ip() const {
  return _ip;
}

const uint32_t &UDPTrackerSession::AnnounceRequest::key() const {
  return _key;
}

const uint32_t &UDPTrackerSession::AnnounceRequest::peers() const {
  return _peers;
}

const uint16_t &UDPTrackerSession::AnnounceRequest::port() const {
  return _port;
}

const std::vector <UDPTrackerSession::AnnounceResponse> &UDPTrackerSession::announceResponses() const {
  return _announceResponses;
}

void UDPTrackerSession::addAnnounceResponse(const Packet &packet) {
  if (packet.payloadSize() >= 20) {
    _time = packet.time();
    _announceResponses.push_back(AnnounceResponse(packet));
  }
}

UDPTrackerSession::AnnounceResponse::AnnounceResponse(const Packet &packet) {
  uint16_t offset = 20;
  const uint32_t *ip;
  const uint16_t *port;
  if (packet.payloadSize() >= 20) {
    _time = packet.time();
    _interval = *(const uint32_t*)(packet.payload() + 8);
    _leechers = *(const uint32_t*)(packet.payload() + 12);
    _seeders = *(const uint32_t*)(packet.payload() + 16);
    while (packet.payloadSize() >= offset + 6) {
      ip = (uint32_t*)(packet.payload() + offset);
      port = (uint16_t*)(packet.payload() + offset + 4);
      _peers.push_back(Peer(*ip, *port));
      offset += 6;
    }
  }
}

const TimeStamp &UDPTrackerSession::AnnounceResponse::time() const {
  return _time;
}

const uint32_t &UDPTrackerSession::AnnounceResponse::interval() const {
  return _interval;
}

const uint32_t &UDPTrackerSession::AnnounceResponse::leechers() const {
  return _leechers;
}

const uint32_t &UDPTrackerSession::AnnounceResponse::seeders() const {
  return _seeders;
}

UDPTrackerSession::AnnounceResponse::Peer::Peer(const uint32_t &ip,
                                                const uint16_t &port) {
  _ip = ip;
  _port = port;
}

const uint32_t &UDPTrackerSession::AnnounceResponse::Peer::ip() const {
  return _ip;
}

const uint16_t &UDPTrackerSession::AnnounceResponse::Peer::port() const {
  return _port;
}

const std::vector <UDPTrackerSession::AnnounceResponse::Peer> &UDPTrackerSession::AnnounceResponse::peers() const {
  return _peers;
}

void UDPTrackerSession::addScrapeRequest(const Packet &packet) {
  if (packet.payloadSize() >= 36) {
    _time = packet.time();
    _scrapeRequests.push_back(ScrapeRequest(packet));
  }
}

/* XXX untested. */
UDPTrackerSession::ScrapeRequest::ScrapeRequest(const Packet &packet) {
  uint16_t offset = 16;
  if (packet.payloadSize() >= 36) {
    while (packet.payloadSize() >= offset + 20) {
      infoHashes.push_back(std::string((const char*)(packet.payload() + offset),
                           20));
      offset += 20;
    }
  }
}

void UDPTrackerSession::addScrapeResponse(const Packet &packet) {
  if (packet.payloadSize() >= 20) {
    _time = packet.time();
    _scrapeResponses.push_back(ScrapeResponse(packet));
  }
}

/* XXX untested. */
UDPTrackerSession::ScrapeResponse::ScrapeResponse(const Packet &packet) {
  uint16_t offset = 8;
  const uint32_t *seeders, *completed, *leechers;
  if (packet.payloadSize() >= 20) {
    while (packet.payloadSize() >= offset + 12) {
      seeders = (const uint32_t*)(packet.payload() + offset);
      completed = (const uint32_t*)(packet.payload() + offset + 4);
      leechers = (const uint32_t*)(packet.payload() + offset + 8);
      infoHashes.push_back(InfoHash(*seeders, *completed, *leechers));
      offset += 12;
    }
  }
}

UDPTrackerSession::ScrapeResponse::InfoHash::InfoHash(const uint32_t &seeders,
                                                      const uint32_t &completed,
                                                      const uint32_t &leechers) {
  _seeders = seeders;
  _completed = completed;
  _leechers = leechers;
}
