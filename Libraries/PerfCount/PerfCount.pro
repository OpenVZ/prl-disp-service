
TEMPLATE = subdirs

LEVEL = ../../..

include($$LEVEL/Sources/Build/Options.pri)

# Each and every subproject should use Parallels.pri constraints.
# Even 'subdirs' template needs this (during qmake generation)
# To prevent fake 'moc' folder generation.
include($$LEVEL/Sources/Parallels.pri)

# Use 'addSubdirs' instead of 'SUBDIRS +='
# if project file name is different from folder name

addSubdirs(PerfLib, PerfLib/PerfCounter.pro)
addSubdirsDir(PerfTest, PerfTest, PerfLib)
addSubdirsDir(PerfCtl, PerfCtl, PerfLib)
addSubdirs(Usage, Usage/PerfUsage.pro)
