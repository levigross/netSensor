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

#ifndef WRITER_HPP
#define WRITER_HPP

#include <cstring>

#include <queue>
#include <string>
#include <tr1/memory>

#include <include/berkeleyDB.h>

template <class Flow>
class Writer {
  public:
    Writer();
    template <class Function>
    Writer(const std::string, const std::string, const uint32_t, Function);
    template <class Function>
    bool initialize(const std::string, const std::string, const uint32_t,
                    Function);
    operator bool() const;
    const std::string &error() const;
    template <class _Flow>
    friend void *writeFlows(void*);
    void write(std::tr1::shared_ptr <Flow>, const uint32_t&);
    void flush();
    void finish();
    ~Writer();
    /* Record class. */
    class Record {
      public:
        Record();
        std::string &append(const char *data, size_t length);
        Record &operator +=(const char &data);
        Record &operator +=(const uint8_t &data);
        Record &operator +=(const uint16_t &data);
        Record &operator +=(const uint32_t &data);
        Record &operator +=(const uint64_t &data);
        Record &operator +=(const std::string &data);
        const char *data() const;
        size_t size() const;
        void clear();
      private:
        std::string record;
    };
  private:
    bool _error;
    std::string errorMessage;
    bool initialized;
    BerkeleyDB db;
    std::queue <std::pair <std::tr1::shared_ptr <Flow>, uint32_t> > writeQueue;
    pthread_t writerThread;
    bool status;
    bool _flush;
    pthread_mutex_t writeLock;
    pthread_mutex_t statusLock;
    pthread_mutex_t queueLock;
    pthread_mutex_t flushLock;
    pthread_cond_t writeCondition;
    bool _write;
    void *_function;
    void _writeFlows();
};

template <class Flow>
Writer <Flow>::Record::Record () {}

template <class Flow>
std::string &Writer <Flow>::Record::append(const char *string, size_t length) {
  record.append(string, length);
  return record;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const char &data) {
  record += data;
  return *this;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const uint8_t &data) {
  record += data;
  return *this;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const uint16_t &data) {
  record.append((const char*)&data, sizeof(data));
  return *this;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const uint32_t &data) {
  record.append((const char*)&data, sizeof(data));
  return *this;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const uint64_t &data) {
  record.append((const char*)&data, sizeof(data));
  return *this;
}

template <class Flow>
typename Writer <Flow>::Record &Writer <Flow>::Record::operator +=(const std::string &data) {
  record += data;
  return *this;
}

template <class Flow>
const char *Writer <Flow>::Record::data() const {
  return record.c_str();
}

template <class Flow>
size_t Writer <Flow>::Record::size() const {
  return record.size();
}

template <class Flow>
void Writer <Flow>::Record::clear() {
  record.clear();
}

template <class Flow>
void *writeFlows(void *writer) {
  ((Writer <Flow>*)writer) -> _writeFlows();
  return NULL;
}

template <class Flow>
void Writer <Flow>::_writeFlows() {
  typedef void (*recordFunction)(Record &record, const Flow &flow);
  Record record;
  while (_write && pthread_mutex_trylock(&writeLock) == 0) {
    pthread_cond_wait(&writeCondition, &writeLock);
    pthread_mutex_lock(&statusLock);
    status = true;
    pthread_mutex_unlock(&statusLock);
    while (!writeQueue.empty()) {
      ((recordFunction)_function)(record, *(writeQueue.front().first));
      db.write(record.data(), record.size(), writeQueue.front().second);
      record.clear();
      pthread_mutex_lock(&queueLock);
      writeQueue.pop();
      pthread_mutex_unlock(&queueLock);
    }
    pthread_mutex_lock(&statusLock);
    status = false;
    pthread_mutex_unlock(&statusLock);
    pthread_mutex_lock(&flushLock);
    if (_flush) {
      _flush = false;
      pthread_mutex_unlock(&flushLock);
      db.flush();
    }
    else {
      pthread_mutex_unlock(&flushLock);
    }
    pthread_mutex_unlock(&writeLock);
  }
}

template <class Flow>
Writer <Flow>::Writer() {
  initialized = false;
  _error = true;
  errorMessage = "Writer::Writer(): class not initialized";
}

template <class Flow>
template <class Function>
Writer <Flow>::Writer(const std::string directory, const std::string fileName,
                      const uint32_t timeout, Function function) {
  initialized = false;
  initialize(directory, fileName, timeout, function);
}

template <class Flow>
template <class Function>
bool Writer <Flow>::initialize(const std::string directory,
                               const std::string fileName,
                               const uint32_t timeout, Function function) {
  int error;
  if (initialized == false) {
    if ((error = pthread_mutex_init(&writeLock, NULL)) != 0 ||
        (error = pthread_mutex_init(&statusLock, NULL)) != 0 ||
        (error = pthread_mutex_init(&queueLock, NULL)) != 0 ||
        (error = pthread_mutex_init(&flushLock, NULL)) != 0) {
      _error = true;
      errorMessage = "Writer::initialize(): pthread_mutex_init(): ";
      errorMessage += strerror(error);
      return false;
    }
    error = pthread_cond_init(&writeCondition, NULL);
    if (error != 0) {
      _error = true;
      errorMessage = "Writer::initialize(): pthread_cond_init(): ";
      errorMessage += strerror(error);
      return false;
    }
    _write = true;
    status = false;
    _flush = false;
    _function = (void*)function;
    if (db.initialize(directory, fileName, timeout) == false) {
      _error = true;
      errorMessage = "Writer::initialize(): " + db.error();
      return false;
    }
    error = pthread_create(&writerThread, NULL, &writeFlows <Flow>, this);
    if (error != 0) {
      _error = true;
      errorMessage = "Writer::initialize(): pthread_create(): ";
      errorMessage += strerror(error);
      return false;
    }
    initialized = true;
    return true;
  }
  return false;
}

template <class Flow>
Writer <Flow>::operator bool() const {
  return !_error;
}

template <class Flow>
const std::string &Writer <Flow>::error() const {
  return errorMessage;
}

template <class Flow>
void Writer <Flow>::write(std::tr1::shared_ptr <Flow> flow,
                          const uint32_t &startTime) {
  pthread_mutex_lock(&queueLock);
  writeQueue.push(std::make_pair(flow, startTime));
  pthread_mutex_unlock(&queueLock);
  pthread_mutex_lock(&statusLock);
  if (!status) {
    pthread_cond_broadcast(&writeCondition);
  }
  pthread_mutex_unlock(&statusLock);
}

template <class Flow>
void Writer <Flow>::flush() {
  pthread_mutex_lock(&flushLock);
  _flush = true;
  pthread_mutex_unlock(&flushLock);
}

template <class Flow>
void Writer <Flow>::finish() {
  _write = false;
  pthread_mutex_lock(&statusLock);
  if (!status) {
    pthread_cond_broadcast(&writeCondition);
  }
  pthread_mutex_unlock(&statusLock);
  pthread_join(writerThread, NULL);
}

template <class Flow>
Writer <Flow>::~Writer() {
  if (initialized) {
    pthread_mutex_destroy(&writeLock);
    pthread_mutex_destroy(&statusLock);
    pthread_mutex_destroy(&queueLock);
    pthread_mutex_destroy(&flushLock);
    pthread_cond_destroy(&writeCondition);
  }
}

#endif
