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

#ifndef MODULE_H
#define MODULE_H

#include <string>
#include <vector>

#include <include/configuration.h>
#include <include/logger.h>
#include <include/packet.h>

typedef int (*processPacketFunction)(const Packet &packet);

class Module {
  public:
    typedef int (*initializeFunction)(const Configuration &conf,
                                      Logger &logger,
                                      std::string &error);
    typedef int (*dependencyInitializeFunction)(const Configuration &conf,
                                                Logger &logger,
                                                const std::vector <void*> &callbacks,
                                                std::string &error);
    typedef int (*flushFunction)();
    typedef int (*finishFunction)();
    Module(const std::string &moduleDirectory,
           const std::string &configurationDirectory, const std::string &name);
    int initialize(Logger &logger);
    processPacketFunction processPacket;
    flushFunction flush;
    finishFunction finish;
    operator bool() const;
    const std::string &error() const;
    const bpf_program &bpfProgram() const;
    const std::string &name() const;
    const std::string &fileName() const;
    const Configuration &conf() const;
    void *handle() const;
    const char *callback() const;
    std::vector <void*> &callbacks();
  private:
    bool _error;
    std::string errorMessage;
    std::string _name;
    std::string _fileName;
    Configuration _conf;
    void *_handle;
    char *_callback;
    std::vector <void*> _callbacks;
    bpf_program _bpfProgram;
    void *_initialize;
};

#endif
