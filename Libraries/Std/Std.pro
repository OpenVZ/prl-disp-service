TEMPLATE = lib
CONFIG += staticlib

include(Std.pri)

INCLUDEPATH += ../../..

	HEADERS += \
		   SmartPtr.h  \
		   PrlAssert.h \
		   RingBuffer.h \
		DumpArray.h

	SOURCES += \
		RingBuffer.c \
		DumpArray.cpp

	HEADERS += \
		   PrlTime.h   \
		   LockedPtr.h  \
		   ProcessWatcher.h \
		   condwait.h \
		   Base32.h \
		   BitOps.h \
		   pollset.h \
		   poll_event.h \
		   pollset_private.h \
		   delete_ptr.h \
		   HackParam.h

	SOURCES += \
		PrlTime.cpp    \
		ProcessWatcher.cpp \
		condwait.cpp \
		poll_event.cpp \
		Base32.cpp \
		rbtree.c \
		pollset.cpp \
		HackParam.cpp

	linux-*:SOURCES += \
			PrlTime_lin.cpp \
			pollset_lin.cpp

	win32:SOURCES += \
			pollset_win.cpp \
			PrlTime_win.cpp

