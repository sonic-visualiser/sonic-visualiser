
include(./config.pri)

CONFIG += staticlib

DEFINES -= USE_REDLAND
QMAKE_CXXFLAGS -= -I/usr/include/rasqal -I/usr/include/raptor2
EXTRALIBS -= -lrdf

DEFINES += USE_SORD
QMAKE_CXXFLAGS += -I/usr/local/include/sord-0 -I/usr/local/include/serd-0 -I/usr/include/sord-0 -I/usr/include/serd-0
EXTRALIBS += -lsord-0 -lserd-0

