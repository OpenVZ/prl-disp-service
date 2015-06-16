
TEMPLATE = subdirs

LEVEL = ../..
include($$LEVEL/Build/Options.pri)

include($$PWD/UuidTest/UuidTest.deps)
include($$PWD/BitOpsTest/BitOpsTest.deps)
include($$PWD/SparseBitmapTest/SparseBitmapTest.deps)
