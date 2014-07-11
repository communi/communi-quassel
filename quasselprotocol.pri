######################################################################
# Communi
######################################################################

isEmpty(QUASSELDIR):error(QUASSELDIR must be set)

PROTODIR = $$QUASSELDIR/protocols

INCLUDEPATH += $$PWD $$QUASSELDIR $$PROTODIR/datastream $$PROTODIR/legacy
DEPENDPATH += $$PWD $$QUASSELDIR $$PROTODIR/datastream $$PROTODIR/legacy

HEADERS += $$PWD/quasselauthhandler.h
HEADERS += $$PWD/quasselbacklog.h
HEADERS += $$PWD/quasselmessage.h
HEADERS += $$PWD/quasselprotocol.h
HEADERS += $$PWD/quasseltypes.h

SOURCES += $$PWD/quasselauthhandler.cpp
SOURCES += $$PWD/quasselbacklog.cpp
SOURCES += $$PWD/quasselmessage.cpp
SOURCES += $$PWD/quasselprotocol.cpp

HEADERS += $$QUASSELDIR/authhandler.h
HEADERS += $$QUASSELDIR/backlogmanager.h
HEADERS += $$QUASSELDIR/bufferinfo.h
HEADERS += $$QUASSELDIR/compressor.h
HEADERS += $$QUASSELDIR/identity.h
HEADERS += $$QUASSELDIR/ircchannel.h
HEADERS += $$QUASSELDIR/ircuser.h
HEADERS += $$QUASSELDIR/message.h
HEADERS += $$QUASSELDIR/network.h
HEADERS += $$QUASSELDIR/protocol.h
HEADERS += $$QUASSELDIR/peer.h
HEADERS += $$QUASSELDIR/remotepeer.h
HEADERS += $$QUASSELDIR/signalproxy.h
HEADERS += $$QUASSELDIR/syncableobject.h
HEADERS += $$QUASSELDIR/types.h
HEADERS += $$QUASSELDIR/util.h

SOURCES += $$QUASSELDIR/authhandler.cpp
SOURCES += $$QUASSELDIR/backlogmanager.cpp
SOURCES += $$QUASSELDIR/bufferinfo.cpp
SOURCES += $$QUASSELDIR/compressor.cpp
SOURCES += $$QUASSELDIR/identity.cpp
SOURCES += $$QUASSELDIR/ircchannel.cpp
SOURCES += $$QUASSELDIR/ircuser.cpp
SOURCES += $$QUASSELDIR/message.cpp
SOURCES += $$QUASSELDIR/network.cpp
SOURCES += $$QUASSELDIR/peer.cpp
SOURCES += $$QUASSELDIR/remotepeer.cpp
SOURCES += $$QUASSELDIR/signalproxy.cpp
SOURCES += $$QUASSELDIR/syncableobject.cpp
SOURCES += $$QUASSELDIR/util.cpp

HEADERS += $$PROTODIR/datastream/datastreampeer.h
SOURCES += $$PROTODIR/datastream/datastreampeer.cpp

HEADERS += $$PROTODIR/legacy/legacypeer.h
SOURCES += $$PROTODIR/legacy/legacypeer.cpp

win32:SOURCES += $$QUASSELDIR/logbacktrace_win.cpp
else:unix:SOURCES += $$QUASSELDIR/logbacktrace_unix.cpp

mac {
    HEADERS += $$QUASSELDIR/mac_utils.h
    SOURCES += $$QUASSELDIR/mac_utils.cpp
    LIBS += -framework AppKit
}

LIBS += -lz
DEFINES += HAVE_ZLIB

#SOURCES += $$PWD/3rdparty/3rdparty/miniz/miniz.c
