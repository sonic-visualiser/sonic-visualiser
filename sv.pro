
TEMPLATE = app

INCLUDEPATH += vamp-plugin-sdk

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += -Lrelease -Lsv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    # We actually expect MSVC to be used only for 64-bit builds,
    # though the qmake spec is still called win32-msvc*
    INCLUDEPATH += sv-dependency-builds/win64-msvc/include
# bah, this is happening even if not debug build
#    CONFIG(debug) {
#        LIBS += -NODEFAULTLIB:MSVCRT -Ldebug \
#            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib/debug \
#            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib
#    }
    CONFIG(release) {
        LIBS += -Lrelease \
            -L../sonic-visualiser/sv-dependency-builds/win64-msvc/lib
    }
}
mac* {
    INCLUDEPATH += sv-dependency-builds/osx/include
    LIBS += -Lsv-dependency-builds/osx/lib
}

exists(config.pri) {
    include(config.pri)
}

!exists(config.pri) {

    CONFIG += release
    DEFINES += NDEBUG BUILD_RELEASE
    DEFINES += NO_TIMING

    DEFINES += HAVE_BZ2 HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_SAMPLERATE HAVE_VAMP HAVE_VAMPHOSTSDK HAVE_RUBBERBAND HAVE_DATAQUAY HAVE_LIBLO HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO

    LIBS += -lbz2 -lrubberband -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lportaudio -lsamplerate -lz -lsord-0 -lserd-0 -llo

    win* {
        DEFINES += NOMINMAX _USE_MATH_DEFINES
        DEFINES -= HAVE_LIBLO
        LIBS += -lwinmm -lws2_32
    }
    win32-msvc* {
        LIBS -= -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -lsord-0 -lserd-0 -llo
        LIBS += -lsord -lserd -ladvapi32
    }
    macx* {
        DEFINES += HAVE_COREAUDIO
        LIBS += -framework CoreAudio -framework CoreMidi -framework AudioUnit -framework AudioToolbox -framework CoreFoundation -framework CoreServices -framework Accelerate
    }
}

LIBS += -lcapnp -lkj ##!!!

CONFIG += qt thread warn_on stl rtti exceptions c++11
QT += network xml gui widgets svg

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

DEPENDPATH += . bqaudioio piper-cpp svcore svgui svapp
INCLUDEPATH += . bqaudioio piper-cpp svcore svgui svapp

TRANSLATIONS += i18n/sonic-visualiser_ru.ts i18n/sonic-visualiser_en_GB.ts i18n/sonic-visualiser_en_US.ts i18n/sonic-visualiser_cs_CZ.ts

OBJECTS_DIR = o
MOC_DIR = o

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

MY_LIBS = -Lsvapp -Lsvgui -Lsvcore -Lchecker -Ldataquay -L. \
          -lsvapp -lsvgui -lsvcore -lchecker -ldataquay -lbq

linux* {
MY_LIBS = -Wl,-Bstatic $$MY_LIBS -Wl,-Bdynamic
}

##??? how to set this appropriately for debug/release
win32*:CONFIG(debug) {
MY_LIBS = -Lsvapp/debug -Lsvgui/debug -Lsvcore/debug -Lchecker/debug -Ldataquay/debug $$MY_LIBS
}
win32*:CONFIG(release) {
MY_LIBS = -Lsvapp/release -Lsvgui/release -Lsvcore/release -Lchecker/release -Ldataquay/release $$MY_LIBS
}

LIBS = $$MY_LIBS $$LIBS

win32-g++* {
PRE_TARGETDEPS += svapp/release/libsvapp.a \
                  svgui/release/libsvgui.a \
                  svcore/release/libsvcore.a \
                  dataquay/release/libdataquay.a \
                  checker/release/libchecker.a
}
!win* {
PRE_TARGETDEPS += svapp/libsvapp.a \
                  svgui/libsvgui.a \
                  svcore/libsvcore.a \
                  dataquay/libdataquay.a \
                  checker/libchecker.a
}

win32-msvc*:CONFIG(debug) {
PRE_TARGETDEPS += svapp/debug/svapp.lib \
                  svgui/debug/svgui.lib \
                  svcore/debug/svcore.lib \
                  dataquay/debug/dataquay.lib \
                  checker/debug/checker.lib
}
win32-msvc*:CONFIG(release) {
PRE_TARGETDEPS += svapp/release/svapp.lib \
                  svgui/release/svgui.lib \
                  svcore/release/svcore.lib \
                  dataquay/release/dataquay.lib \
                  checker/release/checker.lib
}

RESOURCES += sonic-visualiser.qrc

HEADERS += \
        vamp-plugin-sdk/vamp-hostsdk/PluginBase.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginBufferingAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginChannelAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/Plugin.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginHostAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginInputDomainAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginLoader.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginSummarisingAdapter.h \
        vamp-plugin-sdk/vamp-hostsdk/PluginWrapper.h \
        vamp-plugin-sdk/vamp-hostsdk/RealTime.h \
        vamp-plugin-sdk/src/vamp-hostsdk/Window.h \
        main/MainWindow.h \
        main/NetworkPermissionTester.h \
        main/Surveyer.h \
        main/SVSplash.h \
        main/PreferencesDialog.h
SOURCES +=  \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginBufferingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginChannelAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginHostAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginInputDomainAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginLoader.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginSummarisingAdapter.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/PluginWrapper.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/RealTime.cpp \
        vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp \
        piper-cpp/vamp-capnp/piper-capnp.cpp \
	main/main.cpp \
        main/OSCHandler.cpp \
        main/MainWindow.cpp \
        main/NetworkPermissionTester.cpp \
        main/Surveyer.cpp \
        main/SVSplash.cpp \
        main/PreferencesDialog.cpp 

# for mac integration
QMAKE_INFO_PLIST = deploy/osx/Info.plist

