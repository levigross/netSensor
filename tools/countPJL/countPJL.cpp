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
#include <cmath>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <tr1/unordered_map>
#include <vector>

#include <netinet/in.h>
#include <unistd.h>

#include <include/address.h>
#include <include/berkeleyDB.h>
#include <include/timeStamp.h>

using namespace std;
using namespace tr1;

unordered_map <string, uint16_t> computers;
multimap <uint16_t, string> sortedComputers;

void count(const char *data, const uint32_t size) {
  static uint32_t pos;
  static uint16_t length, pages;
  static string value;
  static unordered_map <string, uint16_t>::iterator itr;
  pos = 33;
  length = ntohs(*(uint16_t*)(data + pos));
  value.assign(data + pos + 2, length);
  itr = computers.find(value);
  if (itr == computers.end()) {
    itr = computers.insert(make_pair(value, 0)).first;
  }
  pos = size - 3;
  pages = ntohs(*(uint16_t*)(data + pos));
  itr -> second += pages;
}

void usage(const char *program) {
  cerr << "usage: " << program << " file ..." << endl;
}

string pad(const string _string, size_t length) {
  if (_string.length() < length) {
    return _string + string(length - _string.length(), ' ');
  }
  return _string;
}

int main(int argc, char *argv[]) {
  BerkeleyDB db;
  DBT key, data;
  vector <string> files;
  bool error = false;
  size_t longest = 0;
  int width;
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
    count((const char*)data.data, data.size);
  }
  while (!computers.empty()) {
    sortedComputers.insert(make_pair(computers.begin() -> second,
                                     computers.begin() -> first));
    if (computers.begin() -> first.length() > longest) {
      longest = computers.begin() -> first.length();
    }
    computers.erase(computers.begin());
  }
  width = log10(sortedComputers.rbegin() -> first) + 1;
  for (multimap <uint16_t, string>::const_reverse_iterator itr = sortedComputers.rbegin();
       itr != sortedComputers.rend(); ++itr) {
    cout << pad(itr -> second + ':', longest + 2) << setfill(' ') << setw(width)
         << itr -> first << " page(s)" << endl;
  }
  return 0;
}
