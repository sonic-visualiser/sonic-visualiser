
TEMPLATE = app

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {
    include(noconfig.pri)
}

CONFIG += qt thread warn_on stl rtti exceptions c++11
QT += network xml gui widgets svg

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

TRANSLATIONS += \
        i18n/sonic-visualiser_ru.ts \
	i18n/sonic-visualiser_en_GB.ts \
	i18n/sonic-visualiser_en_US.ts \
	i18n/sonic-visualiser_cs_CZ.ts

# Platform defines for RtMidi
linux*:   DEFINES += __LINUX_ALSASEQ__
macx*:    DEFINES += __MACOSX_CORE__
win*:     DEFINES += __WINDOWS_MM__
solaris*: DEFINES += __RTMIDI_DUMMY_ONLY__

# Defines for Dataquay
DEFINES += USE_SORD

OBJECTS_DIR = o
MOC_DIR = o

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

RESOURCES += sonic-visualiser.qrc

# Mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

SV_INCLUDEPATH = \
        . \
	bqvec \
	bqvec/bqvec \
	bqresample \
	bqaudioio \
	bqaudioio/bqaudioio \
	piper-cpp \
	checker \
	checker/checker \
	dataquay \
	dataquay/dataquay \
	svcore \
	svcore/data \
	svcore/plugin/api/alsa \
	svgui \
	svapp \
	vamp-plugin-sdk

DEPENDPATH += $$SV_INCLUDEPATH
INCLUDEPATH += $$SV_INCLUDEPATH

include(bq-files.pri)
include(vamp-plugin-sdk-files.pri)
include(svcore/files.pri)
include(svgui/files.pri)
include(svapp/files.pri)

DATAQUAY_SOURCES=$$fromfile(dataquay/lib.pro, SOURCES)
DATAQUAY_HEADERS=$$fromfile(dataquay/lib.pro, HEADERS)

CHECKER_SOURCES=$$fromfile(checker/checker.pri, SOURCES)
CHECKER_HEADERS=$$fromfile(checker/checker.pri, HEADERS)

CLIENT_HEADERS=$$fromfile(piper-cpp/vamp-client/client.pro, HEADERS)

for (file, BQ_SOURCES)       { SOURCES += $$file }
for (file, BQ_HEADERS)       { HEADERS += $$file }

for (file, VAMP_SOURCES)     { SOURCES += $$file }
for (file, VAMP_HEADERS)     { HEADERS += $$file }

for (file, SVCORE_SOURCES)   { SOURCES += $$sprintf("svcore/%1",   $$file) }
for (file, SVGUI_SOURCES)    { SOURCES += $$sprintf("svgui/%1",    $$file) }
for (file, SVAPP_SOURCES)    { SOURCES += $$sprintf("svapp/%1",    $$file) }
for (file, DATAQUAY_SOURCES) { SOURCES += $$sprintf("dataquay/%1", $$file) }
for (file, CHECKER_SOURCES)  { SOURCES += $$sprintf("checker/%1",  $$file) }

for (file, SVCORE_HEADERS)   { HEADERS += $$sprintf("svcore/%1",   $$file) }
for (file, SVGUI_HEADERS)    { HEADERS += $$sprintf("svgui/%1",    $$file) }
for (file, SVAPP_HEADERS)    { HEADERS += $$sprintf("svapp/%1",    $$file) }
for (file, DATAQUAY_HEADERS) { HEADERS += $$sprintf("dataquay/%1", $$file) }
for (file, CHECKER_HEADERS)  { HEADERS += $$sprintf("checker/%1",  $$file) }

for (file, CLIENT_HEADERS) {
    HEADERS += $$sprintf("piper-cpp/vamp-client/%1",  $$file)
}

SOURCES += piper-cpp/vamp-capnp/piper-capnp.cpp

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

