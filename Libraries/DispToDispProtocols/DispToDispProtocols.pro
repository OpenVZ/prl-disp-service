TEMPLATE = lib
CONFIG += staticlib
include(DispToDispProtocols.pri)

HEADERS += \
	CDispToDispCommonProto.h \
	CVmMigrationProto.h \
	CVmBackupProto.h \
	CCtTemplateProto.h \

SOURCES += \
	CDispToDispCommonProto.cpp \
	CVmMigrationProto.cpp \
	CVmBackupProto.cpp \
	CCtTemplateProto.cpp \

