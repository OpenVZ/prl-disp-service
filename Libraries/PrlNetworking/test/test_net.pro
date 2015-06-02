TEMPLATE = app
TARGET = test_net
CONFIG += console

LEVEL=../../../..

include($$LEVEL/Sources/Parallels.pri)
include(../PrlNetworking.pri)


QMAKE_LIBDIR +=  $$LEVEL/z-Build/Debug

HEADERS += test.h
SOURCES += test.cpp

LIBS += -lLogging -lprl_xml_model -lprl_common_utils -lStd
win32:LIBS += iphlpapi.lib ws2_32.lib
macx:LIBS += -framework IOKit -framework CoreFoundation -framework SystemConfiguration
