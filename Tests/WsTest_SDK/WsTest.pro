TARGET = test_ws_sdk
TEMPLATE = app
CONFIG += console qt thread
QT += network core xml

LEVEL = ../../..

include($$LEVEL/Sources/Parallels.pri)
include($$LEVEL/Build/Debug.pri)
include($$LEVEL/Sources/Libraries/CComm/pvsCComm/pvsCComm.pri)
include(../../XmlModel/XmlModel.pri)


INCLUDEPATH +=	../../XmlModel/Messaging \
		../../XmlModel/VmConfig \
		../../XmlModel/HostHardwareInfo \
		../../XmlModel/DispConfig \
		../../XmlModel/ProblemReport \
		../../XmlModel/ParallelsObjects \
		../../GUI/Server \
		../../ \
		../../Interfaces \
		../../Interfaces/Private \
		../../Vm


QMAKE_LIBDIR += ../../Libraries

HEADERS	+=	CWsTest.h
FORMS	+=	WsTestDialog.ui
SOURCES +=	CWsTest.cpp

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

LIBS += -lprl_sdk -lStd
include($$LEVEL/Sources/Libraries/Logging/Logging.pri)

QT += gui
