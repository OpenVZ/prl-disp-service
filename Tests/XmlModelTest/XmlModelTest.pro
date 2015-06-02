
TEMPLATE = subdirs

LEVEL = ../../..
include($$LEVEL/Sources/Build/Options.pri)

include($$PWD/CDispatcherConfigTest/CDispatcherConfigTest.deps)
include($$PWD/CHwFileSystemInfoTest/CHwFileSystemInfoTest.deps)
include($$PWD/CProblemReportTest/CProblemReportTest.deps)
include($$PWD/CVmConfigurationTest/CVmConfigurationTest.deps)
include($$PWD/CClientPreferencesTest/CClientPreferencesTest.deps)
include($$PWD/CVmDirectoriesTest/CVmDirectoriesTest.deps)
include($$PWD/CVmEventTest/CVmEventTest.deps)
include($$PWD/CSystemStatisticsTest/CSystemStatisticsTest.deps)
include($$PWD/CUpdaterConfigTest/CUpdaterConfigTest.deps)
include($$PWD/CUpdatesTest/CUpdatesTest.deps)
include($$PWD/CHostHardwareInfoTest/CHostHardwareInfoTest.deps)
