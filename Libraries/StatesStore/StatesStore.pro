TEMPLATE = lib
CONFIG += staticlib

include(StatesStore.pri)

HEADERS += 	SavedState.h \
		SavedStateTree.h \
		SavedStateStore.h \
		SavedStatesModel.h


SOURCES	+= 	SavedState.cpp \
		SavedStateTree.cpp \
		SavedStateStore.cpp
