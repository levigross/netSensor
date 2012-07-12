/*
 * Copyright 2009-2011 Boris Kochergin. All rights reserved.
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

#ifndef SMTP_H
#define SMTP_H

#include <sstream>
#include <string>
#include <vector>

#include <pthread.h>

#include <auth-client.h>
#include <libesmtp.h>

enum MessageStatusType { MESSAGE, DONE };

const char *messageCallback(void *buffer[], int *length, void *smtp);
void monitorCallback(const char *buffer, int length, int writing, void *smtp);
int authCallback(auth_client_request_t request, char *result[], int fields,
                 void *smtp);

class SMTP {
  public:
    bool initialize(const std::string &server, const size_t auth,
                    const std::string &user, const std::string &password,
                    const std::string &senderName,
                    const std::string &senderAddress,
                    const std::vector <std::string> &recipients);
    int lock();
    int unlock();
    std::ostringstream &subject();
    std::ostringstream &message();
    bool send();
    const std::string &error() const;
    friend const char *messageCallback(void *buffer[], int *len, void *arg);   
    friend void monitorCallback(const char *buffer, int length, int writing,
                                void *smtp);
    friend int authCallback(auth_client_request_t request, char *result[],
                            int fields, void *smtp);
  private:
    pthread_mutex_t mutex;
    char _error[1024];
    std::string _server;
    size_t _auth;
    std::string _user;
    std::string _password;
    std::vector <std::string> _recipients;
    std::string _senderName;
    std::string _senderAddress;
    std::string errorMessage;
    std::vector <std::string> smtpErrors;
    std::ostringstream _subject;
    std::string __subject;
    std::ostringstream __message;
    std::string ___message;
    std::string _buffer;
    MessageStatusType messageStatus;
};

#endif
