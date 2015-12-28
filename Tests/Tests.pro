TEMPLATE = subdirs

LEVEL = ..

#archive.target = $$LEVEL/z-Build/unittests.zip
#archive.target = $$LEVEL/z-Build/Debug/out.txt
#archive.command = $$PWD/PackTests.sh
#QMAKE_EXTRA_TARGETS += archive
#
#POST_TARGETDEPS += archive

include($$LEVEL/Build/Options.pri)

include($$PWD/DispatcherInternalTest/DispatcherInternalTest.deps)
include($$PWD/XmlModelTest/XmlModelTest.pro)
#include($$PWD/SDKTest/SDKTest.deps)
include($$PWD/ProtoSerializerTest/ProtoSerializerTest.deps)
#include($$PWD/DesktopControlSDKTest/DesktopControlSDKTest.deps)

#include($$PWD/SDKTest/SDKPrivateTest/SDKPrivateTest.deps)
#include($$PWD/SDKTest/NonInitializedSDKTest/NonInitializedSdkTest.deps)
#include($$PWD/SDKTest/RaceOnSdkDeinitTest/RaceOnSdkDeinitTest.deps)
#include($$PWD/SDKTest/ApiInitDeinitTest/ApiInitDeinitTest.deps)

include($$PWD/QtLibraryTest/QtLibraryTest.deps)
addSubdirsDir(ArchiveTests, $$PWD/ArchiveTests)

#include($$SRC_LEVEL/VI/Tests/ThirdPartyVmConfigTest/ThirdPartyVmConfigTest.deps)
include($$SRC_LEVEL/Libraries/PowerWatcher/test/PowerWatcherTest.deps)
