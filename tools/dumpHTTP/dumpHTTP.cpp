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
 */

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>
#include <utility>

#include <netinet/in.h>
#include <regex.h>
#include <strings.h>
#include <unistd.h>

#include <include/address.h>
#include <include/berkeleyDB.h>
#include <include/options.h>
#include <include/timeStamp.h>

#include "message.hpp"

using namespace std;

/* Available command-line options. */
enum { REQUESTS, RESPONSES, CLIENT_ETHERNET_ADDRESS, SERVER_ETHERNET_ADDRESS,
       CLIENT_IP_ADDRESS, SERVER_IP_ADDRESS, CLIENT_PORT, SERVER_PORT,
       REQUEST_METHOD, PATH, QUERY_STRING, FRAGMENT };

bool printRequests = true, printResponses = true, checkRequestType,
     checkPath = false, checkQueryString = false, checkFragment = false;
vector <string> clientMACs, serverMACs;
vector <pair <uint32_t, uint32_t> > clientIPs, serverIPs;
vector <uint16_t> clientPorts, serverPorts;
regex_t regexes[5];

string pad(const string _string, size_t length) {
  if (_string.length() < length * 8) {
    return _string + string(length - _string.length() / 8, '\t');
  }
  return _string;
}

void print(const char *data) {
  static TimeStamp time;
  static const char *clientMAC, *serverMAC;
  static uint32_t *clientIP, *serverIP, ip, length, numMessages, total;
  static uint16_t *clientPort, *serverPort;
  static vector <HTTPMessage> messages;
  static size_t position;
  static bool match;
  clientMAC = data + 1;
  serverMAC = data + 7;
  clientIP = (uint32_t*)(data + 13);
  serverIP = (uint32_t*)(data + 17);
  clientPort = (uint16_t*)(data + 21);
  serverPort = (uint16_t*)(data + 23);
  /* Match client Ethernet addresses. */
  if (clientMACs.size() > 0) {
    match = false;
    for (size_t i = 0; i < clientMACs.size(); ++i) {
      if (memcmp(clientMAC, clientMACs[i].c_str(), ETHER_ADDR_LEN) == 0) {
        match = true;
        break;
      }
    }
    if (match == false) {
      return;
    }
  }
  /* Match server Ethernet addresses. */
  if (serverMACs.size() > 0) {
    match = false;
    for (size_t i = 0; i < serverMACs.size(); ++i) {
      if (memcmp(serverMAC, serverMACs[i].c_str(), ETHER_ADDR_LEN) == 0) {
        match = true;
        break;
      }
    }
    if (match == false) {
      return;
    }
  }
  /* Match client IPv4 addresses. */
  if (clientIPs.size() > 0) {
    ip = ntohl(*clientIP);
    match = false;
    for (size_t i = 0; i < clientIPs.size(); ++i) {
      if (ip >= clientIPs[i].first && ip <= clientIPs[i].second) {
        match = true;
        break;
      }
    }
    if (match == false) {
      return;
    }
  }
  /* Match server IPv4 addresses. */
  if (serverIPs.size() > 0) {
    ip = ntohl(*serverIP);
    match = false;
    for (size_t i = 0; i < serverIPs.size(); ++i) {
      if (ip >= serverIPs[i].first && ip <= serverIPs[i].second) {
        match = true;
        break;
      }
    } 
    if (match == false) {
      return;
    }
  }
  /* Match client port. */
  if (clientPorts.size() > 0 &&
      find(clientPorts.begin(), clientPorts.end(),
           ntohs(*clientPort)) == clientPorts.end()) {
    return;
  }
  /* Match server port. */
  if (serverPorts.size() > 0 &&
      find(serverPorts.begin(), serverPorts.end(),
           ntohs(*serverPort)) == serverPorts.end()) {
    return;
  }
  /* Populate in-memory messages structure with the ones from disk. */
  numMessages = ntohl(*(uint32_t*)(data + 26));
  messages.resize(numMessages);
  position = 30;
  for (size_t i = 0; i < numMessages; ++i) {
    messages[i].type = *(data + position);
    if ((messages[i].type == HTTP_REQUEST && printRequests == true) ||
        (messages[i].type == HTTP_RESPONSE && printResponses == true)) {
      messages[i].print = true;
    }
    ++position;
    messages[i].time.set(ntohl(*(uint32_t*)(data + position)),
                         ntohl(*(uint32_t*)(data + position + 4)));
    position += 8;
    total = ntohl(*(uint32_t*)(data + position));
    position += 4;
    /* Copy request or response text. */
    for (size_t j = 0; j < total; ++j) {
      length = ntohl(*(uint32_t*)(data + position));
      position += 4;
      messages[i].message.push_back(string(data + position, length));
      position += length;
    }
    /* Match request method regular expression. */
    if (messages[i].type == HTTP_REQUEST && checkRequestType == true &&
        regexec(&(regexes[0]), messages[i].message[0].c_str(), 0, NULL,
                0) == REG_NOMATCH) {
      messages[i].print = false;
    }
    /* Match path regular expression. */
    if (messages[i].type == HTTP_REQUEST && checkPath == true &&
        regexec(&(regexes[1]), messages[i].message[1].c_str(), 0, NULL,
                0) == REG_NOMATCH) {
      messages[i].print = false;
    }
    /* Match query string regular expression. */
    if (messages[i].type == HTTP_REQUEST && checkQueryString == true &&
        regexec(&(regexes[2]), messages[i].message[2].c_str(), 0, NULL,
                0) == REG_NOMATCH) {
      messages[i].print = false;
    }
    /* Match fragment regular expression. */
    if (messages[i].type == HTTP_REQUEST && checkFragment == true &&
        regexec(&(regexes[3]), messages[i].message[3].c_str(), 0, NULL,
                0) == REG_NOMATCH) {
      messages[i].print = false;
    }
    /* Copy headers. */
    total = ntohl(*(uint32_t*)(data + position));
    position += 4;
    for (size_t j = 0; j < total; ++j) {
      length = ntohl(*(uint32_t*)(data + position));
      position += 4;
      messages[i].headers.push_back(make_pair(string(data + position, length), ""));
      position += length;
      length = ntohl(*(uint32_t*)(data + position));
      position += 4;
      messages[i].headers.rbegin() -> second = string(data + position, length);
      position += length;
    }
  }
  for (size_t i = 0; i < messages.size(); ++i) {
    if (messages[i].print == true) {
      cout << "Message type:\t\t\t";
      switch (messages[i].type) {
        case HTTP_REQUEST:
          cout << "request";
          break;
        case HTTP_RESPONSE:
          cout << "response";
          break;
      }
      cout << endl;
      cout << "Time:\t\t\t\t" << messages[i].time.string() << endl;
      cout << "Client Ethernet address:\t" << textMAC(clientMAC) << endl;
      cout << "Client IPv4 address:\t\t" << textIP(*clientIP) << endl;
      cout << "Client port:\t\t\t" << ntohs(*clientPort) << endl;
      cout << "Server ethernet address:\t" << textMAC(serverMAC) << endl;
      cout << "Server IPv4 address:\t\t" << textIP(*serverIP) << endl;
      cout << "Server port:\t\t\t" << ntohs(*serverPort) << endl;
      switch (messages[i].type) {
        case HTTP_REQUEST:
          cout << "Request method:\t\t\t" << messages[i].message[0] << endl;
          cout << "Path:\t\t\t\t" << messages[i].message[1] << endl;
          if (messages[i].message[2].length() > 0) {
            cout << "Query string:\t\t\t" << messages[i].message[2] << endl;
          }
          if (messages[i].message[3].length() > 0) {
            cout << "Fragment:\t\t\t" << messages[i].message[3] << endl;
          }
          cout << "Protocol version:\t\tHTTP/" << messages[i].message[4] << endl;
          break;
        case HTTP_RESPONSE:
          if (messages.size() == 0) {
            cerr << "danger!" << endl;
          }
          cout << "Protocol version:\t\tHTTP/" << messages[i].message[0] << endl;
          cout << "Response code:\t\t\t" << messages[i].message[1] << endl;
          break;
      }
      if (messages[i].headers.size() > 0) {
        for (size_t j = 0; j < messages[i].headers.size(); ++j) {
          cout << pad("Header/" + messages[i].headers[j].first + ':', 4)
               << messages[i].headers[j].second << endl;
        }
      }
      cout << endl;
    }
  }
  messages.clear();
}

