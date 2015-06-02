CONFIG += qtestlib
QT = core xml

include(PrlDataSerializerTest.deps)

HEADERS += \
	CPrlDataSerializerTest.h\
	$$SRC_LEVEL/Tests/CommonTestsUtils.h

SOURCES += CPrlDataSerializerTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
