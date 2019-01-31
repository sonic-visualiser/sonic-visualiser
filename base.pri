
SV_INCLUDEPATH = \
        . \
	bqvec \
	bqvec/bqvec \
	bqfft \
	bqresample \
	bqaudioio \
	bqaudioio/bqaudioio \
	bqaudiostream \
	bqaudiostream/bqaudiostream \
	bqthingfactory \
	piper-vamp-cpp \
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

# Platform defines for RtMidi
linux*:   DEFINES += __LINUX_ALSASEQ__ __LINUX_ALSA__
macx*:    DEFINES += __MACOSX_CORE__
win*:     DEFINES += __WINDOWS_MM__
solaris*: DEFINES += __RTMIDI_DUMMY_ONLY__

# Defines for Dataquay
DEFINES += USE_SORD

CONFIG += qt thread warn_on stl rtti exceptions
