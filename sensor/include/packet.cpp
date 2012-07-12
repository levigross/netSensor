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

#include "packet.h"

bool Packet::initialize(const pcap_pkthdr &pcapHeader, const u_char *pcapPacket) {
  /* Copy timestamp to our more-portable format. */
  _time = pcapHeader.ts;
  _capturedSize = pcapHeader.caplen;
  _packet = pcapPacket;
  /* Determine if the packet is fragmented. */
  if ((ntohs(((ip*)(_packet + sizeof(ether_header))) -> ip_off) & IP_MF) != 0 ||
      (ntohs(((ip*)(_packet + sizeof(ether_header))) -> ip_off) & IP_OFFMASK) != 0) {
    _fragmented = true;
  }
  else {
    _fragmented = false;
  }
  /* Determine payload size and offset into the packet. */
  if (_fragmented == false) {
    switch (protocol()) {
      case IPPROTO_ICMP:
        /* Discard malformed ICMP packets. */
        if (sizeof(ether_header) + sizeof(ip) + sizeof(icmphdr) > pcapHeader.caplen) {
          return false;
        }
        _payloadSize = pcapHeader.caplen - (sizeof(ether_header) + sizeof(ip) + sizeof(icmphdr));
        break;
      case IPPROTO_TCP:
        /* Discard malformed TCP packets. */
        if (sizeof(ether_header) + sizeof(ip) + (((tcphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> th_off << 2) > pcapHeader.caplen) {
          return false;
        }
        _payloadSize = pcapHeader.caplen - (sizeof(ether_header) + sizeof(ip) + (((tcphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> th_off << 2));
        break;
      case IPPROTO_UDP:
        /* Discard malformed UDP packets. */
        if (sizeof(ether_header) + sizeof(ip) + sizeof(udphdr) > pcapHeader.caplen) {
          return false;
        }
        _payloadSize = pcapHeader.caplen - (sizeof(ether_header) + sizeof(ip) + sizeof(udphdr));
        break;
    }
  }
  else {
    _payloadSize = pcapHeader.caplen - (sizeof(ether_header) + sizeof(ip));
  }
  _payload = _packet + (pcapHeader.caplen - _payloadSize);
  return true;
}

const TimeStamp &Packet::time() const {
  return _time;
}

const u_char *Packet::packet() const {
  return _packet;
}

const u_char *Packet::sourceMAC() const {
  return _packet + ETHER_ADDR_LEN;
}

const u_char *Packet::destinationMAC() const {
  return _packet;
}

const bool &Packet::fragmented() const {
  return _fragmented;
}

const uint8_t &Packet::ttl() const {
  return ((ip*)(_packet + sizeof(ether_header))) -> ip_ttl;
}

const uint8_t &Packet::protocol() const {
  return ((ip*)(_packet + sizeof(ether_header))) -> ip_p;
}

/*
 * Ideally, for cosmetic reasons, we would like to do something like this:
 *
 *   return ((ip*)(_packet + sizeof(ether_header))) -> ip_src.s_addr;
 *
 * However, because the "ip" structure is padded in the Berkeley socket API,
 * GCC complains about "returning reference to temporary" and generates
 * undesirable code, so we'll just count the bytes ourselves.
 */
const uint32_t &Packet::sourceIP() const {
  return *(uint32_t*)(_packet + sizeof(ether_header) + 12);
}

const uint32_t &Packet::destinationIP() const {
  return *(uint32_t*)(_packet + sizeof(ether_header) + 16);
}

const uint8_t &Packet::icmpType() const {
#ifdef __FreeBSD__
  return ((icmphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> icmp_type;
#endif
#ifdef __linux__
  return ((icmphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> type;
#endif
}

const uint8_t &Packet::icmpCode() const {
#ifdef __FreeBSD__
  return ((icmphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> icmp_code;
#endif
#ifdef __linux__
  return ((icmphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> code;
#endif
}

const uint16_t &Packet::sourcePort() const {
  return ((tcphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> th_sport;
}

const uint16_t &Packet::destinationPort() const {
  return ((tcphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> th_dport;
}

const uint8_t &Packet::tcpFlags() const {
  return ((tcphdr*)(_packet + sizeof(ether_header) + sizeof(ip))) -> th_flags;
}

const uint16_t &Packet::capturedSize() const {
  return _capturedSize;
}

const uint16_t &Packet::payloadSize() const {
  return _payloadSize;
}

const u_char *Packet::payload() const {
  return _payload;
}
