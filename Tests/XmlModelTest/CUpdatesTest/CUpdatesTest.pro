CONFIG += qtestlib
QT = xml core network

INCLUDEPATH += /usr/share /usr/include/prlsdk

include(CUpdatesTest.deps)

HEADERS += CUpdatesTest.h
SOURCES += CUpdatesTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
