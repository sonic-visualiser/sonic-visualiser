
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-hostsdk rubberband fftw3 fftw3f samplerate jack libpulse portaudio-2.0 mad id3tag oggz fishsound lrdf redland rasqal raptor sndfile liblo

load(../prf/sv.prf)

CONFIG += sv qt thread warn_on stl rtti exceptions
QT += xml network

TARGET = "Sonic Visualiser"
linux*:TARGET = sonic-visualiser
solaris*:TARGET = sonic-visualiser

ICON = icons/sv-macicon.icns
RC_FILE = icons/sv.rc

DEPENDPATH += . .. i18n main transform
INCLUDEPATH += . .. transform main
LIBPATH = ../view ../layer ../data ../widgets ../transform ../plugin ../base ../system ../framework ../audioio ../rdf $$LIBPATH

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -lsvframework -lsvaudioio -lsvview -lsvlayer -lsvrdf -lsvtransform -lsvrdf -lsvtransform -lsvwidgets -lsvdata -lsvplugin -lsvbase -lsvsystem $$LIBS

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
           main/PreferencesDialog.h \
           main/Surveyer.h
SOURCES += main/main.cpp \
           main/OSCHandler.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp \
           main/Surveyer.cpp
RESOURCES += sonic-visualiser.qrc


