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

#include <cstring>

#include <limits>

#include <dlfcn.h>

#include "module.h"

Module::Module(const std::string &moduleDirectory,
               const std::string &configurationDirectory,
               const std::string &name) {
  char *__callback;
  std::string moduleErrorMessage;
  pcap_t *pcapDescriptor;
  _name = name;
  _fileName = moduleDirectory + '/' + name + ".so";
  if (!_conf.initialize(configurationDirectory + '/' + name + ".conf")) {
    _error = true;
    errorMessage = _conf.error();
    return;
  }
  _handle = dlopen(_fileName.c_str(), RTLD_NOW);
  if (_handle == NULL) {
    _error = true;
    errorMessage = "dlopen(): ";
    errorMessage += dlerror();
    return;
  }
  _initialize = dlsym(_handle, "initialize");
  flush = (flushFunction)dlsym(_handle, "flush");
  finish = (finishFunction)dlsym(_handle, "finish");
  processPacket = NULL;
  _callback = NULL;
  if (_initialize == NULL || flush == NULL || finish == NULL) {
    _error = true;
    errorMessage = _fileName + ": dlsym(): " + dlerror();
    return;
  }
  __callback = (char*)dlsym(_handle, "callback");
  if (__callback != NULL) {
    _callback = *(char**)__callback;
  }
  pcapDescriptor = pcap_open_dead(DLT_EN10MB,
                                  std::numeric_limits <uint16_t>::max());
  if (pcapDescriptor == NULL) {
    _error = true;
    errorMessage = pcap_geterr(pcapDescriptor);
    return;
  }
  /*
   * Older versions of libpcap expect the third argument of pcap_compile()
   * to be of type "char*".
   */
  if (pcap_compile(pcapDescriptor, &_bpfProgram,
                   (char*)_conf.getString("filter").c_str(), 1, 0) == -1) {
    _error = true;
    errorMessage = _fileName + ": pcap_compile(): " +
                   pcap_geterr(pcapDescriptor);
    return;
  }
  pcap_close(pcapDescriptor);
  _error = false;
}

int Module::initialize(Logger &logger) {
  initializeFunction _initializeFunction = (initializeFunction)_initialize;
  dependencyInitializeFunction _dependencyInitializeFunction = (dependencyInitializeFunction)_initialize;
  std::string moduleErrorMessage;
  int ret;
  if (_callbacks.size() == 0) {
    ret = _initializeFunction(_conf, logger, moduleErrorMessage);
  }
  else {
    ret = _dependencyInitializeFunction(_conf,logger, _callbacks,
                                        moduleErrorMessage);
  }
  if (ret != 0) {
    _error = true;
    errorMessage = "initialize(): " + moduleErrorMessage;
    return 1;
  }
  return 0;
}

Module::operator bool() const {
  return !_error;
}

const std::string &Module::error() const {
  return errorMessage;
}

const bpf_program &Module::bpfProgram() const {
  return _bpfProgram;
}

const std::string &Module::name() const {
  return _name;
}

const std::string &Module::fileName() const {
  return _fileName;
}

const Configuration &Module::conf() const {
  return _conf;
}

void *Module::handle() const {
  return _handle;
}

const char *Module::callback() const {
  return _callback;
}

std::vector <void*> &Module::callbacks() {
  return _callbacks;
}
