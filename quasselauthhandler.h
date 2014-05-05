/*
  Copyright (C) 2013-2014 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef QUASSELAUTHHANDLER_H
#define QUASSELAUTHHANDLER_H

#include "authhandler.h"

struct NetworkId;
class RemotePeer;
class IrcConnection;

class QuasselAuthHandler : public AuthHandler
{
    Q_OBJECT

public:
    explicit QuasselAuthHandler(IrcConnection* connection);

    RemotePeer* peer() const;
    QString userName() const;
    NetworkId networkId() const;

public slots:
    void authenticate();

signals:
    void protocolUnsupported();
    void clientDenied(const Protocol::ClientDenied& msg);
    void clientRegistered(const Protocol::ClientRegistered& msg);
    void loginFailed(const Protocol::LoginFailed& msg);
    void loginSucceed(const Protocol::LoginSuccess& msg);
    void sessionState(const Protocol::SessionState& msg);

protected:
    void handle(const Protocol::RegisterClient& msg);
    void handle(const Protocol::ClientDenied& msg);
    void handle(const Protocol::ClientRegistered& msg);
    void handle(const Protocol::Login& msg);
    void handle(const Protocol::LoginFailed& msg);
    void handle(const Protocol::LoginSuccess& msg);
    void handle(const Protocol::SessionState& msg);

private slots:
    void initialize();

private:
    struct Private {
        bool probing;
        RemotePeer* peer;
        IrcConnection* connection;
    } d;
};

#endif // QUASSELAUTHHANDLER_H
