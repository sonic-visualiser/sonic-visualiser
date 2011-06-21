
CONFIG += release

DEFINES +=  HAVE_BZ2 HAVE_FFTW3 HAVE_FFTW3F HAVE_SNDFILE HAVE_SAMPLERATE HAVE_VAMP HAVE_VAMPHOSTSDK HAVE_RUBBERBAND HAVE_RAPTOR HAVE_RASQAL HAVE_REDLAND HAVE_LIBLO HAVE_PORTAUDIO_2_0 HAVE_JACK HAVE_OGGZ HAVE_FISHSOUND HAVE_MAD HAVE_ID3TAG

QMAKE_CC = gcc
QMAKE_CXX = g++
QMAKE_LINK = g++

QMAKE_CFLAGS += -g -O2
QMAKE_CXXFLAGS += -g0 -O2 -Wall -pipe -DNDEBUG -DBUILD_RELEASE -DNO_TIMING -I/usr/local/include   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include/rasqal -I/usr/local/include   -I/usr/local/include -I/usr/local/include/rasqal   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include   -I/usr/local/include  

linux*:LIBS += -lasound

macx*:DEFINES += HAVE_QUICKTIME
macx*:LIBS += -framework QuickTime -framework CoreAudio -framework CoreMidi -framework AudioUnit -framework AudioToolbox -framework CoreFoundation -framework CoreServices

LIBS +=  -lbz2 -L/usr/local/lib -lfftw3 -lm   -L/usr/local/lib -lfftw3f -lm   -L/usr/local/lib -lsndfile   -L/usr/local/lib -lsamplerate   -lrubberband -L/usr/local/lib -lraptor   -L/usr/local/lib -lrasqal -lraptor   -L/usr/local/lib -lrdf -lrasqal -lraptor   -L/usr/local/lib -llo -lpthread   -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework Carbon -L/usr/local/lib -lportaudio   -framework CoreAudio -framework CoreServices -framework AudioUnit -L/usr/local/lib -ljack -lpthread   -L/usr/local/lib -loggz -logg   -L/usr/local/lib -lfishsound -lvorbisenc -lspeex -lvorbis -lm -logg   -lmad -lid3tag

