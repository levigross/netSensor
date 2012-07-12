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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cerrno>
#include <cstring>

#include <iomanip>
#include <sstream>
#include <vector>

#include <sys/stat.h>

#include <strings.h>
#include <unistd.h>

#include "berkeleyDB.h"

BerkeleyDB::BerkeleyDB() {
  _error = true;
  errorMessage = "BerkeleyDB::BerkeleyDB(): class not initialized";
}

BerkeleyDB::BerkeleyDB(const std::string __directory,
                       const std::string _fileName,
                       const uint32_t _timeout) {
  initialize(__directory, _fileName, _timeout);
}

bool BerkeleyDB::initialize(const std::string __directory,
                            const std::string _fileName,
                            const uint32_t _timeout) {
  int ret = pthread_mutex_init(&_lock, NULL);
  if (ret != 0) {
    _error = true;
    errorMessage = "BerkeleyDB::initialize(): pthread_mutex_init(): ";
    errorMessage += strerror(ret);
    return false;
  }
  _directory = __directory;
  if (*(_directory.rbegin()) != '/') {
    _directory += '/';
  }
  if (checkDirectory(_directory) == false) {
    _error = true;
    errorMessage = "BerkeleyDB::initialize(): checkDirectory(): " +
                   _directory + ": ";
    errorMessage += strerror(errno);
    return false;
  }
  fileName = _fileName;
  timeout = _timeout;
  return true;
}

BerkeleyDB::operator bool() const {
  return !_error;
}

const std::string &BerkeleyDB::error() {
  return errorMessage;
}

int BerkeleyDB::lock() {
  return pthread_mutex_lock(&_lock);
}

int BerkeleyDB::unlock() {
  return pthread_mutex_unlock(&_lock);
}

/*
 * Given a time, returns the hour of the day in the format of "00" through
 * "23."
 */
std::string BerkeleyDB::hour(const uint32_t &_time) {
  time_t time = _time;
  tm _tm;
  localtime_r(&time, &_tm);
  std::ostringstream hour;
  hour << std::setfill('0') << std::setw(2) << _tm.tm_hour;
  return hour.str();
}

/* Given a time, returns an absolute data directory. */
std::string BerkeleyDB::directory(const time_t &time) {
  tm _tm;
  localtime_r(&time, &_tm);
  std::ostringstream dataDirectory;
  dataDirectory << _directory << '/' << _tm.tm_year + 1900 << '/'
                << std::setfill('0') << std::setw(2) << _tm.tm_mon + 1 << '/'
                << std::setfill('0') << std::setw(2) << _tm.tm_mday << '/';
  return dataDirectory.str();
}

/*
 * Checks whether an absolute directory (or as much of it as exists) is writable.
 */
bool BerkeleyDB::checkDirectory(const std::string &directory) {
  size_t position, lastPosition = directory.length() - 1;;
  while ((position = directory.rfind('/', lastPosition)) != std::string::npos) {
    lastPosition = position - 1;
    switch (access(directory.substr(0, position).c_str(), W_OK)) {
      case 0:
        return true;
      case -1:
        switch (errno) {
          case ENOENT:
            continue;
          default:
            return false;
        }
    }
  }
  /* Not reached. */
  return false;
}

/* Recursively creates an absolute directory. */
bool BerkeleyDB::makeDirectory(const std::string &directory,
                               const mode_t mode) {
  size_t lastPosition = 0, position;
  std::string currentDirectory;
  do {
    position = directory.find('/', lastPosition);
    currentDirectory += directory.substr(lastPosition,
                                         position - lastPosition) + '/';
    lastPosition = position + 1;
    if (currentDirectory != "/" &&
        mkdir(currentDirectory.c_str(), mode) == -1 && errno != EEXIST) {
      return false;
    }
  } while (position != std::string::npos);
  return true;
}

/*
 * Given a time, opens the appropriate Berkeley DB database--creating it if it
 * doesn't exist, along with any directories in its path--and sets its record
 * number to the appropriate value (1 for new databases, last record number + 1
 * for existing databases).
 */
std::tr1::unordered_map <uint32_t, BerkeleyDB::_BerkeleyDB>::iterator BerkeleyDB::create(const uint32_t &time) {
  std::string dataFileName = directory(time);
  std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator db;
  if (!makeDirectory(dataFileName, 0755)) {
    return databases.end();
  }
  db = databases.insert(std::make_pair(time, _BerkeleyDB())).first;
  dataFileName += fileName + '_' + hour(time);
  if (db_create(&(db -> second.db), NULL, 0) != 0) {
    return databases.end();
  }
  if (db -> second.db -> open(db -> second.db, NULL, dataFileName.c_str(),
                              NULL, DB_RECNO, DB_CREATE,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
    return databases.end();
  }
  if (db -> second.db -> cursor(db -> second.db, NULL, &(db -> second.cursor),
                                0) != 0) {
    return databases.end();
  }
  if (db -> second.cursor -> c_get(db -> second.cursor, &(db -> second.key),
                                   &(db -> second.data),
                                   DB_LAST) == DB_NOTFOUND) {
    db -> second.recordNumber = 1;
  }
  else {
    db -> second.recordNumber = *(uint32_t*)(db -> second.key.data) + 1;
  }
  db -> second.cursor -> c_close(db -> second.cursor);
  return db;
}

BerkeleyDB::_BerkeleyDB::_BerkeleyDB() {
  bzero(&key, sizeof(key));
  bzero(&data, sizeof(data));
}

/*
 * Given a time, returns an iterator to the appropriate database, creating it if
 * it doesn't exist.
 */
std::tr1::unordered_map <uint32_t, BerkeleyDB::_BerkeleyDB>::iterator BerkeleyDB::find(const uint32_t &time) {
  std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator db = databases.find(time);
  if (db != databases.end()) {
    return db;
  }
  return create(time);
}

/*
 * Given a record, its size, and its start time, writes it to the appropriate
 * database, creating it if it doesn't exist.
 */
bool BerkeleyDB::write(const void* data, const size_t dataSize, const uint32_t time) {
  std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator db = find(time - (time % 3600));
  if (db != databases.end()) {
    db -> second.key.size = sizeof(db -> second.recordNumber);
    db -> second.key.data = &(db -> second.recordNumber);
    db -> second.data.size = dataSize;
    db -> second.data.data = (void*)data;
    if (db -> second.db -> put(db -> second.db, NULL, &(db -> second.key),
                               &(db -> second.data), 0) == 0) {
      ++(db -> second.recordNumber);
      return true;
    }
  }
  return false;
}

/*
 * Writes any records in Berkeley DB's cache to disk and closes any databases
 * that have been open for at least as long as the timeout.
 */
bool BerkeleyDB::flush() {
  uint32_t _time = time(NULL);
  std::vector <std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator> erase;
  for (std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator db = databases.begin();
       db != databases.end(); ++db) {
    if (db -> second.db -> sync(db -> second.db, 0) != 0) {
      return false;
    }
    if (_time >= db -> first + 3600 + timeout) {
      if (db -> second.db -> close(db -> second.db, 0) != 0) {
        return false;
      }
      else {
        erase.push_back(db);
      }
    }
  }
  for (size_t index = 0; index < erase.size(); ++index) {
    databases.erase(erase[index]);
  }
  return true;
}

BerkeleyDB::~BerkeleyDB() {
  for (std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator db = databases.begin();
       db != databases.end(); ++db) {
    db -> second.db -> close(db -> second.db, 0);
  }
}
