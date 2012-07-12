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

#include <climits>
#include <cstdlib>

#include <iostream>

#include <netinet/in.h>
#include <unistd.h>

#include <include/address.h>
#include <include/berkeleyDB.h>
#include <include/timeStamp.h>

using namespace std;

void usage(const char *program) {
  cerr << "usage: " << program << " file key ..." << endl;
}

int main(int argc, char *argv[]) {
  DB *db;
  DBT key;
  uint32_t _key; 
  int ret;
  if (argc < 3) {
    usage(argv[0]);
    return 1;
  }
  bzero(&key, sizeof(key));
  ret = db_create(&db, NULL, 0);
  if (ret != 0) {
    cerr << argv[0] << ": db_create(): " << db_strerror(ret) << endl;
    return 1;
  }
  ret = db -> open(db, NULL, argv[1], NULL, DB_RECNO, 0, 0);
  if (ret != 0) {
    cerr << argv[0] << ": open(): " << db_strerror(ret) << endl;
    return 1;
  }
  for (int i = 2; i < argc; ++i) {
    _key = strtoul(argv[i], NULL, 10);
    key.data = &_key;
    key.size = sizeof(_key);
    ret = db -> del(db, NULL, &key, 0);
    if (ret != 0) {
      cerr << argv[0] << ": del(): " << db_strerror(ret) << endl;
      return 1;
    }
  }
  ret = db -> close(db, 0);
  if (ret != 0) {
    cerr << argv[0] << ": close(): " << db_strerror(ret) << endl;
    return 1;
  }
  return 0;
}
