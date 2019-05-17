
TEMPLATE = app

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

include(base.pri)

CONFIG += console
QT += network xml testlib
QT -= gui

win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console
macx*: CONFIG -= app_bundle

TARGET = test-svcore-data-model

OBJECTS_DIR = o
MOC_DIR = o

include(svcore/data/model/test/files.pri)

for (file, TEST_SOURCES) { SOURCES += $$sprintf("svcore/data/model/test/%1", $$file) }
for (file, TEST_HEADERS) { HEADERS += $$sprintf("svcore/data/model/test/%1", $$file) }

!win32* {
    POST_TARGETDEPS += $$PWD/libbase.a
    QMAKE_POST_LINK = ./$${TARGET}
}
