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

#include "quasselauthhandler.h"
#include "datastreampeer.h"
#include "legacypeer.h"
#include "compressor.h"
#include "types.h"
#include <IrcConnection>
#include <QtEndian>

IRC_USE_NAMESPACE

QuasselAuthHandler::QuasselAuthHandler(IrcConnection* connection) : AuthHandler(connection)
{
    d.peer = 0;
    d.legacy = false;
    d.probing = false;
    d.connection = connection;

    QTcpSocket* socket = qobject_cast<QTcpSocket*>(connection->socket());
    if (socket) {
        connect(socket, SIGNAL(readyRead()), SLOT(onReadyRead()));
        connect(socket, SIGNAL(connected()), SLOT(onSocketConnected()));
        setSocket(socket);
    }
}

RemotePeer* QuasselAuthHandler::peer() const
{
    return d.peer;
}

QString QuasselAuthHandler::userName() const
{
    return d.connection->userName().section('/', 0, 0);
}

NetworkId QuasselAuthHandler::networkId() const
{
    return NetworkId(d.connection->userName().section('/', -1).toInt());
}

bool QuasselAuthHandler::isProbing() const
{
    return d.probing;
}

void QuasselAuthHandler::authenticate()
{
    onSocketConnected();
}

void QuasselAuthHandler::handle(const Protocol::RegisterClient& msg)
{
    Q_UNUSED(msg);
}

void QuasselAuthHandler::handle(const Protocol::ClientDenied& msg)
{
    emit clientDenied(msg);
}

void QuasselAuthHandler::handle(const Protocol::ClientRegistered& msg)
{
    emit clientRegistered(msg);
}

void QuasselAuthHandler::handle(const Protocol::Login& msg)
{
    Q_UNUSED(msg);
}

void QuasselAuthHandler::handle(const Protocol::LoginFailed& msg)
{
    emit loginFailed(msg);
}

void QuasselAuthHandler::handle(const Protocol::LoginSuccess& msg)
{
    emit loginSucceed(msg);
}

void QuasselAuthHandler::handle(const Protocol::SessionState& msg)
{
    emit sessionState(msg);
}

void QuasselAuthHandler::onReadyRead()
{
    // make sure to not read more data than needed
    if (socket()->bytesAvailable() < 4 || !d.probing)
        return;

    d.probing = false;
    disconnect(socket(), SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    quint32 reply;
    socket()->read((char*)&reply, 4);
    reply = qFromBigEndian<quint32>(reply);

    Protocol::Type type = static_cast<Protocol::Type>(reply & 0xff);
    if (type == Protocol::DataStreamProtocol || type == Protocol::LegacyProtocol) {
        quint8 connectionFeatures = static_cast<quint8>(reply >> 24);
        Compressor::CompressionLevel compression = Compressor::NoCompression;
        if (connectionFeatures & Protocol::Compression)
            compression = Compressor::BestCompression;

        if (type == Protocol::DataStreamProtocol) {
            quint16 protoFeatures = static_cast<quint16>(reply >> 8 & 0xffff);
            setPeer(new DataStreamPeer(this, socket(), protoFeatures, compression, d.connection));
        } else {
            setPeer(new LegacyPeer(this, socket(), compression, d.connection));
        }
    } else {
        emit protocolUnsupported();
    }
}

void QuasselAuthHandler::onSocketConnected()
{
    if (d.peer)
        return;

    socket()->setSocketOption(QAbstractSocket::KeepAliveOption, true);

    if (!d.legacy) {
        // First connection attempt, try probing for a capable core
        d.probing = true;

        QDataStream stream(socket()); // stream handles the endianness for us

        quint32 magic = Protocol::magic;
        if (d.connection->isSecure())
            magic |= Protocol::Encryption;
        magic |= Protocol::Compression;
        stream << magic;

        stream << quint32(Protocol::DataStreamProtocol | (DataStreamPeer::supportedFeatures() << 8));
        stream << quint32(Protocol::LegacyProtocol | 0x80000000); // end of list

        socket()->flush(); // make sure the probing data is sent immediately
        return;
    }

    setPeer(new LegacyPeer(this, socket(), Compressor::NoCompression, d.connection));
}

void QuasselAuthHandler::onSocketDisconnected()
{
    if (d.probing && d.legacy) {
        // Remote host has closed the connection while probing
        d.probing = false;
        disconnect(socket(), SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        qDebug("Reconnecting in compatibility mode...");
        d.connection->open();
        return;
    }

    AuthHandler::onSocketDisconnected();
}

void QuasselAuthHandler::onSocketError(QAbstractSocket::SocketError error)
{
    if (d.probing && error == QAbstractSocket::RemoteHostClosedError) {
        d.legacy = true;
        return;
    }

    d.probing = false; // all other errors are unrelated to probing and should be handled
    AuthHandler::onSocketError(error);
}

void QuasselAuthHandler::login()
{
    QString version = "Communi " + Irc::version();
    QString date = QString("%1 %2").arg(__DATE__, __TIME__);
    d.peer->dispatch(Protocol::RegisterClient(version, date, d.connection->isSecure()));
    d.peer->dispatch(Protocol::Login(userName(), d.connection->password()));
}

void QuasselAuthHandler::setPeer(RemotePeer* peer)
{
    d.peer = peer;
    d.legacy = peer->protocol() == Protocol::LegacyProtocol;
    if (d.legacy)
        connect(d.peer, SIGNAL(protocolVersionMismatch(int,int)), this, SIGNAL(protocolUnsupported()));
    socket()->setParent(d.connection); // reclaim ownership from RemotePeer
    login();
}
