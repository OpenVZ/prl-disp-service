TEMPLATE = lib
CONFIG += staticlib

include(Buse.pri)

HEADERS += Buse.h
linux-*:SOURCES += Buse.cpp

