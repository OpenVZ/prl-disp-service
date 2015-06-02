TEMPLATE = app
CONFIG += console warn_on
QT = core gui network

include(Server.deps)

FORMS += Server.ui
SOURCES += Server.cpp
HEADERS += Server.h

DEFINES += _UNICODE

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
