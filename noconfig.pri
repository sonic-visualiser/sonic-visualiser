
CONFIG += release
DEFINES += NDEBUG BUILD_RELEASE
DEFINES += NO_TIMING

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



