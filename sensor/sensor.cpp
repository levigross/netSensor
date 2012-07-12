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

#include <cerrno>
#include <csignal>

#include <iostream>
#include <limits>
#include <map>

#ifdef __FreeBSD__
#include <sys/ioctl.h>
#endif
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>

#include <dlfcn.h>
#include <pcap.h>
#include <stdint.h>
#include <unistd.h>

#include <include/configuration.h>
#include <include/module.h>
#include <include/logger.h>
#include <include/packet.h>
#include <include/string.h>

using namespace std;

const string programName = "Net Sensor 0.8.1";
string pidFileName = "/var/run/netSensor.pid";
Logger logger;
vector <Module> modules;
map <string, size_t> moduleIndex;
pthread_t flushThread;
bool capture = true;
size_t flushInterval;

void signalHandler(int signal) {
  switch (signal) {
    case SIGUSR1:
      break;
    case SIGTERM:
      logger.lock();
      logger << logger.time() << "Caught SIGTERM; exiting." << endl;
      logger.unlock();
      capture = false;
      break;
  }
}

void *flush(void*) {
  while (capture == true) {
    sleep(flushInterval);
    for (size_t i = 0; i < modules.size(); ++i) {
      modules[i].flush();
    }
  }
  return NULL;
}

void cleanup(const pid_t &pid, const std::string &pidFileName) {
  kill(pid, SIGUSR1);
  unlink(pidFileName.c_str());
}

