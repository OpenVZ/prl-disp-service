LIBTARGET = VmFileList
PROJ_FILE = $$PWD/VmFileList.pro
QTCONFIG = core xml network
!include(../../Build/qmake/staticlib.pri): error(include error)

# This must be some unique private name, because it must not change
# variables of file that had included this one.
#__SRC_LEVEL	=  $$PWD/../..


INCLUDEPATH += $$PWD
