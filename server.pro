
TEMPLATE = app

CONFIG += stl c++11 exceptions console warn_on

CONFIG -= qt

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but this may be necessary when cross-compiling
win32-x-g++: QMAKE_LFLAGS += -Wl,-subsystem,console

macx*: CONFIG -= app_bundle

linux*: LIBS += -ldl

TARGET = piper-vamp-server

OBJECTS_DIR = o
MOC_DIR = o

INCLUDEPATH += piper-cpp vamp-plugin-sdk

include(vamp-plugin-sdk-files.pri)

for (file, VAMP_SOURCES) { SOURCES += $$file }
for (file, VAMP_HEADERS) { HEADERS += $$file }

HEADERS += \
        piper-cpp/vamp-capnp/piper.capnp.h \
        piper-cpp/vamp-capnp/VampnProto.h

SOURCES += \
        piper-cpp/vamp-capnp/piper-capnp.cpp \
        piper-cpp/vamp-server/server.cpp
