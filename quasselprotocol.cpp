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

#include "quasselprotocol.h"
#include "quasselauthhandler.h"
#include "quasselmessage.h"
#include "backlogmanager.h"
#include "quasselbacklog.h"
#include "quasseltypes.h"
#include "remotepeer.h"
#include "bufferinfo.h"
#include "protocol.h"
#include "network.h"
#include "message.h"

IRC_USE_NAMESPACE

QuasselProtocol::QuasselProtocol(IrcConnection* connection) : IrcProtocol(connection)
{
    d.proxy = 0;
    d.handler = 0;
    d.network = 0;

    Quassel::registerTypes();

    d.backlog = new QuasselBacklog(this);
    connect(d.backlog, SIGNAL(messageReceived(Message)), this, SLOT(receiveMessage(Message)));
}

QuasselProtocol::~QuasselProtocol()
{
}

void QuasselProtocol::open()
{
    if (d.handler)
        return;

    d.proxy = new SignalProxy(this);
    d.proxy->attachSignal(this, SIGNAL(sendInput(BufferInfo,QString)));
    d.proxy->attachSlot(SIGNAL(displayMsg(Message)), this, SLOT(receiveMessage(Message)));

    d.handler = new QuasselAuthHandler(connection());
    connect(d.handler, SIGNAL(clientDenied(Protocol::ClientDenied)), this, SLOT(clientDenied(Protocol::ClientDenied)));
    connect(d.handler, SIGNAL(clientRegistered(Protocol::ClientRegistered)), this, SLOT(clientRegistered(Protocol::ClientRegistered)));
    connect(d.handler, SIGNAL(loginFailed(Protocol::LoginFailed)), this, SLOT(loginFailed(Protocol::LoginFailed)));
    connect(d.handler, SIGNAL(loginSucceed(Protocol::LoginSuccess)), this, SLOT(loginSucceed(Protocol::LoginSuccess)));
    connect(d.handler, SIGNAL(sessionState(Protocol::SessionState)), this, SLOT(sessionState(Protocol::SessionState)));
    d.handler->authenticate();
}

void QuasselProtocol::close()
{
    if (d.handler && d.handler->isProbing())
        return;

    if (d.proxy) {
        d.proxy->deleteLater();
        d.proxy = 0;
    }
    if (d.handler) {
        d.handler->deleteLater();
        d.handler = 0;
    }
    if (d.network) {
        d.network->deleteLater();
        d.network = 0;
    }
}

void QuasselProtocol::read()
{
}

bool QuasselProtocol::write(const QByteArray& data)
{
    if (data.length() >= 4) {
        const QByteArray cmd = data.left(5).toUpper();
        if (cmd.startsWith("QUIT") && (data.length() == 4 || QChar(data.at(4)).isSpace()))
            return true;
    }

    QByteArray line(data);
    const QByteArray cmd = line.mid(0, line.indexOf(' '));
    line.remove(0, cmd.length() + 1);

    if (cmd == "PRIVMSG" || cmd == "NOTICE") {
        const QByteArray target = line.mid(0, line.indexOf(' '));
        line.remove(0, target.length() + 1);
        if (line.startsWith(":")) {
            BufferInfo buffer = findBuffer(QString::fromUtf8(target));
            if (buffer.isValid()) {
                line.remove(0, 1);
                if (line.startsWith("\1ACTION ") && line.endsWith('\1')) {
                    line.remove(0, 8);
                    line.remove(line.length() - 1, 1);
                    emit sendInput(buffer, "/ME " + QString::fromUtf8(line));
                } else if (cmd == "PRIVMSG") {
                    emit sendInput(buffer, "/SAY " + QString::fromUtf8(line));
                } else {
                    emit sendInput(buffer, "/NOTICE " + buffer.bufferName() + " " + QString::fromUtf8(line));
                }
                return true;
            }
        }
    } else {
        BufferInfo buffer = BufferInfo::fakeStatusBuffer(d.network->networkId());
        emit sendInput(buffer, "/QUOTE " + QString::fromUtf8(data));
        return true;
    }
    return false;
}

void QuasselProtocol::initNetwork()
{
    setStatus(IrcConnection::Connected);
    receiveInfo(Irc::RPL_MYINFO, "Done");

    QHash<QString, QString> info;
    info.insert("NETWORK", d.network->support("NETWORK"));
    info.insert("PREFIX", d.network->support("PREFIX"));
    info.insert("CHANTYPES", d.network->support("CHANTYPES"));
    // TODO: ...
    setInfo(info);

    foreach (const QString& name, d.network->channels())
        addChannel(d.network->ircChannel(name));
    connect(d.network, SIGNAL(ircChannelAdded(IrcChannel*)), SLOT(addChannel(IrcChannel*)));

    d.proxy->synchronize(d.backlog);
    foreach (const BufferInfo& buffer, d.buffers)
        d.backlog->requestBacklog(buffer.bufferId(), -1, -1, 100); // TODO: limit (100)
}

void QuasselProtocol::addChannel(IrcChannel* channel)
{
    if (!channel->isInitialized())
        connect(channel, SIGNAL(initDone()), this, SLOT(initChannel()));
    else
        initChannel(channel);
}

void QuasselProtocol::initChannel(IrcChannel* channel)
{
    if (!channel)
        channel = qobject_cast<IrcChannel*>(sender());

    if (channel) {
        IrcMessage* msg = IrcMessage::fromParameters(prefix(), "JOIN", QStringList() << channel->name(), connection());
        IrcProtocol::receiveMessage(msg);

        updateTopic(channel);
        updateUsers(channel);
        connect(channel, SIGNAL(topicSet(QString)), this, SLOT(updateTopic()));
    }
}

