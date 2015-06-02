CONFIG += qtestlib
QT = core

include(ApiInitDeinitTest.deps)

HEADERS +=	ApiInitDeinitTest.h \
			$$SRC_LEVEL/Tests/CommonTestsUtils.h

SOURCES +=	ApiInitDeinitTest.cpp\
			$$SRC_LEVEL/Tests/CommonTestsUtils.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
