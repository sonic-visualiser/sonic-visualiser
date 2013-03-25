
TEMPLATE = app

include(config.pri)

CONFIG += qt thread warn_on stl rtti exceptions
QT += network xml gui widgets

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

DEPENDPATH += . svcore svgui svapp
INCLUDEPATH += . svcore svgui svapp

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += sv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    INCLUDEPATH += sv-dependency-builds/win32-msvc/include
    LIBS += sv-dependency-builds/win32-msvc/lib
}

TRANSLATIONS += i18n/sonic-visualiser_ru.ts i18n/sonic-visualiser_en_GB.ts i18n/sonic-visualiser_en_US.ts i18n/sonic-visualiser_cs_CZ.ts

OBJECTS_DIR = o
MOC_DIR = o

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

MY_LIBS = -Lsvapp -Lsvgui -Lsvcore -Ldataquay -lsvapp -lsvgui -lsvcore -ldataquay

linux* {
MY_LIBS = -Wl,-Bstatic $$MY_LIBS -Wl,-Bdynamic
}

win* {
MY_LIBS = -Lsvapp/debug -Lsvgui/debug -Lsvcore/debug -Ldataquay/debug $$MY_LIBS
}

LIBS = $$MY_LIBS $$LIBS

win* {
#PRE_TARGETDEPS += svapp/svapp.lib \
#                  svgui/svgui.lib \
#                  svcore/svcore.lib \
#                  dataquay/dataquay.lib
}
!win* {
PRE_TARGETDEPS += svapp/libsvapp.a \
                  svgui/libsvgui.a \
                  svcore/libsvcore.a \
                  dataquay/libdataquay.a
}

RESOURCES += sonic-visualiser.qrc

HEADERS += main/MainWindow.h \
           main/PreferencesDialog.h
SOURCES += main/main.cpp \
           main/OSCHandler.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp 

# for mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

