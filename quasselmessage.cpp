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

#include "quasselmessage.h"
#include "message.h"
#include <IrcConnection>
#include <IrcMessage>

IRC_USE_NAMESPACE

namespace Quassel
{
    static QString messageCommand(Message::Type type)
    {
        switch (type)
        {
        case Message::Plain:        return QLatin1String("PRIVMSG");
        case Message::Notice:       return QLatin1String("NOTICE");
        case Message::Action:       return QLatin1String("PRIVMSG");
        case Message::Nick:         return QLatin1String("NICK");
        case Message::Mode:         return QLatin1String("MODE");
        case Message::Join:         return QLatin1String("JOIN");
        case Message::Part:         return QLatin1String("PART");
        case Message::Quit:         return QLatin1String("QUIT");
        case Message::Kick:         return QLatin1String("KICK");
        case Message::Kill:         return QString();
        case Message::Server:       return QString();
        case Message::Info:         return QString();
        case Message::Error:        return QString();
        case Message::DayChange:    return QString();
        case Message::Topic:        return QLatin1String("TOPIC");
        case Message::NetsplitJoin: return QLatin1String("JOIN");
        case Message::NetsplitQuit: return QLatin1String("QUIT");
        case Message::Invite:       return QLatin1String("INVITE");
        default:                    return QString();
        }
    }

    QList<IrcMessage*> convertMessage(const Message& message, IrcConnection* connection)
    {
        QList<IrcMessage*> msgs;
        if (message.flags() & Message::Self)
            return msgs;

        QString buffer = message.bufferInfo().bufferName();
        QString command = messageCommand(message.type());
        QString contents = message.contents();
        QStringList split;
        if (message.type() == Message::Mode || message.type() == Message::Kick ||
                message.type() == Message::Topic || message.type() == Message::Invite)
            split = contents.split(' ', QString::SkipEmptyParts);

        switch (message.type())
        {
        case Message::Action:
            contents = QString("\1ACTION %1\1").arg(contents);
            // flow through
        case Message::Plain:
        case Message::Notice:
        case Message::Join:
        case Message::Part:
            msgs += IrcMessage::fromParameters(message.sender(), command, QStringList() << buffer << contents, connection);
            break;
        case Message::Nick:
        case Message::Quit:
            msgs += IrcMessage::fromParameters(message.sender(), command, QStringList() << contents, connection);
            break;
        case Message::Mode:
            msgs += IrcMessage::fromParameters(message.sender(), command, split, connection);
            break;
        case Message::Kick:
            msgs += IrcMessage::fromParameters(message.sender(), command, QStringList() << buffer << split.value(0) << QStringList(split.mid(1)).join(" "), connection);
            break;
        case Message::Topic:
            // There's no sane way to parse the topic from a _localized_ message that could be:
            // - "Topic for #channel is "topic"
            // - "Topic set by nick!user@host on ..."
            // - "Homepage for #channel is ..."
            // - "nick has changed topic for #channel to: "topic""
            break;
        case Message::Invite:
            msgs += IrcMessage::fromParameters(split.first(), command, QStringList() << connection->nickName() << split.last(), connection);
            break;
        case Message::Server:
            msgs += IrcMessage::fromParameters(message.sender(), QString::number(Irc::RPL_WELCOME), QStringList() << "*" << message.contents(), connection);
            break;
        case Message::Error:
            msgs += IrcMessage::fromParameters("ERROR", "NOTICE", QStringList() << "*" << contents, connection);
            break;
        case Message::NetsplitQuit:
            split = contents.split("#:#");
            for (int i = 0; i < split.count() - i; ++i)
                msgs += IrcMessage::fromParameters(split.at(i), command, QStringList() << split.last(), connection);
            break;
            // TODO:
        case Message::Kill:
        case Message::Info:
        case Message::DayChange:
        case Message::NetsplitJoin:
        default:
            qDebug() << "Quassel::convertMessage(): TODO:" << QString::number(message.type(), 16) << message.sender() << message.contents();
            break;
        }

        foreach (IrcMessage* msg, msgs) {
            msg->setTimeStamp(message.timestamp());
            if (message.flags() & Message::Backlog)
                msg->setFlags(msg->flags() | IrcMessage::Playback);
        }

        return msgs;
    }
}
