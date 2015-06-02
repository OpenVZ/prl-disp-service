LIBTARGET = PowerWatcher
PROJ_FILE = $$PWD/PowerWatcher.pro
QTCONFIG = core

!include(../../Build/qmake/staticlib.pri): error(include error)

include($$LIBS_LEVEL/Logging/Logging.pri)

win32: {
    LIBS += user32.lib
    LIBS += powrprof.lib
}
