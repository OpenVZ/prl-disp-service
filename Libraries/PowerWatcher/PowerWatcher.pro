TEMPLATE = lib
CONFIG   += staticlib

include(PowerWatcher.pri)

HEADERS += PowerWatcher.h

SOURCES += PowerWatcher.cpp

linux-* {
    SOURCES += PowerWatcher_lin.cpp
}

win32 {
    SOURCES += PowerWatcher_win.cpp
}

