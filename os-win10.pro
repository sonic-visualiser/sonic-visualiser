
TEMPLATE = lib

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

CONFIG -= qt
CONFIG += plugin no_plugin_name_prefix release warn_on

TARGET = os

OBJECTS_DIR = o

win32-msvc* {
    LIBS += -EXPORT:OSReportsDarkThemeActive -EXPORT:OSQueryAccentColour
    LIBS += -lWindowsApp
}

SOURCES += \
    svcore/system/os-win10.cpp

