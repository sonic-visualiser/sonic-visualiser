
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

TARGET = test-svcore-system

OBJECTS_DIR = o
MOC_DIR = o

include(svcore/system/test/files.pri)

for (file, TEST_SOURCES) { SOURCES += $$sprintf("svcore/system/test/%1", $$file) }
for (file, TEST_HEADERS) { HEADERS += $$sprintf("svcore/system/test/%1", $$file) }

!win32* {
    POST_TARGETDEPS += $$PWD/libbase.a
    QMAKE_POST_LINK = ./$${TARGET}
}
