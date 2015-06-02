CONFIG += qtestlib core

include(DsaTest.deps)

# Uncomment this line to define additional include paths
# INCLUDEPATH *= $$SRC_LEVEL/VI/Interfaces

# Do not edit variables below, they are updated automatically
TESTS_H =  CDsaWrapTest.h
TESTS_CPP =  CDsaWrapTest.cpp

HEADERS += \
	$$TESTS_H \
	$$SRC_LEVEL/Tests/CommonTestsUtils.h

SOURCES += \
	$$TESTS_CPP \
	main.cpp \
	$$SRC_LEVEL/Tests/CommonTestsUtils.cpp
