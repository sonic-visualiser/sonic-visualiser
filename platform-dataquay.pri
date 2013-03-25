
exists(config.pri) {
    include(./config.pri)
}

CONFIG += staticlib

DEFINES -= USE_REDLAND
QMAKE_CXXFLAGS -= -I/usr/include/rasqal -I/usr/include/raptor2
EXTRALIBS -= -lrdf

DEFINES += USE_SORD
!win*: {
    QMAKE_CXXFLAGS += -I/usr/local/include/sord-0 -I/usr/local/include/serd-0
}
EXTRALIBS += -lsord-0 -lserd-0

win32-g++: {
    INCLUDEPATH += ../sv-dependency-builds/win32-mingw/include
    LIBS += -L../../sv-dependency-builds/win32-mingw/lib
}
