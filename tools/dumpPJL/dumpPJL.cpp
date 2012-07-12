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

#include <cerrno>
#include <cstring>

#include <iostream>
#include <string>
#include <vector>

#include <netinet/in.h>
#include <unistd.h>

#include <include/address.h>
#include <include/berkeleyDB.h>
#include <include/timeStamp.h>

using namespace std;

void print(const char *data) {
  static TimeStamp time;
  static const char *clientMAC, *serverMAC;
  static uint32_t *clientIP, *serverIP;
  static uint16_t *clientPort, *serverPort;
  static size_t pos;
  uint16_t length;
  static string value;
  clientMAC = data + 9;
  serverMAC = data + 15;
  clientIP = (uint32_t*)(data + 21);
  serverIP = (uint32_t*)(data + 25);
  clientPort = (uint16_t*)(data + 29);
  serverPort = (uint16_t*)(data + 31);
  time.set(ntohl(*(uint32_t*)(data + 1)), ntohl(*(uint32_t*)(data + 5)));
  cout << "Time:\t\t\t\t" << time.string() << endl
       << "Client Ethernet address:\t" << textMAC(clientMAC) << endl
       << "Server Ethernet address:\t" << textMAC(serverMAC) << endl
       << "Client IP address:\t\t" << textIP(*clientIP) << endl
       << "Server IP address:\t\t" << textIP(*serverIP) << endl
       << "Client port:\t\t\t" << ntohs(*clientPort) << endl
       << "Server port:\t\t\t" << ntohs(*serverPort) << endl;
  pos = 33;
  length = ntohs(*(uint16_t*)(data + pos));
  value.assign(data + pos + 2, length);
  cout << "Computer name:\t\t\t" << value << endl;
  pos += length + 2;
  length = ntohs(*(uint16_t*)(data + pos));
  value.assign(data + pos + 2, length);
  cout << "Username:\t\t\t" << value << endl;
  pos += length + 2;
  length = ntohs(*(uint16_t*)(data + pos));
  value.assign(data + pos + 2, length);
  cout << "Title:\t\t\t\t" << value << endl;
  pos += length + 2;
  cout << "Size:\t\t\t\t" << ntohl(*(uint32_t*)(data + pos)) << endl;
  pos += 4;
  cout << "Pages:\t\t\t\t" << ntohs(*(uint16_t*)(data + pos)) << endl
       << "Out of memory:\t\t\t";
  switch (*(uint8_t*)(data + pos + 2)) {
    case 0:
      cout << "no" << endl << endl;
      break;
    case 1:
      cout << "yes" << endl << endl;
      break;
  }
}

void usage(const char *program) {
  cerr << "usage: " << program << " file ..." << endl;
}

int main(int argc, char *argv[]) {
  BerkeleyDB db;
  DBT key, data;
  vector <string> files;
  bool error = false;
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
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
