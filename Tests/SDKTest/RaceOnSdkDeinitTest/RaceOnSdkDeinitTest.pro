CONFIG += qtestlib
QT = core

include(RaceOnSdkDeinitTest.deps)

HEADERS +=	RaceOnSdkDeinitTest.h \
			$$SRC_LEVEL/Tests/DispatcherTestsUtils.h

SOURCES +=	RaceOnSdkDeinitTest.cpp\

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

