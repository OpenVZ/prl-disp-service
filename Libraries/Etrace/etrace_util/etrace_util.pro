TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
QT=core

include(etrace_util.deps)


SOURCES += etrace_util.cpp
SOURCES += $$SRC_LEVEL/Monitor/Source/Std/HackParam.cpp
