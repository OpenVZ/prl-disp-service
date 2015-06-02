CONFIG += qtestlib
QT = xml core network

include(CDispatcherConfigTest.deps)

HEADERS += CDispatcherConfigTest.h

SOURCES += CDispatcherConfigTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

