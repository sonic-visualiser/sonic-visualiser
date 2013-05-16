
include(./config.pri)

CONFIG += staticlib

DEFINES -= USE_REDLAND
QMAKE_CXXFLAGS -= -I/usr/include/rasqal -I/usr/include/raptor2
EXTRALIBS -= -lrdf

DEFINES += USE_SORD
# Libraries and paths should be added by config.pri
