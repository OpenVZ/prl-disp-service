TEMPLATE = lib
CONFIG += staticlib

include(CAuth.pri)

HEADERS +=  CAuth.h\
						CAclHelper.h

SOURCES +=	CAclHelper.cpp

unix:SOURCES		+= CAuth_unix.cpp
win32:SOURCES	 	+= CAuth_win32.cpp

linux*:SOURCES	+= CAclHelper_lin.cpp
win*:SOURCES		+= CAclHelper_win.cpp
