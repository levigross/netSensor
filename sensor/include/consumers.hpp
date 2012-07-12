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

#ifndef CONSUMERS_HPP
#define CONSUMERS_HPP

#include <vector>
#include <tr1/memory>

template <class T>
class Consumers {
  public:
    typedef int (*callback)(const std::tr1::shared_ptr <T> record);
    Consumers();
    Consumers(std::vector <void*> _callbacks);
    void initialize(std::vector <void*> _callbacks);
    int consume(const std::tr1::shared_ptr <T> record) const;
  private:
    std::vector <callback> callbacks;
};

template <class T>
Consumers <T>::Consumers() {}

template <class T>
Consumers <T>::Consumers(std::vector <void*> _callbacks) {
  initialize(_callbacks);
}

template <class T>
void Consumers <T>::initialize(std::vector <void*> _callbacks) {
  for (size_t i = 0; i < _callbacks.size(); ++i) {
    callbacks.push_back((callback)_callbacks[i]);
  }
}

template <class T>
int Consumers <T>::consume(const std::tr1::shared_ptr <T> record) const {
  for (size_t i = 0; i < callbacks.size(); ++i) {
    if (callbacks[i](record) != 0) {
      return 1;
    }
  }
  return 0;
}

#endif
