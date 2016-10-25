
TEMPLATE = app

CONFIG += qt stl c++11 exceptions console warn_on
QT -= xml network gui widgets

exists(config.pri) {
    include(config.pri)
}

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += -Lrelease -Lsv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    # We actually expect MSVC to be used only for 64-bit builds,
    # though the qmake spec is still called win32-msvc*
    INCLUDEPATH += sv-dependency-builds/win64-msvc/include
# bah, this is happening even if not debug build
    CONFIG(debug) {
        LIBS += -NODEFAULTLIB:MSVCRT -Ldebug \
            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib/debug \
            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib
    }
#    CONFIG(release) {
#        LIBS += -Lrelease \
#            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib
#    }
}
mac* {
    INCLUDEPATH += sv-dependency-builds/osx/include
    LIBS += -Lsv-dependency-builds/osx/lib
}

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but this may be necessary when cross-compiling
win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console

!win32 {
    QMAKE_CXXFLAGS += -Werror
}

TARGET = piper-vamp-server

OBJECTS_DIR = o
MOC_DIR = o

INCLUDEPATH += piper-cpp vamp-plugin-sdk
LIBS += -lcapnp -lkj

HEADERS += \
        piper-cpp/vamp-capnp/piper.capnp.h \
        piper-cpp/vamp-capnp/VampnProto.h

SOURCES += \
        piper-cpp/vamp-capnp/piper-capnp.cpp \
        piper-cpp/vamp-server/server.cpp

SOURCES +=  \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginBufferingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginChannelAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginHostAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginInputDomainAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginLoader.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginSummarisingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginWrapper.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/RealTime.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp
