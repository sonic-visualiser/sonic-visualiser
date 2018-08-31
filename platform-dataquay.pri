
exists(config.pri) {
    include(./config.pri)
}

!exists(config.pri) {
    include(./noconfig.pri)
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
    # We actually expect MSVC to be used only for 64-bit builds,
    # though the qmake spec is still called win32-msvc*
    INCLUDEPATH += ../sv-dependency-builds/win64-msvc/include
    LIBS += -L../../sv-dependency-builds/win64-msvc/lib
}
mac* {
    INCLUDEPATH += ../sv-dependency-builds/osx/include
    LIBS += -L../sv-dependency-builds/osx/lib
}
