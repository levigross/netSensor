/*
 * Copyright 2007-2011 Boris Kochergin. All rights reserved.
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

#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstring>

#include <string>
#include <utility>

#include <sys/socket.h>

#include <arpa/inet.h>
#include <stdint.h>

#define	ETHER_ADDR_LEN	6

/*
 * Given a text representation of an Ethernet address in the form
 * "de:ad:be:ef:ca:fe", returns it in binary form.
 */
std::string binaryMAC(const std::string &textEthernetAddress);

/*
 * Given a binary representation of an Ethernet address, returns it in text
 * form.
 */
std::string textMAC(const char *binaryEthernetAddress);
std::string textMAC(const u_char *binaryEthernetAddress);

/*
 * Given a text representation of an IPv4 address in the form "192.168.0.1",
 * returns it in binary form, in network byte order.
 */
uint32_t binaryIP(const std::string &textIP);

/*
 * Given an address range in CIDR notation, returns a pair containing the first
 * and last addresses in binary form.
 */
std::pair <uint32_t, uint32_t> cidrToIPs(const std::string &cidr);

/*
 * Given a binary representation of an IPv4 address in network byte order,
 * returns it in text form.
 */
std::string textIP(const uint32_t &binaryIP);

#endif
