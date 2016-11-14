
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

TARGET = test-svcore-base

OBJECTS_DIR = o
MOC_DIR = o

include(svcore/base/test/files.pri)

for (file, TEST_SOURCES) { SOURCES += $$sprintf("svcore/base/test/%1", $$file) }
for (file, TEST_HEADERS) { HEADERS += $$sprintf("svcore/base/test/%1", $$file) }

