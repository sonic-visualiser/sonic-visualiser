
TEMPLATE = lib

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

CONFIG -= qt
CONFIG += plugin no_plugin_name_prefix release warn_on

TARGET = os_other

OBJECTS_DIR = o

win32-msvc* {
    LIBS += -EXPORT:OSReportsDarkThemeActive -EXPORT:OSQueryAccentColour
}

SOURCES += \
    svcore/system/os-other.cpp

