LEVEL = ../../..

include($$LEVEL/Sources/Parallels.pri)
include($$LEVEL/Sources/Libraries/WifiHelper/WifiHelper.pri)
include($$LEVEL/Sources/Libraries/Std/Std.pri)
include($$LEVEL/Sources/Libraries/Logging/Logging.pri)

QT = core
CONFIG += console

# AutoLogin.exe should be linked sith runtime library statically.
QMAKE_CFLAGS_RELEASE	-= -MD
QMAKE_CFLAGS_DEBUG	+= -MDd
QMAKE_CFLAGS_RELEASE	+= -MT
QMAKE_CFLAGS_DEBUG	+= -MTd

QMAKE_CXXFLAGS_RELEASE	-= -MD
QMAKE_CXXFLAGS_DEBUG	+= -MDd
QMAKE_CXXFLAGS_RELEASE	+= -MT
QMAKE_CXXFLAGS_DEBUG	+= -MTd

#qt_shared {
#	QMAKE_LFLAGS += /NODEFAULTLIB:libcmt /NODEFAULTLIB:libcmtd
#}
#AKE_LFLAGS += /NODEFAULTLIB:msvcrtd /NODEFAULTLIB:msvcprtd


TARGET = wlan_test
TEMPLATE = app

SOURCES += \
	main.cpp

