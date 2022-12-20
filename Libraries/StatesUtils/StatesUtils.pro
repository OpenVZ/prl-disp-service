TEMPLATE = lib
CONFIG += staticlib

include(StatesUtils.pri)

DEFINES += BOOST_BIND_GLOBAL_PLACEHOLDERS

HEADERS += \
	StatesHelper.h \

SOURCES	+= \
	StatesHelper.cpp \
