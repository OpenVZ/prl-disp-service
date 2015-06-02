TEMPLATE = app
CONFIG = console

include(memleak8745.deps)

SOURCES +=	../main.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

macx:QMAKE_CFLAGS -=  -Werror
macx:QMAKE_CXXFLAGS -=  -Werror