int main(int argc, char *argv[]) {
  string configFileName = "sensor.conf", filter;
  Configuration conf;
  ofstream pidFile;
  vector <string> moduleNames, dependencies, filters;
  map <string, size_t>::iterator itr;
  pcap_t *pcapDescriptor;
  char option, errorBuffer[PCAP_ERRBUF_SIZE], cwd[MAXPATHLEN];
  bpf_program bpfProgram;
#ifdef __FreeBSD__
  u_int immediate = 1;
#endif
  sigset_t mask;
  pid_t pid;
  pthread_t flushThread;
  const u_char *pcapPacket;
  pcap_pkthdr pcapHeader;
  Packet packet;
  int error;
  if (signal(SIGTERM, signalHandler) == SIG_ERR ||
      signal(SIGUSR1, signalHandler) == SIG_ERR) {
    cerr << argv[0] << ": signal(): " << strerror(errno) << endl;
    return 1;
  }
  while ((option = getopt(argc, argv, "c:p:")) != -1) {
    switch (option) { 
      case 'c':
        configFileName = optarg;
        break;
      case 'p':
        pidFileName = optarg;
        break;
      default:
        return 1;
    }
  }
  if (!conf.initialize(configFileName)) {
    cerr << argv[0] << ": " << conf.error() << endl;
    return 1;
  }
  if (conf.getString("logging") == "") {
    cerr << argv[0] << ": no logging mode specified" << endl;
    return 1;
  }
  else {
    if (conf.getString("logging") != "on" && 
        conf.getString("logging") != "off") {
      cerr << argv[0] << ": " << conf.fileName() << ": "
           << "unknown logging mode \"" << conf.getString("logging")
           << "\" specified" << endl;
      return 1;
    }
  }
  if (conf.getString("logging") == "on") {
    if (conf.getString("log") == "") {
      cerr << argv[0] << ": " << conf.fileName() << ": no log file specified"
           << endl;
      return 1;
    }
    logger.initialize(conf.getString("log"), "[%F %T] ");
    if (!logger) {
      cerr << argv[0] << ": " << logger.error() << endl;
      return 1;
    }
  }
  if (conf.getString("interface") == "") {
    cerr << argv[0] << ": no interface specified" << endl;
    return 1;
  }
  if (conf.getString("flushInterval") == "") {
    cerr << argv[0] << ": no flush interval specified" << endl;
    return 1;
  }
  flushInterval = conf.getNumber("flushInterval");
  moduleNames = explode(conf.getString("modules"));
  /* Load modules. */
  for (size_t i = 0; i < moduleNames.size(); ++i) {
    modules.push_back(Module("modules/" + moduleNames[i],
                      "modules/" + moduleNames[i], moduleNames[i]));
    if (!modules[i]) {
      cerr << argv[0] << ": " << modules[i].error() << endl;
      return 1;
    }
    moduleIndex.insert(make_pair(moduleNames[i], i));
  }
  /* Check dependencies and set callbacks. */
  for (size_t i = 0; i < modules.size(); ++i) {
    if (modules[i].conf().getString("dependencies") != "") {
      dependencies = explode(modules[i].conf().getString("dependencies"));
      for (size_t j = 0; j < dependencies.size(); ++j) {
         /* Check dependencies on packets from the sensor. */
        if (dependencies[j] == "packet") {
          modules[i].processPacket = (processPacketFunction)dlsym(modules[i].handle(),
                                                                  "processPacket");
          if (modules[i].processPacket == NULL) {
            cerr << argv[0] << ": " << modules[i].fileName() << ": no "
                 << "\"processPacket\" callback defined" << endl;
            return 1;
          }
        }
        /* Check dependencies on other modules. */
        else {
          itr = moduleIndex.find(dependencies[j]);
          if (modules[i].name() == dependencies[j]) {
            cerr << argv[0] << ": " << modules[i].fileName()
                 << ": module cannot depend on itself" << endl;
            return 1;
          }
          if (itr == moduleIndex.end()) {
            cerr << argv[0] << ": " << modules[i].fileName()
                 << ": dependency \"" << dependencies[j] << "\" not found"
                 << endl;
            return 1;
          }
          if (modules[itr -> second].callback() == NULL) {
            cerr << argv[0] << ": " <<  modules[i].fileName()
                 << ": dependency \"" << dependencies[j]
                 << "\" has no callback defined" << endl;
            return 1;
          }
          modules[itr -> second].callbacks().push_back(dlsym(modules[i].handle(),
                                                       modules[itr -> second].callback()));
          if (*(modules[itr -> second].callbacks().rbegin()) == NULL) {
            cerr << argv[0] << ": " << modules[i].fileName() << ": no \""
                 << modules[itr -> second].callback() << "\" callback defined"
                 << endl;
            return 1;
          }
        }
      }
    }
  }
  pcapDescriptor = pcap_open_live(conf.getString("interface").c_str(),
                                  std::numeric_limits <uint16_t>::max(), 1, 0,
                                  errorBuffer);
  if (pcapDescriptor == NULL) {
    cerr << argv[0] << ": " << errorBuffer << endl;
    return 1;
  }
  /*
   * The pcap filter string we will use to capture traffic is formed by
   * combining the filter strings of all modules that will consume packets.
   */
  for (size_t i = 0; i < modules.size(); ++i) {
    if (modules[i].processPacket != NULL &&
        modules[i].conf().getString("filter") != "") {
      filters.push_back(modules[i].conf().getString("filter"));
    }
  }
  for (size_t i = 0; i < filters.size(); ++i) {
    filter += '(' + filters[i] + ')';
    if (i < filters.size() - 1) {
      filter += " or ";
    }
  }
  /*
   * Older versions of libpcap expect the third argument of pcap_compile()
   * to be of type "char*".
   */
 if (pcap_compile(pcapDescriptor, &bpfProgram, (char*)filter.c_str(), 1,
                  0) == -1) {
    cerr << argv[0] << ": pcap_compile(): " << pcap_geterr(pcapDescriptor)
         << endl;
    return 1;
  }
  if (pcap_setfilter(pcapDescriptor, &bpfProgram) == -1) {
    cerr << argv[0] << ": pcap_setfilter(): " << pcap_geterr(pcapDescriptor)
         << endl;
    return 1;
  }
/* If running on FreeBSD, put the BPF device into immediate mode. */
#ifdef __FreeBSD__
    if (ioctl(pcap_fileno(pcapDescriptor), BIOCIMMEDIATE, &immediate) == -1) {
      cerr << argv[0] << ": ioctl(): " << strerror(errno) << endl;
      return 1;
    }
#endif
  if (sigfillset(&mask) == -1) {
    cerr << argv[0] << ": sigfillset(): " << strerror(errno) << endl;
    return 1;
  }
  if (sigdelset(&mask, SIGUSR1) == -1) {
    cerr << argv[0] << ": sigdelset(): " << strerror(errno) << endl;
    return 1;
  }
  pid = fork();
  if (pid < 0) {
    cerr << ": fork(): " << strerror(errno) << "; exiting" << endl;
    return 1;
  }
  /* If this is the parent process, wait for SIGUSR1 from the child. */
  if (pid != 0) {
    sigsuspend(&mask);
    return 0;
  }
  /* Initialize modules. */
  for (size_t i = 0; i < modules.size(); ++i) {
    if (modules[i].initialize(logger) != 0) {
      cerr << argv[0] << ": " << modules[i].fileName() << ": "
           << modules[i].error() << endl;
      return 1;
    }
  }
  /* Start flush() thread. */
  error = pthread_create(&flushThread, NULL, &flush, NULL);
  if (error != 0) {
    cerr << argv[0] << ": pthread_create(): " << strerror(error) << endl;
    return 1;
  }
  /*
   * If we are not given an absolute path to the PID file, form an absolute
   * path by prepending the current working directory to it, so that we can
   * unlink it later.
   */
  if (pidFileName[0] != '/') {
    if (getcwd(cwd, MAXPATHLEN) == NULL) {
      cerr << argv[0] << ": getcwd(): " << strerror(errno) << endl;
      return 1;
    }
    pidFileName = '/' + pidFileName;
    pidFileName = cwd + pidFileName;
  }
  /* Write daemonized child's PID to the PID file. */
  pidFile.open(pidFileName.c_str());
  if (!pidFile) {
    cerr << argv[0] << ": open(): " << pidFileName << ": " << strerror(errno)
         << "; exiting" << endl;
    kill(getppid(), SIGUSR1);
    return 1;
  }
  pidFile << getpid() << endl;
  pidFile.close();
  if (chdir("/") != 0) {
    cerr << argv[0] << ": chdir(): /: " << strerror(errno) << endl;
    cleanup(getppid(), pidFileName);
    return 1;
  }
  /* Close unneeded file descriptors. */
  if (close(STDIN_FILENO) != 0) {
    cerr << argv[0] << ": close(): STDIN_FILENO: " << strerror(errno) << endl;
    cleanup(getppid(), pidFileName);
    return 1;
  }
  if (close(STDOUT_FILENO) != 0) {
    cerr << argv[0] << ": close(): STDOUT_FILENO: " << strerror(errno)
         << endl;
    cleanup(getppid(), pidFileName);
    return 1;
  }
  if (kill(getppid(), SIGUSR1) == -1) {
    cerr << argv[0] << ": kill(): " << strerror(errno) << endl;
    cleanup(getppid(), pidFileName);
    return 1;
  }
  if (close(STDERR_FILENO) != 0) {
    cerr << argv[0] << ": close(): STDERR_FILENO: " << strerror(errno)
         << endl;
    unlink(pidFileName.c_str());
    return 1;
  }
  logger.lock();
  logger << logger.time() << programName << " starting." << endl;
  logger.unlock();
  /*
   * For each packet received, check which modules are interested in it and
   * call each interested module's processPacket() function with it.
   */
  while (capture == true) {
    if ((pcapPacket = pcap_next(pcapDescriptor, &pcapHeader)) != NULL) {
      if (packet.initialize(pcapHeader, pcapPacket) == true) {
        for (size_t i = 0; i < modules.size(); ++i) {
          if (modules[i].processPacket != NULL &&
              bpf_filter(modules[i].bpfProgram().bf_insns, (u_char*)pcapPacket,
              pcapHeader.len, pcapHeader.caplen) != 0) {
            modules[i].processPacket(packet);
          }
        }
      }
    }
  }
  /* Allow the flush() thread to exit gracefully. */
  error = pthread_join(flushThread, NULL);
  if (error != 0) {
    logger.lock();
    logger << logger.time() << "pthread_join(): " << strerror(error)
           << "; exiting." << endl;
    logger.unlock();
    unlink(pidFileName.c_str());
    return 1;
  }
  /* Call each module's finish() function. */
  for (size_t i = 0; i < modules.size(); ++i) {
    modules[i].finish();
  }
  unlink(pidFileName.c_str());
  return 0;
}
