CONFIG += qtestlib
QT = xml core network

include(DispatcherAPITest.deps)

HEADERS += \
	TestDispatcherBase.h\
	$$SRC_LEVEL/Tests/CMockPveEventsHandler.h\
	$$SRC_LEVEL/Tests/CommonTestsUtils.h\
	$$SRC_LEVEL/SDK/Handles/PveControl.h\
	TestDspCmdUserLogin.h\
	TestDspCmdDirGetVmList.h\
	TestDspCmdVmGetConfig.h\
	TestDspFSCommands.h\
	TestDspCmdUserGetProfile.h\
	TestDspCmdUserGetHostHwInfo.h\
	TestDspCmdDirVmCreate.h\
	TestDspCmdDirVmDelete.h\
	TestDspCmdDirVmClone.h\
	Main.h

SOURCES +=	\
	TestDispatcherBase.cpp\
	$$SRC_LEVEL/Tests/CMockPveEventsHandler.cpp\
	$$SRC_LEVEL/Tests/CommonTestsUtils.cpp\
	$$SRC_LEVEL/SDK/Handles/PveControl.cpp\
	Main.cpp\
	TestDspCmdUserLogin.cpp\
	TestDspCmdDirGetVmList.cpp\
	TestDspCmdVmGetConfig.cpp\
	TestDspFSCommands.cpp\
	TestDspCmdUserGetProfile.cpp\
	TestDspCmdUserGetHostHwInfo.cpp\
	TestDspCmdDirVmCreate.cpp\
	TestDspCmdDirVmDelete.cpp\
	TestDspCmdDirVmClone.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
