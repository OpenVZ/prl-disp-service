TEMPLATE = lib
CONFIG += staticlib
QTCONFIG = xml

include(VmFileList.pri)

HEADERS += \
	CVmFileListCopy.h

SOURCES += \
	CVmFileListCopy.cpp
