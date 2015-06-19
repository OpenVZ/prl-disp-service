CONFIG += qtestlib
QT = xml core

INCLUDEPATH += /usr/share /usr/include/prlsdk

include(CHwFileSystemInfoTest.deps)

HEADERS += CHwFileSystemInfoTest.h

SOURCES += CHwFileSystemInfoTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
