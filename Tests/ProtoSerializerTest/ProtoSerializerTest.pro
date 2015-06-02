CONFIG += qtestlib
QT = core xml network

include(ProtoSerializerTest.deps)

HEADERS += \
	CProtoSerializerTest.h\
	$$SRC_LEVEL/Tests/CommonTestsUtils.h \
	CDispToDispProtoSerializerTest.h \
	CVmMigrationProtoTest.h

SOURCES += \
	CProtoSerializerTest.cpp\
	$$SRC_LEVEL/Tests/CommonTestsUtils.cpp \
	CDispToDispProtoSerializerTest.cpp \
	CVmMigrationProtoTest.cpp \
	Main.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
