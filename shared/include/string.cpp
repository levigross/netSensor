/*
 * Copyright 2008-2011 Boris Kochergin. All rights reserved.
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

#include <cctype>

#include "string.h"

void implode(std::string &_string, const std::vector <std::string> &array,
             const std::string delimiter) {
  for (size_t index = 0; index < array.size(); ++index) {
    _string += array[index];
    if (index < array.size() - 1) {
      _string += delimiter;
    }
  }
}

std::string implode(const std::vector <std::string> &array,
                    const std::string delimiter) {
  std::string _string;
  implode(_string, array, delimiter);
  return _string;
}

void explode(std::vector <std::string> &array, const std::string &_string,
             const std::string delimiter) {
  size_t start = 0, end;
  do {
    end = _string.find(delimiter, start);
    array.push_back(_string.substr(start, end - start));
    start = end + 1;
  } while (end != std::string::npos);
}

std::vector <std::string> explode(const std::string &_string,
                                  const std::string delimiter) {
  std::vector <std::string> array;
  explode(array, _string, delimiter);
  return array;
}

void explode(std::vector <std::string> &array, const std::string &_string) {
  size_t start = 0;
  for (size_t i = 0; i < _string.length(); ++i) {
    if (isspace(_string[i]) != 0) {
      if (isspace(_string[i - 1]) == 0) {
        array.push_back(_string.substr(start, i - start));
      }
      start = i + 1;
    }
  }
  if (isspace(*(_string.rbegin())) == 0) {
    array.push_back(_string.substr(start));
  }
}

std::vector <std::string> explode(const std::string &_string) {
  std::vector <std::string> array;
  explode(array, _string);
  return array;
}
