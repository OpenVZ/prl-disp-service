CONFIG += qtestlib
QT += xml core
QT -= gui

TARGET = test_qtservice

LEVEL = ./../../../
include($$LEVEL/Sources/Parallels.pri)
include($$LEVEL/Sources/Build/Debug.pri)
include($$LEVEL/Sources/XmlModel/XmlModel.pri)
include($$LEVEL/Sources/Dispatcher/CAuth/CAuth/CAuth.pri)
include($$LEVEL/Sources/Libraries/CComm/pvsCComm/pvsCComm.pri)

INCLUDEPATH += ./../../

#LIBS += -lprl_xml_model -lprl_local_disp -lprl_local_recv -lwscommon -lpvsCComm -lStd
# LIBS += -lInterfaces
LIBS += -lqtservice

include($$LEVEL/Sources/Libraries/Logging/Logging.pri)

HEADERS +=

SOURCES += Main.cpp

macx {
	PLATFORM = MacOS
	DEFINES += _MACOS_
}

win32 {
#   QMAKE_LIBDIR += ../../AuxLibs/IPHlpApi
#   LIBS += -lUserenv -lWinspool -lWinmm -lSetupapi -lIPHlpApi -lNetapi32 -lAdvapi32
	LIBS += -lAdvapi32 -lUserenv -lNetapi32
}


contains( DEFINES, __WS_SERVER__) {

	unix {
	    # Unix users, if your SSL libraries or include files are not
	    # found, insert your paths here.
	    exists(/usr/local/include):INCLUDEPATH += /usr/local/include
	    exists(/usr/local/ssl/include):INCLUDEPATH += /usr/local/ssl/include
	    exists(/usr/local/openssl/include):INCLUDEPATH += /usr/local/openssl/include
	    exists(/opt/ssl/include):INCLUDEPATH += /opt/ssl/include
	    exists(/opt/openssl/include):INCLUDEPATH += /opt/openssl/include
	    exists(/usr/local/lib):LIBS += -L/usr/local/lib
	    exists(/usr/local/ssl/lib):LIBS += -L/usr/local/ssl/lib
	    exists(/usr/local/openssl/lib):LIBS += -L/usr/local/openssl/lib
	    exists(/opt/ssl/lib):LIBS += -L/opt/ssl/lib
	    exists(/opt/openssl/lib):LIBS += -L/opt/openssl/lib

	    LIBS += -lssl -lcrypto
	}

	win32 {
		LIBS += ssleay32.lib libeay32.lib
	}

}

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$LEVEL/Sources/Tests/UnitTests.rc
