TEMPLATE = lib
CONFIG += staticlib

include(PrlNetworking.pri)

DEFINES += PRINTABLE_TARGET=prl_net

INCLUDEPATH += $$ROOT_LEVEL

HEADERS += PrlNetLibrary.h PrlNetworkingConstants.h IpStatistics.h \
	    PrlNetInternal.h netconfig.h

SOURCES += VMNetworking.cpp netconfig.cpp
SOURCES += $$LIBS_LEVEL/Std/HackParam.cpp

linux-* {
	HEADERS += unix/ethlist.h
	SOURCES += unix/ethlist.cpp

	SOURCES += unix/PrlNetLibrary.cpp
	SOURCES += unix/PrlNetLibrary_lin.cpp

	SOURCES += unix/IpStatistics.cpp

	SOURCES += unix/VMDefaultAdapter_unix.cpp

	HEADERS += unix/libnetlink.h
	SOURCES += unix/libnetlink.cpp

	HEADERS += unix/libarp.h
	SOURCES += unix/libarp.cpp

	SOURCES += unix/PrlRoutes.cpp
}


win32 {
	SOURCES += win/PrlNetLibrary_win.cpp

	HEADERS	+= win/ethlist.h win/ServiceControl.h
	SOURCES += win/ethlist.cpp win/ServiceControl.cpp

	HEADERS += win/WinNetworkHelpers.h
	SOURCES += win/WinNetworkHelpers.cpp

	SOURCES += win/IpStatistics.cpp
}

