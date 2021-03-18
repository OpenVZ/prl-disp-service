CONFIG += qtestlib
QT = core xml network

INCLUDEPATH += /usr/share /usr/include/prlsdk

include(ProtoSerializerTest.deps)

HEADERS += \
	CProtoSerializerTest.h\
	$$SRC_LEVEL/Tests/DispatcherTestsUtils.h \
	CDispToDispProtoSerializerTest.h \
	CVmMigrationProtoTest.h

SOURCES += \
	CProtoSerializerTest.cpp\
	CDispToDispProtoSerializerTest.cpp \
	CVmMigrationProtoTest.cpp \
	Main.cpp

linux-g++* {
	system(ld -lboost_thread-mt) {
		message( boost-with-mt )
		CONFIG += boost-with-mt
	} else {
		system(ld -lboost_thread) {
			message( boost-without-mt )
			CONFIG += boost-without-mt
		}
	}
}

LIBS += -lprlcommon -lprlTestsUtils

boost-with-mt {
	LIBS += -lboost_filesystem-mt -lboost_system-mt
}
boost-without-mt {
	LIBS += -lboost_filesystem -lboost_system
}

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
