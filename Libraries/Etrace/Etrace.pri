LIBTARGET = etrace
PROJ_FILE = $$PWD/Etrace.pro
QTCONFIG  = core
!include(../../Build/qmake/staticlib.pri): error(include error)
include($$LIBS_LEVEL/Logging/Logging.pri)
include($$LIBS_LEVEL/HostUtils/HostUtils.pri)

etrace_webrtc {
	include($$EXT_LEVEL/google-webrtc/google-webrtc.pri)
	DEFINES += "ETRACE_WEBRTC=1"
}
