LEVEL = ../..

include($$LEVEL/Sources/Parallels.pri)
include($$LEVEL/Sources/Libraries/CAuth/CAuth.pri)
include($$LEVEL/Sources/Libraries/PrlCommonUtils/PrlCommonUtils.pri)
include($$LEVEL/Sources/Libraries/Std/Std.pri)

QT += core
CONFIG += console

# AutoLogin.exe should be linked sith runtime library statically.
#QMAKE_CFLAGS_RELEASE	-= -MD
#QMAKE_CFLAGS_DEBUG	-= -MDd
#QMAKE_CFLAGS_RELEASE	+= -MT
#QMAKE_CFLAGS_DEBUG	+= -MTd

#QMAKE_CXXFLAGS_RELEASE	-= -MD
#QMAKE_CXXFLAGS_DEBUG	-= -MDd
#QMAKE_CXXFLAGS_RELEASE	+= -MT
#QMAKE_CXXFLAGS_DEBUG	+= -MTd

#qt_shared {
#	QMAKE_LFLAGS += /NODEFAULTLIB:libcmt /NODEFAULTLIB:libcmtd
#}
#QMAKE_LFLAGS += /NODEFAULTLIB:msvcrtd /NODEFAULTLIB:msvcprtd


TARGET = autologin_test
TEMPLATE = app


# MCSVCRT80 is not necessary for this application, but there is reference on it in generated manifest file.
#win32:CONFIG -= embed_manifest_exe


SOURCES = main.cpp \
	  $$LEVEL/Sources/VI/Sources/STW7Agent/LoginHelper/CLoginHelper.cpp

INCLUDEPATH += $$LEVEL/Sources/VI/Sources/STW7Agent/LoginHelper

LIBS+= -lUser32 -lAdvapi32 -lKernel32 -lSecur32 -lShell32 -lutw7_autologin

