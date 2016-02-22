CONFIG += qtestlib
QT = network core xml

include(SDKPrivateTest.deps)

HEADERS +=	PrlHandleSmartPtrTest.h \
			PrlApiTest.h \
			PrlHandleServerJobTest.h \
			PrlApiEventsTest.h \
			PrlHandleVmDeviceTest.h \
			PrlHandleVmTest.h \
			PrlQuestionsListTest.h \
			MigrationFlagsMacrosesTest.h \
			PrlHandlePluginInfoTest.h \
			$$SRC_LEVEL/Tests/CommonTestsUtils.h

SOURCES +=	PrlHandleSmartPtrTest.cpp \
			PrlApiTest.cpp \
			PrlHandleServerJobTest.cpp \
			PrlApiEventsTest.cpp \
			PrlHandleVmDeviceTest.cpp \
			PrlHandleVmTest.cpp \
			PrlQuestionsListTest.cpp \
			MigrationFlagsMacrosesTest.cpp \
			PrlHandlePluginInfoTest.cpp \
			Main.cpp \
			$$SRC_LEVEL/Tests/CommonTestsUtils.cpp

include($$SRC_LEVEL/SDK/Handles/SDKSources.pri)

macx {
  LIBS += \
		-framework Cocoa 					\
		-framework IOKit 					\
		-framework CoreFoundation 			\
		-framework ApplicationServices 		\
		-framework SystemConfiguration 		\
		-framework CoreAudio 				\
		-framework AudioUnit 				\
		-framework DiskArbitration 			\
		-framework AudioToolbox				\
		-framework Security \
		-framework DirectoryService
}

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

# Workaround for linker bug (nested class multiple definition in ContextSwitcher)
win32:win32-msvc2005:QMAKE_LFLAGS += /FORCE:MULTIPLE
