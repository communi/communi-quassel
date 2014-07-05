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

#ifndef QUASSELPROTOCOL_H
#define QUASSELPROTOCOL_H

#include <ircprotocol.h>
#include "protocol.h"
#include "types.h"

class Message;
class Network;
class IrcChannel;
class BufferInfo;
class SignalProxy;
class QuasselBacklog;
class QuasselAuthHandler;

class QuasselProtocol : public IRC_PREPEND_NAMESPACE(IrcProtocol)
{
    Q_OBJECT

public:
    explicit QuasselProtocol(IRC_PREPEND_NAMESPACE(IrcConnection*) connection);
    virtual ~QuasselProtocol();

    virtual void open();
    virtual void close();
    virtual void read();
    virtual bool write(const QByteArray& data);

signals:
    void sendInput(const BufferInfo& buffer, const QString& message);

protected slots:
    void initNetwork();
    void addChannel(IrcChannel* channel);
    void initChannel(IrcChannel* channel = 0);
    void updateTopic(IrcChannel* channel = 0);
    void updateUsers(IrcChannel* channel);
    void receiveMessage(const Message& message);

    void protocolUnsupported();
    void clientDenied(const Protocol::ClientDenied& msg);
    void clientRegistered(const Protocol::ClientRegistered& msg);
    void loginFailed(const Protocol::LoginFailed& msg);
    void loginSucceed(const Protocol::LoginSuccess& msg);
    void sessionState(const Protocol::SessionState& msg);

private:
    QString prefix() const;
    BufferInfo findBuffer(const QString& name) const;
    void receiveInfo(int code, const QString& info);

    struct Private {
        MsgId lastMsg;
        Network* network;
        SignalProxy* proxy;
        QuasselBacklog* backlog;
        QuasselAuthHandler* handler;
        QHash<QString, BufferInfo> buffers;
    } d;
};

#endif // QUASSELPROTOCOL_H
