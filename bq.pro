
TEMPLATE = lib

win32-g++ {
    INCLUDEPATH += sv-dependency-builds/win32-mingw/include
    LIBS += -Lsv-dependency-builds/win32-mingw/lib
}
win32-msvc* {
    # We actually expect MSVC to be used only for 64-bit builds,
    # though the qmake spec is still called win32-msvc*
    INCLUDEPATH += sv-dependency-builds/win64-msvc/include
    LIBS += -Lrelease -Lsv-dependency-builds/win64-msvc/lib
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
    DEFINES += NDEBUG BUILD_RELEASE NO_TIMING

    linux* {
        DEFINES += HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_LIBSAMPLERATE HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO
        LIBS += -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lportaudio -lsamplerate -lz
    }

    win32-g++ {
        DEFINES += HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_LIBSAMPLERATE HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO
        LIBS += -lfftw3 -lfftw3f -lsndfile -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile -logg -lmad -lid3tag -lportaudio -lsamplerate -lz
        DEFINES += USE_OWN_ALIGNED_MALLOC _USE_MATH_DEFINES
        LIBS += -lwinmm -lws2_32
    }
    win32-msvc* {
        DEFINES += HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_LIBSAMPLERATE HAVE_MAD HAVE_ID3TAG HAVE_PORTAUDIO
        LIBS += -lfftw3 -lfftw3f -lportaudio
        DEFINES += USE_OWN_ALIGNED_MALLOC _USE_MATH_DEFINES
        LIBS += -lwinmm -lws2_32
    }

    macx* {
        DEFINES += HAVE_COREAUDIO MALLOC_IS_ALIGNED
        LIBS += -framework CoreAudio -framework CoreMidi -framework AudioUnit -framework AudioToolbox -framework CoreFoundation -framework CoreServices -framework Accelerate
    }
}

CONFIG += staticlib warn_on stl exceptions c++11
CONFIG -= qt

TARGET = bq

DEPENDPATH += bqvec bqresample bqaudioio bqvec/bqvec bqresample/bqresample bqaudioio/bqaudioio
INCLUDEPATH += bqvec bqresample bqaudioio bqvec/bqvec bqresample/bqresample bqaudioio/bqaudioio

OBJECTS_DIR = o

HEADERS += \
	bqvec/bqvec/Allocators.h \
	bqvec/bqvec/Barrier.h \
	bqvec/bqvec/ComplexTypes.h \
	bqvec/bqvec/Restrict.h \
	bqvec/bqvec/RingBuffer.h \
	bqvec/bqvec/VectorOpsComplex.h \
	bqvec/bqvec/VectorOps.h \
	bqvec/pommier/neon_mathfun.h \
	bqvec/pommier/sse_mathfun.h \
	bqresample/bqresample/Resampler.h \
	bqresample/speex/speex_resampler.h \
	bqaudioio/bqaudioio/ApplicationPlaybackSource.h \
	bqaudioio/bqaudioio/ApplicationRecordTarget.h \
	bqaudioio/bqaudioio/AudioFactory.h \
	bqaudioio/bqaudioio/SystemAudioIO.h \
	bqaudioio/bqaudioio/SystemPlaybackTarget.h \
	bqaudioio/bqaudioio/SystemRecordSource.h \
	bqaudioio/src/DynamicJACK.h \
	bqaudioio/src/JACKAudioIO.h \
	bqaudioio/src/JACKPlaybackTarget.h \
	bqaudioio/src/JACKRecordSource.h \
	bqaudioio/src/PortAudioIO.h \
	bqaudioio/src/PortAudioPlaybackTarget.h \
	bqaudioio/src/PortAudioRecordSource.h \
	bqaudioio/src/PulseAudioIO.h \
	bqaudioio/src/PulseAudioPlaybackTarget.h

SOURCES += \
	bqvec/src/Allocators.cpp \
	bqvec/src/Barrier.cpp \
	bqvec/src/VectorOpsComplex.cpp \
	bqresample/src/Resampler.cpp \
	bqaudioio/src/AudioFactory.cpp \
	bqaudioio/src/JACKAudioIO.cpp \
	bqaudioio/src/JACKPlaybackTarget.cpp \
	bqaudioio/src/JACKRecordSource.cpp \
	bqaudioio/src/PortAudioIO.cpp \
	bqaudioio/src/PortAudioPlaybackTarget.cpp \
	bqaudioio/src/PortAudioRecordSource.cpp \
	bqaudioio/src/PulseAudioIO.cpp \
	bqaudioio/src/PulseAudioPlaybackTarget.cpp \
	bqaudioio/src/SystemPlaybackTarget.cpp \
	bqaudioio/src/SystemRecordSource.cpp

