TEMPLATE = lib
CONFIG += staticlib

include(CpuFeatures.pri)

# Input
HEADERS +=                      \
	CCpuHelper.h

SOURCES +=                      \
	CCpuHelper.cpp
