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

/*
 * A getopt()-like command-line option parser with support for multi-character
 * options.
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <tr1/unordered_map>

class Options {
  public:
    Options();
    Options(int argc, char *const *argv, const char *optionString);
    void initialize(int argc, char *const *argv, const char *optionString);
    int option();
    const std::string &argument() const;
    const int &index() const;
    bool operator!() const;
    const std::string &error() const;
  private:
    bool initialized;
    std::tr1::unordered_map <std::string, std::pair <int, bool> > options;
    std::tr1::unordered_map <std::string, std::pair <int, bool> >::const_iterator optionItr;
    int _argc;
    int __argc;
    char *const *_argv;
    std::string _argument;
    bool _error;
    std::string errorMessage;
    void _initialize(int argc, char *const *argv, const char *optionString);
};

#endif
