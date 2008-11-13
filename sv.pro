
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-hostsdk rubberband fftw3 fftw3f samplerate jack libpulse portaudio-2.0 mad id3tag oggz fishsound lrdf redland rasqal raptor sndfile liblo

load(../sv.prf)

CONFIG += sv qt thread warn_on stl rtti exceptions
QT += xml network

TARGET = sonic-visualiser

ICON = icons/sv-macicon.icns

DEPENDPATH += . .. i18n main transform
INCLUDEPATH += . .. transform main
LIBPATH = ../view ../layer ../data ../widgets ../transform ../plugin ../base ../system ../framework ../audioio ../rdf $$LIBPATH

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -lsvframework -lsvaudioio -lsvview -lsvlayer -lsvrdf -lsvtransform -lsvrdf -lsvwidgets -lsvdata -lsvplugin -lsvbase -lsvsystem $$LIBS

PRE_TARGETDEPS += ../view/libsvview.a \
                  ../layer/libsvlayer.a \
                  ../data/libsvdata.a \
                  ../framework/libsvframework.a \
                  ../transform/libsvtransform.a \
                  ../widgets/libsvwidgets.a \
                  ../plugin/libsvplugin.a \
                  ../base/libsvbase.a \
                  ../audioio/libsvaudioio.a \
                  ../rdf/libsvrdf.a \
                  ../system/libsvsystem.a

OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += main/MainWindow.h \
           main/PreferencesDialog.h
SOURCES += main/main.cpp \
           main/OSCHandler.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp
RESOURCES += sonic-visualiser.qrc


