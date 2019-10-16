
TEMPLATE = app

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

include(base.pri)

QT += network xml gui widgets svg

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

!win32 {
    PRE_TARGETDEPS += $$PWD/libbase.a
}

linux* {

    sv_bins.path = $$PREFIX_PATH/bin/
    sv_bins.files = checker/vamp-plugin-load-checker piper-vamp-simple-server sonic-visualiser
    sv_bins.CONFIG = no_check_exist

    sv_desktop.path = $$PREFIX_PATH/share/applications/
    sv_desktop.files = sonic-visualiser.desktop
    sv_desktop.CONFIG = no_check_exist

    sv_icon.path = $$PREFIX_PATH/share/icons/hicolor/scalable/apps/
    sv_icon.files = icons/sonic-visualiser.svg
    sv_icon.CONFIG = no_check_exist
    
    INSTALLS += sv_bins sv_desktop sv_icon
}

TRANSLATIONS += \
        i18n/sonic-visualiser_ru.ts \
	i18n/sonic-visualiser_en_GB.ts \
	i18n/sonic-visualiser_en_US.ts \
	i18n/sonic-visualiser_cs_CZ.ts

OBJECTS_DIR = o
MOC_DIR = o

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

RESOURCES += sonic-visualiser.qrc

# Mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

include(svgui/files.pri)
include(svapp/files.pri)

for (file, SVGUI_SOURCES)    { SOURCES += $$sprintf("svgui/%1",    $$file) }
for (file, SVAPP_SOURCES)    { SOURCES += $$sprintf("svapp/%1",    $$file) }

for (file, SVGUI_HEADERS)    { HEADERS += $$sprintf("svgui/%1",    $$file) }
for (file, SVAPP_HEADERS)    { HEADERS += $$sprintf("svapp/%1",    $$file) }

HEADERS += \
        main/MainWindow.h \
        main/NetworkPermissionTester.h \
        main/Surveyer.h \
        main/SVSplash.h \
        main/PreferencesDialog.h

SOURCES +=  \
	main/main.cpp \
        main/OSCHandler.cpp \
        main/MainWindow.cpp \
        main/NetworkPermissionTester.cpp \
        main/Surveyer.cpp \
        main/SVSplash.cpp \
        main/PreferencesDialog.cpp 

macx* {
    QMAKE_POST_LINK += cp checker/vamp-plugin-load-checker . && deploy/osx/deploy.sh $$shell_quote(Sonic Visualiser)
}

linux {
    QMAKE_POST_LINK += cp checker/vamp-plugin-load-checker .
}
