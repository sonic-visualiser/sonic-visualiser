
TEMPLATE = app

include(config.pri)

CONFIG += qt thread warn_on stl rtti exceptions
QT += network xml gui

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

DEPENDPATH += . ../svcore ../svgui ../svapp
INCLUDEPATH += . ../svcore ../svgui ../svapp

TRANSLATIONS += i18n/sonic-visualiser_ru.ts i18n/sonic-visualiser_en_GB.ts i18n/sonic-visualiser_en_US.ts i18n/sonic-visualiser_cs_CZ.ts

OBJECTS_DIR = o
MOC_DIR = o

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -L../svapp -L../svgui -L../svcore -lsvapp -lsvgui -lsvcore -lvamp-hostsdk $$LIBS

PRE_TARGETDEPS += ../svapp/libsvapp.a \
                  ../svgui/libsvgui.a \
                  ../svcore/libsvcore.a

RESOURCES += sonic-visualiser.qrc

HEADERS += main/MainWindow.h \
           main/PreferencesDialog.h \
           main/Surveyer.h
SOURCES += main/main.cpp \
           main/OSCHandler.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp \
           main/Surveyer.cpp

# for mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

