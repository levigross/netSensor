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

#include "berkeleyDB.h"

BerkeleyDB::BerkeleyDB() {
  newDatabase = false;
  _finished = true;
}

void BerkeleyDB::add(const std::vector <std::string> &_files) {
  for (size_t file = 0; file < _files.size(); ++file) {
    files.push_back(_files[file]);
  }
  if (files.size() > 0 && _finished == true) {
    openNextDatabase();
    _finished = false;
  }
}

bool BerkeleyDB::closeCurrentDatabase() {
  if (_finished == false) {
    if (cursor -> c_close(cursor) != 0) {
      return false;
    }
    if (db -> close(db, 0) != 0) {
      return false;
    }
    if (_file.length() > 0) {
      _file.clear();
    }
  }
  return true;
}

BerkeleyDB::~BerkeleyDB() {
  closeCurrentDatabase();
}

bool BerkeleyDB::openNextDatabase() {
  if (files.size() == 0) {
    return false;
  }
  if (closeCurrentDatabase() == false) {
    return false;
  }
  if (db_create(&db, NULL, 0) != 0) {
    return false;
  }
  if (db -> open(db, NULL, files.begin() -> c_str(), NULL, DB_RECNO, DB_RDONLY,
                 0) != 0) {
    return false;
  }
  if (db -> cursor(db, NULL, &cursor, 0) != 0) {
    return false;
  }
  _file = files.front();
  files.pop_front();
  return true;
}

/*
 * Returns BDB_OK if a record was read successfully, BDB_NEW_DB if a record
 * was read successfully and a new database has been opened, or BDB_DONE if
 * there are no more records to read.
 */
unsigned int BerkeleyDB::read(DBT &key, DBT &data) {
  while (1) {
    if (cursor -> c_get(cursor, &key, &data, DB_NEXT) == 0) {
      if (newDatabase == false) {
        return BDB_OK;
      }
      newDatabase = false;
      return BDB_NEW_DB;
    }
    if (openNextDatabase() == false) {
      _finished = true;
      return BDB_DONE;
    }
    newDatabase = true;
  }
}

const std::string &BerkeleyDB::file() {
  return _file;
}

bool BerkeleyDB::finished() const {
  return _finished;
}
