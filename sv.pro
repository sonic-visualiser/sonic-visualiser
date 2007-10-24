
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-hostsdk fftw3f samplerate jack portaudio mad id3tag oggz fishsound lrdf raptor sndfile liblo
load(../sv.prf)

CONFIG += sv qt thread warn_on stl rtti exceptions
QT += xml network

TARGET = sonic-visualiser

ICON = icons/sv-macicon.icns

DEPENDPATH += . .. i18n main transform
INCLUDEPATH += . .. transform main
LIBPATH = ../view ../layer ../data ../widgets ../plugin ../base ../system ../document ../audioio $$LIBPATH

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -lsvdocument -lsvaudioio -lsvview -lsvlayer -lsvdata -lsvwidgets -lsvplugin -lsvbase -lsvsystem $$LIBS

PRE_TARGETDEPS += ../view/libsvview.a \
                  ../layer/libsvlayer.a \
                  ../data/libsvdata.a \
                  ../document/libsvdocument.a \
                  ../widgets/libsvwidgets.a \
                  ../plugin/libsvplugin.a \
                  ../base/libsvbase.a \
                  ../audioio/libsvaudioio.a \
                  ../system/libsvsystem.a

OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += main/MainWindow.h \
           main/PreferencesDialog.h
SOURCES += main/main.cpp \
           main/MainWindow.cpp \
           main/PreferencesDialog.cpp
RESOURCES += sonic-visualiser.qrc


