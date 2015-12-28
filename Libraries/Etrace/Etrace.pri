LIBTARGET = etrace
PROJ_FILE = $$PWD/Etrace.pro
QTCONFIG  = core
!include(../../Build/qmake/staticlib.pri): error(include error)

etrace_webrtc {
	include($$EXT_LEVEL/google-webrtc/google-webrtc.pri)
	DEFINES += "ETRACE_WEBRTC=1"
}
