TEMPLATE = app
CONFIG += qtestlib
QT = core

include(PowerWatcherTest.deps)

HEADERS += PowerWatcherTest.h

SOURCES += PowerWatcherTest.cpp

DEFINES += PRINTABLE_TARGET=PowerWatcherTest

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
