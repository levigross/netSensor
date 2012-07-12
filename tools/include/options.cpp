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

#include <vector>

#include <include/string.h>

#include "options.h"

Options::Options() {
  initialized = false;
}

Options::Options(int argc, char *const *argv, const char *optionString) {
  _initialize(argc, argv, optionString);
  initialized = true;
}

void Options::initialize(int argc, char *const *argv,
                         const char *optionString) {
  _initialize(argc, argv, optionString);
  initialized = true;
}

/*
 * Returns a non-negative value corresponding to an option, -1 if there are no
 * more options to process, or -2 if there was an error.
 */
int Options::option() {
  /* Checks whether the class has been initialized. */
  if (initialized) {
    /* Checks whether there are any more options to process. */
    if (__argc < _argc && _argv[__argc][0] == '-') {
      /* If so, checks whether the option is valid. */
      optionItr = options.find(_argv[__argc] + 1);
      /* Returns an error if the option is invalid. */
      if (optionItr == options.end()) {
        _error = true;
        errorMessage = "illegal option \"";
        errorMessage += _argv[__argc];
        errorMessage += '"';
        ++__argc;
        return -2;
      }
      /* Otherwise, determines if the option requires an argument. */
      else {
        /* If so, checks whether the option has an argument. */
        if (optionItr -> second.second) {
          /*
           * If so, sets the argument to the word following the option and
           * returns the option's value.
           */
          if (__argc < _argc - 1) {
            _argument = _argv[++__argc];
            ++__argc;
            return (optionItr -> second.first);
          }
          /* Otherwise, returns an error. */
          else {
            _error = true;
            errorMessage = "option \"";
            errorMessage += _argv[__argc];
            errorMessage += "\" requires argument";
            ++__argc;
            return -2;
          }
        }
        /*
         * Returns the option's value if it does not require an argument.
         */
        ++__argc;
        return (optionItr -> second.first);
      }
    }
    return -1;
  }
  /* Returns an error if the class has not been initialized. */
  _error = true;
  errorMessage = "object not initialized";
  return -2;
}

const std::string &Options::argument() const {
  return _argument;
}

const int &Options::index() const {
  return __argc;
}

bool Options::operator!() const {
  return _error;
}
  
const std::string &Options::error() const {
  return errorMessage;
}

void Options::_initialize(int argc, char *const *argv,
                          const char *optionString) {
  /* The supported options are delimited by spaces. */
  std::vector <std::string> optionStrings = explode(optionString, " ");
  _argc = argc;
  __argc = 1;
  _argv = argv;
  _error = false;
  /* Assigns values to options in the order they appear, starting with 0. */
  for (size_t option = 0; option < optionStrings.size(); ++option) {
    /* If an option ends with a colon, it requires an argument. */
    if (optionStrings[option][optionStrings[option].length() - 1] == ':') {
      options.insert(std::make_pair(optionStrings[option].substr(0, optionStrings[option].length() - 1),
                                    std::make_pair(option, true)));
    }
    /* Otherwise, it doesn't. */
    else {
      options.insert(std::make_pair(optionStrings[option],
                                    std::make_pair(option, false)));
    }
  }
}
