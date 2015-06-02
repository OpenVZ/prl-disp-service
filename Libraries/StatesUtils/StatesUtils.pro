TEMPLATE = lib
CONFIG += staticlib

include(StatesUtils.pri)

HEADERS += \
	sarefile.h \
	StatesHelper.h \

SOURCES	+= \
	StatesHelper.cpp \
	sarefile.cpp \
	SaReShared.cpp \
