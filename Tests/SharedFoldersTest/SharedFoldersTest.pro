TEMPLATE = app
win32:CONFIG += console
QT=

include(SharedFoldersTest.deps)

HEADERS += \
	testutils.h

SOURCES += \
	testutils.cpp \
	main.cpp

win32:LIBS += \
	user32.lib
