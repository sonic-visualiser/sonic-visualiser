
exists(config.pri) {
    include(./config.pri)
}

CONFIG += staticlib

DEFINES -= USE_REDLAND
QMAKE_CXXFLAGS -= -I/usr/include/rasqal -I/usr/include/raptor2
QMAKE_CXXFLAGS -= -Werror
EXTRALIBS -= -lrdf

DEFINES += USE_SORD
# Libraries and paths should be added by config.pri

win32-g++ {
    INCLUDEPATH += ../sv-dependency-builds/win32-mingw/include
    LIBS += -L../../sv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    INCLUDEPATH += ../sv-dependency-builds/win32-msvc/include
    LIBS += -L../../sv-dependency-builds/win32-msvc/lib
}
mac* {
    INCLUDEPATH += ../sv-dependency-builds/osx/include
    LIBS += -L../sv-dependency-builds/osx/lib
}
