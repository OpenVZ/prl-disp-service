TEMPLATE = subdirs

LEVEL = ../..
include($$LEVEL/Build/Options.pri)

include($$PWD/CoreUtilsTest/CoreUtilsTest.deps)
include($$PWD/DispatcherInternalTest/DispatcherInternalTest.deps)
include($$PWD/XmlModelTest/XmlModelTest.pro)
#include($$PWD/SDKTest/SDKTest.deps)
include($$PWD/ProtoSerializerTest/ProtoSerializerTest.deps)
include($$PWD/PrlDataSerializerTest/PrlDataSerializerTest.deps)
addSubdirsDir(AtomicOpsTest, $$PWD/AtomicOpsTest)
addSubdirsDir(MonitorAtomicOpsTest, $$PWD/MonitorAtomicOpsTest)
#include($$PWD/DesktopControlSDKTest/DesktopControlSDKTest.deps)

#include($$PWD/SDKTest/SDKPrivateTest/SDKPrivateTest.deps)
#include($$PWD/SDKTest/NonInitializedSDKTest/NonInitializedSdkTest.deps)
#include($$PWD/SDKTest/RaceOnSdkDeinitTest/RaceOnSdkDeinitTest.deps)
#include($$PWD/SDKTest/ApiInitDeinitTest/ApiInitDeinitTest.deps)

include($$PWD/ParallelsDirTest/ParallelsDirTest.deps)
include($$PWD/StdTest/StdTest.pro)
include($$PWD/QtLibraryTest/QtLibraryTest.deps)

#win32:SUBDIRS += MonitorStdTest

#include($$SRC_LEVEL/VI/Tests/ThirdPartyVmConfigTest/ThirdPartyVmConfigTest.deps)
include($$SRC_LEVEL/Libraries/PowerWatcher/test/PowerWatcherTest.deps)
