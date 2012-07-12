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

#ifndef PACKET_H
#define PACKET_H

#include <pcap.h>

#include <sys/socket.h>

#define __FAVOR_BSD

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <include/timeStamp.h>

class Packet {
  public:
    bool initialize(const pcap_pkthdr &pcapheader,
                    const u_char *pcapPacket);
    const TimeStamp &time() const;
    const uint16_t &capturedSize() const;
    const uint16_t &size() const;
    const u_char *packet() const;
    const u_char *sourceMAC() const;
    const u_char *destinationMAC() const;
    const uint8_t &tos() const;
    const bool &fragmented() const;
    const uint8_t &ttl() const;
    const uint8_t &protocol() const;
    const uint32_t &sourceIP() const;
    const uint32_t &destinationIP() const;
    const uint8_t &icmpType() const;
    const uint8_t &icmpCode() const;
    const uint16_t &sourcePort() const;
    const uint16_t &destinationPort() const;
    const uint8_t &tcpFlags() const;
    const uint16_t &payloadSize() const;
    const u_char *payload() const;
  private:
    TimeStamp _time;
    uint16_t _capturedSize;
    const u_char *_packet;
    bool _fragmented;
    uint16_t _payloadSize;
    const u_char *_payload;
};

#endif
