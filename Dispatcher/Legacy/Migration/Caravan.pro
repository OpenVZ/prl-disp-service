TEMPLATE = app
CONFIG += console qt warn_on thread

QT += network core xml gui

include(Caravan.deps)

DEFINES += PRINTABLE_TARGET=migration_app
DEFINES += BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS BOOST_MPL_LIMIT_VECTOR_SIZE=40

INCLUDEPATH *= \
	. \
	.. \

HEADERS += \
	CVmMigrateTargetDisk.h \
	CVmMigrateTargetServer.h \
	CVmMigrateTargetServer_p.h \
	Cancellation.h \
	BlockingQueue.h \
	$$SRC_LEVEL/Libraries/VmFileList/CVmFileListCopy.h


SOURCES += \
	main.cpp \
	CVmMigrateTargetDisk.cpp \
	CVmMigrateTargetServer.cpp \
	Cancellation.cpp \
	$$SRC_LEVEL/Libraries/VmFileList/CVmFileListCopy.cpp

