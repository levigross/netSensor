/*
 * Copyright 2007-2011 Boris Kochergin. All rights reserved.
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

#include "configuration.h"

Configuration::Configuration(const std::string fileName) {
  initialize(fileName);
}

Configuration::Configuration() {
  _error = true;
  errorMessage = "Configuration::Configuration(): class not initialized";
}

bool Configuration::initialize(const std::string fileName) {
  std::string line, option, value;
  size_t delimiter, openingQuote, closingQuote;
  std::ifstream file;
  _fileName = fileName;
  file.open(fileName.c_str());
  if (!file) {
    _error = true;
    errorMessage = "Configuration::initialize(): " + fileName + ": " +
                   strerror(errno);
    return false;
  }
  while (getline(file, line)) {
    delimiter = line.find('=');
    option = line.substr(0, delimiter);
    value = line.substr(delimiter + 1);
    openingQuote = value.find('"');
    if (openingQuote != std::string::npos) {
      closingQuote = value.find('"', openingQuote + 1);
      if (closingQuote != std::string::npos) {
        value = value.substr(openingQuote + 1,
                             closingQuote - openingQuote - 1);
      }
    }    
    options[option].push_back(value);
  }
  file.close();
  _error = false;
  errorMessage.clear();
  return true;
}

Configuration::operator bool() const {
  return !_error;
}

const std::string &Configuration::error() const {
  return errorMessage;
}

const std::string &Configuration::fileName() const {
  return _fileName;
}

const std::string &Configuration::getString(const std::string option) const {
  std::map <std::string, std::vector <std::string> >::const_iterator _option = options.find(option);
  if (_option == options.end()) {
    return empty;
  }
  return (_option -> second)[0];
}

const std::vector <std::string> &Configuration::getStrings(const std::string option) const {
  std::map <std::string, std::vector <std::string> >::const_iterator _option = options.find(option);
  return _option -> second;
}

size_t Configuration::getNumber(const std::string option) const {
  std::map <std::string, std::vector <std::string> >::const_iterator _option = options.find(option);
  if (_option == options.end()) {
    return std::numeric_limits <size_t>::max();
  }
  return strtoul((_option -> second)[0].c_str(), NULL, 10);
}
