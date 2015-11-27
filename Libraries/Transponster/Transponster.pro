TEMPLATE = lib
CONFIG += staticlib

include(Transponster.pri)

DEFINES += BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS BOOST_MPL_LIMIT_VECTOR_SIZE=40

HEADERS += \
	base.h \
	enum.h \
	domain_enum.h \
	domain_data.h \
	domain_type.h \
	iface_data.h \
	iface_enum.h \
	iface_type.h \
	network_enum.h \
	network_data.h \
	network_type.h \
	snapshot_enum.h \
	snapshot_data.h \
	snapshot_type.h \
	marshal.h \
	patterns.h \
	Direct.h \
	Direct_p.h \
	Reverse.h \
	Reverse_p.h \

SOURCES	+= \
	text.cpp \
	domain_data.cpp \
	domain_enum.cpp \
	domain_type.cpp \
	iface_data.cpp \
	iface_enum.cpp \
	iface_type.cpp \
	network_data.cpp \
	network_enum.cpp \
	network_type.cpp \
	snapshot_data.cpp \
	snapshot_enum.cpp \
	snapshot_type.cpp \
	Direct.cpp \
	Reverse.cpp