void QuasselProtocol::updateTopic(IrcChannel* channel)
{
    if (!channel)
        channel = qobject_cast<IrcChannel*>(sender());

    if (channel) {
        IrcMessage* msg = 0;
        if (!channel->topic().isEmpty())
            msg = IrcMessage::fromParameters(prefix(), QString::number(Irc::RPL_TOPIC), QStringList() << connection()->nickName() << channel->name() << channel->topic(), connection());
        else
            msg = IrcMessage::fromParameters(prefix(), QString::number(Irc::RPL_NOTOPIC), QStringList() << connection()->nickName() << channel->name(), connection());
        IrcProtocol::receiveMessage(msg);
    }
}

void QuasselProtocol::updateUsers(IrcChannel* channel)
{
    if (channel) {
        QStringList users;
        Network* network = channel->network();
        foreach (IrcUser* user, channel->ircUsers()) {
            QString nick = user->nick();
            QString prefix = channel->userModes(nick);
            if (!prefix.isEmpty())
                prefix = network->modeToPrefix(prefix);
            users += prefix + nick;
        }
        IrcMessage* msg = new IrcNamesMessage(connection());
        msg->setPrefix(prefix());
        msg->setCommand("NAMES");
        msg->setParameters(QStringList() << channel->name() << users);
        IrcProtocol::receiveMessage(msg);
    }
}

void QuasselProtocol::receiveMessage(const Message& message)
{
    BufferInfo buffer = message.bufferInfo();
    if (buffer.networkId() == d.network->networkId()) {
        d.buffers.insert(buffer.bufferName(), buffer);

        QList<IrcMessage*> msgs = Quassel::convertMessage(message, connection());
        foreach (IrcMessage* msg, msgs)
            IrcProtocol::receiveMessage(msg);
        d.lastMsg = message.msgId();
    }
}

static QList<int> toIntList(const QVariantList& networkIds)
{
    QList<int> lst;
    foreach (const QVariant& v, networkIds) {
        NetworkId nid = v.value<NetworkId>();
        lst += nid.toInt();
    }
    return lst;
}

static QString toString(const QList<int>& ids)
{
    QStringList strings;
    foreach (int id, ids)
        strings += QString::number(id);
    return strings.join(",");
}

static NetworkId findNetworkId(const NetworkId& nid, const QList<int>& networkIds)
{
    if (nid.isValid()) {
        if (networkIds.contains(nid.toInt()))
            return nid;
    } else if (networkIds.count() == 1) {
        return NetworkId(networkIds.first());
    }
    return NetworkId();
}

void QuasselProtocol::protocolUnsupported()
{
    receiveError(tr("Unsupported Quassel protocol"));
}

void QuasselProtocol::clientDenied(const Protocol::ClientDenied& msg)
{
    receiveError(tr("Client denied: %1").arg(msg.errorString));
}

void QuasselProtocol::clientRegistered(const Protocol::ClientRegistered& msg)
{
    Q_UNUSED(msg);
    setStatus(IrcConnection::Connecting);
    receiveInfo(Irc::RPL_MYINFO, "Welcome to Quassel");
}

void QuasselProtocol::loginFailed(const Protocol::LoginFailed& msg)
{
    receiveError(tr("Login failed: %1").arg(msg.errorString));
}

void QuasselProtocol::loginSucceed(const Protocol::LoginSuccess& msg)
{
    Q_UNUSED(msg);
}

void QuasselProtocol::sessionState(const Protocol::SessionState& msg)
{
    QList<int> nids = toIntList(msg.networkIds);
    receiveInfo(Irc::RPL_MYINFO, QString("Available networks: (%1)").arg(toString(nids)));

    NetworkId nid = findNetworkId(d.handler->networkId(), nids);
    if (nid.isValid()) {
        RemotePeer* peer = d.handler->peer();
        peer->setParent(d.proxy);
        d.proxy->addPeer(peer);
        d.network = new Network(nid, this);
        foreach (const QVariant& v, msg.bufferInfos) {
            BufferInfo buffer = v.value<BufferInfo>();
            if (buffer.networkId() == nid)
                d.buffers.insert(buffer.bufferName(), buffer);
        }
        connect(d.network, SIGNAL(initDone()), this, SLOT(initNetwork()));
        d.network->setProxy(d.proxy);
        d.proxy->synchronize(d.network);
        receiveInfo(Irc::RPL_MYINFO, QString("Connected to network %1").arg(nid.toInt()));
        receiveInfo(Irc::RPL_MYINFO, "Synchronizing...");
    } else {
        if (d.handler->networkId().isValid())
            receiveError(QString("Unknown network: %1").arg(d.handler->networkId().toInt()));
        else
            receiveError(QString("Choose a network by setting username '%1/network'").arg(d.handler->userName()));
    }

    d.handler->deleteLater();
    d.handler = 0;
}

QString QuasselProtocol::prefix() const
{
    return connection()->nickName() + "!" + connection()->userName() + "@quassel";
}

BufferInfo QuasselProtocol::findBuffer(const QString& name) const
{
    foreach (const BufferInfo& buffer, d.buffers) {
        if (!buffer.bufferName().compare(name, Qt::CaseInsensitive))
            return buffer;
    }
    return BufferInfo();
}

void QuasselProtocol::receiveInfo(int code, const QString &info)
{
    IrcMessage* msg = IrcMessage::fromParameters(prefix(), QString::number(code), QStringList() << connection()->nickName() << info, connection());
    IrcProtocol::receiveMessage(msg);
}

void QuasselProtocol::receiveError(const QString& error)
{
    receiveInfo(Irc::ERR_UNKNOWNERROR, error);
    connection()->close();
    setStatus(IrcConnection::Error);
}
