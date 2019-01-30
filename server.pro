
TEMPLATE = app

CONFIG += stl exceptions console warn_on
CONFIG -= qt

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)

    macx*: LIBS -= -framework CoreAudio -framework CoreMidi -framework AudioUnit -framework AudioToolbox -framework CoreFoundation -framework CoreServices -framework Accelerate -lbz2 -lz
}

# Can't support this flag with the JSON11 and basen modules as they stand
QMAKE_CXXFLAGS -= -Werror

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but this may be necessary when cross-compiling
win32-x-g++: QMAKE_LFLAGS += -Wl,-subsystem,console

macx*: CONFIG -= app_bundle

linux*: LIBS += -ldl

TARGET = piper-vamp-simple-server

OBJECTS_DIR = o
MOC_DIR = o

INCLUDEPATH += piper-vamp-cpp piper-vamp-cpp/ext vamp-plugin-sdk

include(vamp-plugin-sdk-files.pri)

for (file, VAMP_SOURCES) { SOURCES += $$file }
for (file, VAMP_HEADERS) { HEADERS += $$file }

HEADERS += \
        piper-vamp-cpp/vamp-capnp/piper.capnp.h \
        piper-vamp-cpp/vamp-capnp/VampnProto.h

SOURCES += \
        piper-vamp-cpp/vamp-capnp/piper-capnp.cpp \
        piper-vamp-cpp/ext/json11/json11.cpp \
        piper-vamp-cpp/vamp-server/simple-server.cpp
