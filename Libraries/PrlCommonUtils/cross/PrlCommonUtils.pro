TEMPLATE = lib
CONFIG += staticlib force_mac_fat_bin

DEFINES += PRINTABLE_TARGET=cmn_utils

include(PrlCommonUtils.pri)

HEADERS = \
		../OsInfo.h \
		../TerminalUtils.h

SOURCES = \
		../OsInfo.c \
		../TerminalUtils.c
