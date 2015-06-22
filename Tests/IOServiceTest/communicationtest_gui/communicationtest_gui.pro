
TEMPLATE = subdirs

LEVEL = ../../../..
include($$LEVEL/Build/Options.pri)

include($$PWD/Client/Client.deps)
include($$PWD/Server/Server.deps)
