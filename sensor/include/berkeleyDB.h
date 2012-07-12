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

#ifndef BERKELEY_DB_H
#define BERKELEY_DB_H

#include <string>
#include <tr1/unordered_map>

#include <db.h>
#include <pthread.h>

class BerkeleyDB {
  public:
    BerkeleyDB();
    BerkeleyDB(const std::string __directory, const std::string fileName,
               const uint32_t _timeout);
    bool initialize(const std::string, const std::string,
                    const uint32_t _timeout);
    operator bool() const;
    const std::string &error();
    int lock();
    int unlock();
    bool write(const void *data, const size_t size, const uint32_t time);
    bool flush();
    ~BerkeleyDB();
  private:
    std::string _directory;
    std::string fileName;
    uint32_t timeout;
    pthread_mutex_t _lock;
    bool _error;
    std::string errorMessage;
    /*
     * A hash table of _BerkeleyDB classes allows us to find the one that we
     * will write to quickly given the start time of the record to be written.
     */
    class _BerkeleyDB {
      public:
        _BerkeleyDB();
        DB *db;
        DBC *cursor;
        DBT key;
        DBT data;
        uint32_t recordNumber;
    };
    std::tr1::unordered_map <uint32_t, _BerkeleyDB> databases;
    std::string directory(const time_t &time);
    std::string hour(const uint32_t &time);
    std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator find(const uint32_t &time);
    std::tr1::unordered_map <uint32_t, _BerkeleyDB>::iterator create(const uint32_t &time);
    bool checkDirectory(const std::string &directory);
    bool makeDirectory(const std::string &directory, const mode_t mode);
};

#endif
