TEMPLATE = lib
CONFIG += staticlib

include(Virtuozzo.pri)

KSRC  = $$EXT_LEVEL/Virtuozzo
# INCLUDEPATH += ../../
INCLUDEPATH += $$KSRC/include
win32:INCLUDEPATH += $$KSRC/include/vzwin

DEFINES += VZCTL_NETSTAT

# Input
HEADERS +=		\
	CVzHelper.h	\
	CVzTemplateHelper.h	\
	CVzNetworkShaping.h \
	CVzPrivateNetwork.h \
	UuidMap.h

SOURCES =		\
	CVzHelper.cpp	\
	CVzTemplateHelper.cpp	\
	CVzNetworkShaping.cpp \
	CVzPrivateNetwork.cpp \
	UuidMap.cpp \
	CVzPloop.cpp

contains(DYN_VZLIB, TRUE) {
	DEFINES += _DYN_VZLIB_
}
