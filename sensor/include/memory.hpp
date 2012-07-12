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

#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <stack>
#include <tr1/memory>

template <class T>
class Memory {
  public:
    Memory();
    Memory(const size_t numBlocks);
    Memory(const size_t numBlocks, const size_t size);
    bool initialize(const size_t numBlocks);
    bool initialize(const size_t numBlocks, const size_t size);
    operator bool() const;
    const std::string &error() const;
    std::tr1::shared_ptr <T> allocate();
    const size_t &size() const;
    const size_t &capacity() const;
    ~Memory();
  private:
    pthread_mutex_t mutex;
    bool _error;
    std::string errorMessage;
    size_t _size;
    size_t _capacity;
    T *beginning;
    T *end;
    std::stack <T*> blocks;
    bool initialized;
    bool free(T*);
    class Deallocator {
      public:
        Deallocator(Memory &memory);
        void operator ()(T *address);
      private:
        Memory <T> &memory;
   };
};

template <class T>
Memory <T>::Memory() {
  _error = true;
  errorMessage = "Memory::Memory(): class not initialized";
  initialized = false;
}

template <class T>
Memory <T>::Memory(const size_t numBlocks) {
  initialized = false;
  initialize(numBlocks, 1);
}

template <class T>
Memory <T>::Memory(const size_t numBlocks, const size_t size) {
  initialized = false;
  initialize(numBlocks, size);
}

template <class T>
bool Memory <T>::initialize(const size_t numBlocks) {
  return initialize(numBlocks, 1);
}

template <class T>
bool Memory <T>::initialize(const size_t numBlocks, const size_t size) {
  int error;
  if (initialized == false) {
    error = pthread_mutex_init(&mutex, NULL);
    if (error != 0) {
      _error = true;
      errorMessage = "Memory::initialize(): pthread_mutex_init(): ";
      errorMessage +=  strerror(error);
      return false;
    }
    _size = 0;
    _capacity = numBlocks;
    beginning = (T*)malloc(sizeof(T) * numBlocks * size);
    if (beginning == NULL) {
      _error = true;
      errorMessage = "Memory::initialize(): malloc(): ";
      errorMessage += strerror(errno);
      return false;
    }
    end = beginning + (numBlocks * size);
    for (size_t blockNumber = 0; blockNumber < numBlocks; ++blockNumber) {
      blocks.push(&(beginning[blockNumber * size]));
    }
    initialized = true;
    _error = false;
    return true;
  }
  return false;
}

template <class T>
Memory <T>::operator bool() const {
  return !_error;
}

template <class T>
const std::string &Memory <T>::error() const {
  return errorMessage;
}

template <class T>
std::tr1::shared_ptr <T> Memory <T>::allocate() {
  T *temp;
  if (initialized == true && blocks.size() > 0) {
    pthread_mutex_lock(&mutex);
    temp = blocks.top();
    blocks.pop();
    ++_size;
    pthread_mutex_unlock(&mutex);
    return std::tr1::shared_ptr <T>(new(temp) T, Deallocator(*this));
  }
  return std::tr1::shared_ptr <T>();
}

template <class T>
bool Memory <T>::free(T *address) {
  if (initialized == true) {
    address -> ~T();
    pthread_mutex_lock(&mutex);
    blocks.push(address);
    --_size;
    pthread_mutex_unlock(&mutex);
    return true;
  }
  return false;
}

template <class T>
const size_t &Memory <T>::size() const {
  return _size;
}

template <class T>
const size_t &Memory <T>::capacity() const {
  return _capacity;
}

template <class T>
Memory <T>::~Memory() {
  if (initialized == true) {
    pthread_mutex_destroy(&mutex);
    free(beginning);
  }
}

template <class T>
Memory <T>::Deallocator::Deallocator(Memory &_memory)
  : memory(_memory)
{}

template <class T>
void Memory <T>::Deallocator::operator ()(T *address) {
  memory.free(address);
}

#endif
