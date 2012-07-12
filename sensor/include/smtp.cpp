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

#include <cerrno>
#include <cstring>

#include <include/string.h>

#include "smtp.h"

bool SMTP::initialize(const std::string &server, const size_t auth,
                      const std::string &user, const std::string &password,
                      const std::string &senderName,
                      const std::string &senderAddress,
                      const std::vector <std::string> &recipients) {
  int error = pthread_mutex_init(&mutex, NULL);
  if (error != 0) {
    errorMessage = "SMTP::initialize(): pthread_mutex_init(): ";
    errorMessage += strerror(error);
    return false;
  }
  _server = server;
  _auth = auth;
  _user = user;
  _password = password;
  _senderName = senderName;
  _senderAddress = senderAddress;
  _recipients = recipients;
  return true;
}

int SMTP::lock() {
  return pthread_mutex_lock(&mutex);
}
    
int SMTP::unlock() {
  return pthread_mutex_unlock(&mutex);
}

std::ostringstream &SMTP::subject() {
  return _subject;
}

std::ostringstream &SMTP::message() {
  return __message;
}

bool SMTP::send() {
  smtp_session_t session = NULL;
  smtp_message_t _message;
  smtp_recipient_t __recipients;
  auth_context_t authContext = NULL;

  smtpErrors.clear();
  session = smtp_create_session();
  if (session == NULL) {
    errorMessage = "smtp_create_session(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  _message = smtp_add_message(session);
  if (_message == NULL) {
    errorMessage = "smtp_add_message(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  if (smtp_set_server(session, _server.c_str()) == 0) {
    errorMessage = "smtp_set_server(): " + _server + ": " +
                   smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  if (_auth == 1) {
    auth_client_init();
    authContext = auth_create_context();
    if (authContext == NULL) {
      errorMessage = "auth_create_context(): ";
      errorMessage += strerror(ENOMEM);
      return false;
    }
    auth_set_mechanism_flags(authContext, AUTH_PLUGIN_PLAIN, 0);
    auth_set_interact_cb(authContext, authCallback, this);
    smtp_auth_set_context(session, authContext);
  }
  if (smtp_set_reverse_path(_message, _senderAddress.c_str()) == 0) {
    errorMessage = "smtp_set_reverse_path(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  if (smtp_set_header(_message, "From", _senderName.c_str(),
                      _senderAddress.c_str()) == 0) {
    errorMessage = "smtp_set_header(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  if (smtp_set_header(_message, "To", NULL, NULL) == 0) {
    errorMessage = "smtp_set_header(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  for (size_t i = 0; i < _recipients.size(); ++i) {
    __recipients = smtp_add_recipient(_message, _recipients[i].c_str());
    if (__recipients == NULL) {
      errorMessage = "smtp_add_recipient(): ";
      errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
      return false;
    }
  }
  if (smtp_set_messagecb(_message, &messageCallback, this) == 0) {
    errorMessage = "smtp_set_messagecb(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  if (smtp_set_monitorcb(session, &monitorCallback, this, 0) == 0) {
    errorMessage = "smtp_set_monitorcb(): ";
    errorMessage += smtp_strerror(smtp_errno(), _error, sizeof(_error));
    return false;
  }
  ___message = "Subject: " + _subject.str() + "\r\n\r\n";
  /* Convert bare LFs to CR LFs. */
  for (size_t i = 0; i < __message.str().length(); ++i) {
    if (__message.str()[i] == '\n' && __message.str()[i - 1] != '\r') {
      ___message += '\r';
    }
    ___message += __message.str()[i];
  }
  messageStatus = MESSAGE;
  if (smtp_start_session(session) == 0) {
    errorMessage = "smtp_start_session(): " + _server + ": " + strerror(errno);
    return false;
  }
  if (smtpErrors.size() > 0) {
    errorMessage = "smtp_start_session(): " + _server + ": " +
                   implode(smtpErrors, ", ");
    return false;
  }
  smtp_destroy_session(session);
  auth_destroy_context(authContext);
  auth_client_exit();
  return true;
}

const std::string &SMTP::error() const {
  return errorMessage;
}

const char *messageCallback(void *buffer[] __attribute__((unused)),
                            int *length, void *smtp) {
  SMTP *_smtp = (SMTP*)smtp;
  if (length == NULL) {
    return NULL;
  }
  switch (_smtp -> messageStatus) {
    case MESSAGE:
      _smtp -> messageStatus = DONE;
      *length = _smtp -> ___message.length();
      return _smtp -> ___message.c_str();
      break;
    case DONE:
      return NULL;
  }
  /* Not reached. */
  return NULL;
}

/*
 * Records SMTP errors from the server, which are defined as lines that do not
 * begin with '2' or '3' (the 200 and 300 series of SMTP response codes do not
 * indicate errors).
 */
void monitorCallback(const char *buffer, int length, int writing, void *smtp) {
  SMTP *_smtp = (SMTP*)smtp;
  size_t newline;
  if (writing == 0) {
    _smtp -> _buffer.append(buffer, length);
    while ((newline = _smtp -> _buffer.find("\r\n")) != std::string::npos) {
      if (_smtp -> _buffer[0] != '2' && _smtp -> _buffer[0] != '3') {
        _smtp -> smtpErrors.push_back(_smtp -> _buffer.substr(0, newline));
      }
      _smtp -> _buffer.erase(0, newline + 2);
    }
  }
}

int authCallback(auth_client_request_t request, char *result[], int fields,
                 void *smtp) {
  SMTP *_smtp = (SMTP*)smtp;
  for (int i = 0; i < fields; ++i) {
    if (request[i].flags & AUTH_USER) {
      result[i] = (char*)(_smtp -> _user.c_str());
    }
    else {
      result[i] = (char*)(_smtp -> _password.c_str());
    }
  }
  return 1;
}
