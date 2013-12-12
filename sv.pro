
TEMPLATE = app

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += -Lsv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    INCLUDEPATH += sv-dependency-builds/win32-msvc/include
    LIBS += -Lsv-dependency-builds/win32-msvc/lib
}

exists(config.pri) {
    include(config.pri)
}

win* {
    !exists(config.pri) {
        DEFINES += HAVE_BZ2 HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_SAMPLERATE HAVE_VAMP HAVE_VAMPHOSTSDK HAVE_RUBBERBAND HAVE_DATAQUAY HAVE_LIBLO HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO_2_0
        LIBS += -lbz2 -lrubberband -lvamp-hostsdk -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lportaudio -lsamplerate -llo -lz -lsord-0 -lserd-0 -lwinmm -lws2_32
    }
}

CONFIG += qt thread warn_on stl rtti exceptions
QT += network xml gui widgets

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

DEPENDPATH += . svcore svgui svapp
INCLUDEPATH += . svcore svgui svapp

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
MY_LIBS = -Lsvapp/release -Lsvgui/release -Lsvcore/release -Ldataquay/release $$MY_LIBS
}

LIBS = $$MY_LIBS $$LIBS

win* {
PRE_TARGETDEPS += svapp/release/libsvapp.a \
                  svgui/release/libsvgui.a \
                  svcore/release/libsvcore.a \
                  dataquay/release/libdataquay.a
}
!win* {
PRE_TARGETDEPS += svapp/libsvapp.a \
                  svgui/libsvgui.a \
                  svcore/libsvcore.a \
                  dataquay/libdataquay.a
}

RESOURCES += sonic-visualiser.qrc

HEADERS += main/MainWindow.h \
           main/IMAFencoder.h \
           main/checkbox.h \
           main/NetworkPermissionTester.h \
           main/Surveyer.h \
           main/PreferencesDialog.h

SOURCES += main/main.cpp \
           main/OSCHandler.cpp \
           main/MainWindow.cpp \
           main/IMAFencoder.c \
           main/imafdecoder.cpp \
           main/NetworkPermissionTester.cpp \
           main/Surveyer.cpp \
           main/PreferencesDialog.cpp 

# for mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

