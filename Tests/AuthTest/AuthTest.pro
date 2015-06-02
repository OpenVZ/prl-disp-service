TARGET		= AuthTest
TEMPLATE	= app
CONFIG		+= console
QT			-= gui

LEVEL = ../../..
include($$LEVEL/Sources/Parallels.pri)

INCLUDEPATH +=  $$LEVEL/Sources

SOURCES 	+= AuthTest.cpp

include($$LEVEL/Sources/Libraries/CAuth/CAuth.pri)
include($$LEVEL/Sources/Libraries/Logging/Logging.pri)
include($$LEVEL/Sources/Libraries/Std/Std.pri)