void usage(const char *program) {
  cerr << "usage: " << program << " [-req|-res] [-cE client Ethernet address] "
       << "[-sE server Ethernet address] [-cI client IPv4 address (CIDR)] "
       << "[-sI server IPv4 address (CIDR)] [-cP client port] "
       << "[-sP server port] [-rM request method] [-p path] [-q query string] "
       << "[-f fragment] file ..." << endl;
}

int main(int argc, char *argv[]) {
  Options options(argc, argv, "req res cE: sE: cI: sI: cP: sP: rM: p: q: f:");
  int option, ret;
  char buffer[1024];
  BerkeleyDB db;
  DBT key, data;
  vector <string> files;
  bool error = false;
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }
  while ((option = options.option()) != -1) {
    if (!options) {
      cerr << argv[0] << ": " << options.error() << endl;
      return 1;
    }
    switch (option) {
      case REQUESTS:
        if (printRequests == false) {
          cerr << argv[0] << ": " << "the \"-req\" and \"-resp\" options are "
               << "mutually-exclusive" << endl;
          return 1;
        }
        printResponses = false;
        break;
      case RESPONSES:
        if (printResponses == false) {
          cerr << argv[0] << ": " << "the \"-req\" and \"-resp\" options are "
               << "mutually-exclusive" << endl;
          return 1;
        }
        printRequests = false;
        break;
      case CLIENT_ETHERNET_ADDRESS:
        clientMACs.push_back(binaryMAC(options.argument()));
        break;
      case SERVER_ETHERNET_ADDRESS:
        serverMACs.push_back(binaryMAC(options.argument()));
        break;
      case CLIENT_IP_ADDRESS:
        clientIPs.push_back(cidrToIPs(options.argument()));
        break;
      case SERVER_IP_ADDRESS:
        serverIPs.push_back(cidrToIPs(options.argument()));
        break;
      case CLIENT_PORT:
        clientPorts.push_back(strtoul(options.argument().c_str(), NULL, 10));
        break;
      case SERVER_PORT:
        serverPorts.push_back(strtoul(options.argument().c_str(), NULL, 10));
        break;
      case REQUEST_METHOD:
        ret = regcomp(&(regexes[0]), options.argument().c_str(), REG_EXTENDED);
        if (ret != 0) {
          regerror(ret, &(regexes[0]), buffer, sizeof(buffer));
          cerr << argv[0] << ": regcomp(): " << buffer << endl;
          return 1;
        }
        checkRequestType = true;
        break;
      case PATH:
        ret = regcomp(&(regexes[1]), options.argument().c_str(), REG_EXTENDED);
        if (ret != 0) {
          regerror(ret, &(regexes[1]), buffer, sizeof(buffer));
          cerr << argv[0] << ": regcomp(): " << buffer << endl;
          return 1;
        }
        checkPath = true;
        break;
      case QUERY_STRING:
        ret = regcomp(&(regexes[2]), options.argument().c_str(), REG_EXTENDED);
        if (ret != 0) {
          regerror(ret, &(regexes[2]), buffer, sizeof(buffer));
          cerr << argv[0] << ": regcomp(): " << buffer << endl;
          return 1;
        }
        checkQueryString = true;
        break;
      case FRAGMENT:
        ret = regcomp(&(regexes[3]), options.argument().c_str(), REG_EXTENDED);
        if (ret != 0) {
          regerror(ret, &(regexes[3]), buffer, sizeof(buffer));
          cerr << argv[0] << ": regcomp(): " << buffer << endl;
          return 1;
        }
        checkFragment = true;
        break; 
   }
  }
  if (options.index() == argc) {
    usage(argv[0]);
    return 1;
  }
  for (int i = options.index(); i < argc; ++i) {
    if (access(argv[i], R_OK) != 0) {
      cerr << argv[0] << ": " << argv[i] << ": " << strerror(errno) << endl;
      error = true;
    }
    else {
      files.push_back(argv[i]);
    }
  }
  if (files.empty() == true) {
    return 1;
  }
  if (error == true) {
    cout << endl;
  }
  bzero(&key, sizeof(key));
  bzero(&data, sizeof(data));
  db.add(files);
  while (db.read(key, data) != BDB_DONE) {
    print((const char*)data.data);
  }
  return 0;
}
