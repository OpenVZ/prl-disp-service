TEMPLATE = lib
CONFIG += staticlib

include(CpuFeatures.pri)

# Input
HEADERS +=                      \
	CCpuHelper.h \
	ChipsetHelper.h

SOURCES +=                      \
	CCpuHelper.cpp
