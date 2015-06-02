
TEMPLATE = subdirs

LEVEL = ../../..
include($$LEVEL/Sources/Build/Options.pri)

include($$PWD/UuidTest/UuidTest.deps)
include($$PWD/BitOpsTest/BitOpsTest.deps)
include($$PWD/SparseBitmapTest/SparseBitmapTest.deps)
