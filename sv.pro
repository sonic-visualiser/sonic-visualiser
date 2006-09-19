
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-sdk fftw3f samplerate jack portaudio mad oggz fishsound lrdf raptor sndfile
load(../sv.prf)

CONFIG += sv qt thread warn_on stl rtti exceptions
QT += xml

TARGET = sonic-visualiser

DEPENDPATH += . .. audioio document i18n main transform
INCLUDEPATH += . .. audioio document transform main
LIBPATH = ../view ../layer ../data ../widgets ../plugin ../base ../system $$LIBPATH

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -lsvview -lsvlayer -lsvdata -lsvwidgets -lsvplugin -lsvbase -lsvsystem $$LIBS

PRE_TARGETDEPS += ../view/libsvview.a \
                  ../layer/libsvlayer.a \
                  ../data/libsvdata.a \
                  ../widgets/libsvwidgets.a \
                  ../plugin/libsvplugin.a \
                  ../base/libsvbase.a \
                  ../system/libsvsystem.a

OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += audioio/AudioCallbackPlaySource.h \
           audioio/AudioCallbackPlayTarget.h \
           audioio/AudioCoreAudioTarget.h \
           audioio/AudioGenerator.h \
           audioio/AudioJACKTarget.h \
           audioio/AudioPortAudioTarget.h \
           audioio/AudioTargetFactory.h \
           audioio/PhaseVocoderTimeStretcher.h \
           document/Document.h \
           document/SVFileReader.h \
           main/MainWindow.h \
           main/PreferencesDialog.h \
           transform/FeatureExtractionPluginTransform.h \
           transform/PluginTransform.h \
           transform/RealTimePluginTransform.h \
           transform/Transform.h \
           transform/TransformFactory.h
SOURCES += audioio/AudioCallbackPlaySource.cpp \
           audioio/AudioCallbackPlayTarget.cpp \
           audioio/AudioCoreAudioTarget.cpp \
           audioio/AudioGenerator.cpp \
           audioio/AudioJACKTarget.cpp \
           audioio/AudioPortAudioTarget.cpp \
           audioio/AudioTargetFactory.cpp \
           audioio/PhaseVocoderTimeStretcher.cpp \
           document/Document.cpp \
           document/SVFileReader.cpp \
           main/main.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp \
           transform/FeatureExtractionPluginTransform.cpp \
           transform/PluginTransform.cpp \
           transform/RealTimePluginTransform.cpp \
           transform/Transform.cpp \
           transform/TransformFactory.cpp
RESOURCES += sonic-visualiser.qrc
TRANSLATIONS += i18n/sonic-visualiser_ru.ts
