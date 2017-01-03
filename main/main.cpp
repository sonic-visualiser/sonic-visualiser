/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MainWindow.h"
#include "SVSplash.h"

#include "system/System.h"
#include "system/Init.h"
#include "base/TempDirectory.h"
#include "base/PropertyContainer.h"
#include "base/Preferences.h"
#include "data/fileio/FileSource.h"
#include "widgets/TipDialog.h"
#include "widgets/InteractiveFileFinder.h"
#include "svapp/framework/TransformUserConfigurator.h"
#include "transform/TransformFactory.h"
#include "svcore/plugin/PluginScan.h"

#include <QMetaType>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QIcon>
#include <QSessionManager>
#include <QDir>
#include <QTimer>
#include <QPainter>
#include <QFileOpenEvent>

#include <iostream>
#include <signal.h>

#ifdef HAVE_FFTW3F
#include <fftw3.h>
#endif

/*! \mainpage Sonic Visualiser

\section interesting Summary of interesting classes

 - Data models: Model and subclasses, e.g. WaveFileModel

 - Graphical layers: Layer and subclasses, displayed on View and its
 subclass widgets.

 - Main window class, document class, and file parser: MainWindow,
 Document, SVFileReader

 - Turning one model (e.g. audio) into another (e.g. more audio, or a
 curve extracted from it): Transform, encapsulating the data that need
 to be stored to be able to reproduce a given transformation;
 TransformFactory, for discovering the available types of transform;
 ModelTransformerFactory, ModelTransformer and subclasses, providing
 the mechanisms for applying transforms to data models

 - Creating the plugins used by transforms: RealTimePluginFactory,
 FeatureExtractionPluginFactory.  See also the API documentation for
 Vamp feature extraction plugins at
 http://www.vamp-plugins.org/code-doc/.

 - File reading and writing code: AudioFileReader and subclasses,
 WavFileWriter, DataFileReader, SVFileReader

 - FFT calculation and cacheing: FFTModel, FFTDataServer

 - Widgets that show groups of editable properties: PropertyBox for
 layer properties (contained in a PropertyStack), PluginParameterBox
 for plugins (contained in a PluginParameterDialog)

 - Audio playback: AudioCallbackPlaySource and subclasses,
 AudioCallbackPlayTarget and subclasses, AudioGenerator

\section model Data sources: the Model hierarchy

   A Model is something containing, or knowing how to obtain, data.

   For example, WaveFileModel is a model that knows how to get data
   from an audio file; SparseTimeValueModel is a model containing
   editable "curve" data.

   Models typically subclass one of a number of abstract subclasses of
   Model.  For example, WaveFileModel subclasses DenseTimeValueModel,
   which describes an interface for models that have a value at each
   time point for a given sampling resolution.  (Note that
   WaveFileModel does not actually read the files itself: it uses
   AudioFileReader classes for that.  It just makes data from the
   files available in a Model.)  SparseTimeValueModel uses the
   SparseModel template class, which provides most of the
   implementation for models that contain a series of points of some
   sort -- also used by NoteModel, TextModel, and
   SparseOneDimensionalModel.

   Everything that goes on the screen originates from a model, via a
   layer (see below).  The models are contained in a Document object.
   There is no containment hierarchy or ordering of models in the
   document.  One model is the main model, which defines the sample
   rate for playback.

   A model may also be marked as a "derived" model, which means it was
   generated from another model using some transform (feature
   extraction or effect plugin, etc) -- the idea being that they can
   be re-generated using the same transform if a new source model is
   loaded.

\section layer Things that can display data: the Layer hierarchy

   A Layer is something that knows how to draw parts of a model onto a
   timeline.

   For example, WaveformLayer is a layer which draws waveforms, based
   on WaveFileModel; TimeValueLayer draws curves, based on
   SparseTimeValueModel; SpectrogramLayer draws spectrograms, based on
   WaveFileModel (via FFTModel).

   The most basic functions of a layer are: to draw itself onto a
   Pane, against a timeline on the x axis; and to permit user
   interaction.  If you were thinking of adding the capability to
   display a new sort of something, then you would want to add a new
   layer type.  (You may also need a new model type, depending on
   whether any existing model can capture the data you need.)
   Depending on the sort of data in question, there are various
   existing layers that might be appropriate to start from -- for
   example, a layer that displays images that the user has imported
   and associated with particular times might have something in common
   with the existing TextLayer which displays pieces of text that are
   associated with particular times.

   Although layers are visual objects, they are contained in the
   Document in Sonic Visualiser rather than being managed together
   with display widgets.  The Sonic Visualiser file format has
   separate data and layout sections, and the layers are defined in
   the data section and then referred to in the layout section which
   determines which layers may go on which panes (see Pane below).

   Once a layer class is defined, some basic data about it needs to be
   set up in the LayerFactory class, and then it will appear in the
   menus and so on on the main window.

\section view Widgets that are used to show layers: The View hierarchy

   A View is a widget that displays a stack of layers.  The most
   important subclass is Pane, the widget that is used to show most of
   the data in the main window of Sonic Visualiser.

   All a pane really does is contain a set of layers and get them to
   render themselves (one on top of the other, with the topmost layer
   being the one that is currently interacted with), cache the
   results, negotiate user interaction with them, and so on.  This is
   generally fiddly, if not especially interesting.  Panes are
   strictly layout objects and are not stored in the Document class;
   instead the MainWindow contains a PaneStack widget (the widget that
   takes up most of Sonic Visualiser's main window) which contains a
   set of panes stacked vertically.

   Another View subclass is Overview, which is the widget that
   contains that green waveform showing the entire file at the bottom
   of the window.

*/

static QMutex cleanupMutex;
static bool cleanedUp = false;

static void
signalHandler(int /* signal */)
{
    // Avoid this happening more than once across threads

    cerr << "signalHandler: cleaning up and exiting" << endl;
    cleanupMutex.lock();
    if (!cleanedUp) {
        TempDirectory::getInstance()->cleanup();
        cleanedUp = true;
    }
    cleanupMutex.unlock();
    exit(0);
}

class SVApplication : public QApplication
{
public:
    SVApplication(int &argc, char **argv) :
        QApplication(argc, argv),
        m_readyForFiles(false),
        m_filepathQueue(QStringList()),
        m_mainWindow(0)
    {
    }
    virtual ~SVApplication() { }

    void setMainWindow(MainWindow *mw) { m_mainWindow = mw; }
    void releaseMainWindow() { m_mainWindow = 0; }

    virtual void commitData(QSessionManager &manager) {
        if (!m_mainWindow) return;
        bool mayAskUser = manager.allowsInteraction();
        bool success = m_mainWindow->commitData(mayAskUser);
        manager.release();
        if (!success) manager.cancel();
    }

    void handleFilepathArgument(QString path, SVSplash *splash);

    bool m_readyForFiles;
    QStringList m_filepathQueue;

protected:
    MainWindow *m_mainWindow;
    bool event(QEvent *);
};

int
main(int argc, char **argv)
{
    svSystemSpecificInitialisation();

#ifdef Q_WS_X11
#if QT_VERSION >= 0x040500
//    QApplication::setGraphicsSystem("raster");
#endif
#endif

#ifdef Q_OS_MAC
    if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8) {
        // Fix for OS/X 10.9 font problem
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif

    SVApplication application(argc, argv);

    QStringList args = application.arguments();

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

#ifndef Q_OS_WIN32
    signal(SIGHUP,  signalHandler);
    signal(SIGQUIT, signalHandler);
#endif

    bool audioOutput = true;
    bool oscSupport = true;

    if (args.contains("--help") || args.contains("-h") || args.contains("-?")) {
        cerr << QApplication::tr(
            "\nSonic Visualiser is a program for viewing and exploring audio data\nfor semantic music analysis and annotation.\n\nUsage:\n\n  %1 [--no-audio] [--no-osc] [<file> ...]\n\n  --no-audio: Do not attempt to open an audio output device\n  --no-osc: Do not provide an Open Sound Control port for remote control\n  <file>: One or more Sonic Visualiser (.sv) and audio files may be provided.\n").arg(argv[0]) << endl;
        exit(2);
    }

    if (args.contains("--no-audio")) audioOutput = false;
    if (args.contains("--no-osc")) oscSupport = false;

    QApplication::setOrganizationName("sonic-visualiser");
    QApplication::setOrganizationDomain("sonicvisualiser.org");
    QApplication::setApplicationName(QApplication::tr("Sonic Visualiser"));

    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    SVSplash *splash = 0;

    QSettings settings;

    settings.beginGroup("Preferences");
    // Default to using Piper server; can change in preferences
    if (!settings.contains("run-vamp-plugins-in-process")) {
        settings.setValue("run-vamp-plugins-in-process", false);
    }
    settings.endGroup();

    settings.beginGroup("Preferences");
    if (settings.value("show-splash", true).toBool()) {
        splash = new SVSplash();
        splash->show();
        QTimer::singleShot(5000, splash, SLOT(hide()));
        application.processEvents();
    }
    settings.endGroup();

    settings.beginGroup("RDF");
    if (!settings.contains("rdf-indices")) {
        QStringList list;
        list << "http://www.vamp-plugins.org/rdf/plugins/index.txt";
        settings.setValue("rdf-indices", list);
    }
    settings.endGroup();

    QIcon icon;
    int sizes[] = { 16, 22, 24, 32, 48, 64, 128 };
    for (int i = 0; i < int(sizeof(sizes)/sizeof(sizes[0])); ++i) {
        icon.addFile(QString(":icons/sv-%1x%2.png").arg(sizes[i]).arg(sizes[i]));
    }
    QApplication::setWindowIcon(icon);

    QString language = QLocale::system().name();
    SVDEBUG << "System language is: " << language << endl;

    settings.beginGroup("Preferences");
    QString prefLanguage = settings.value("locale", language).toString();
    if (prefLanguage != QString()) language = prefLanguage;
    settings.endGroup();

    QTranslator qtTranslator;
    QString qtTrName = QString("qt_%1").arg(language);
    SVDEBUG << "Loading " << qtTrName << "... ";
    bool success = false;
    if (!(success = qtTranslator.load(qtTrName))) {
        QString qtDir = getenv("QTDIR");
        if (qtDir != "") {
            success = qtTranslator.load
                (qtTrName, QDir(qtDir).filePath("translations"));
        }
    }
    if (!success) {
        SVDEBUG << "Failed\nFailed to load Qt translation for locale" << endl;
    } else {
        cerr << "Done" << endl;
    }
    application.installTranslator(&qtTranslator);

    QTranslator svTranslator;
    QString svTrName = QString("sonic-visualiser_%1").arg(language);
    SVDEBUG << "Loading " << svTrName << "... ";
    svTranslator.load(svTrName, ":i18n");
    SVDEBUG << "Done" << endl;
    application.installTranslator(&svTranslator);

    StoreStartupLocale();

    // Make known-plugins query as early as possible after showing
    // splash screen.
    PluginScan::getInstance()->scan();
    
    // Permit size_t and PropertyName to be used as args in queued signal calls
    qRegisterMetaType<PropertyContainer::PropertyName>("PropertyContainer::PropertyName");

    MainWindow::SoundOptions options = MainWindow::WithEverything;
    if (!audioOutput) options = 0;
    
    MainWindow *gui = new MainWindow(options, oscSupport);
    application.setMainWindow(gui);
    InteractiveFileFinder::setParentWidget(gui);
    TransformUserConfigurator::setParentWidget(gui);
    if (splash) {
        QObject::connect(gui, SIGNAL(hideSplash()), splash, SLOT(hide()));
        QObject::connect(gui, SIGNAL(hideSplash(QWidget *)),
                         splash, SLOT(finishSplash(QWidget *)));
    }

    QDesktopWidget *desktop = QApplication::desktop();
    QRect available = desktop->availableGeometry();

    int width = (available.width() * 2) / 3;
    int height = available.height() / 2;
    if (height < 450) height = (available.height() * 2) / 3;
    if (width > height * 2) width = height * 2;

    settings.beginGroup("MainWindow");

    QSize size = settings.value("size", QSize(width, height)).toSize();
    gui->resizeConstrained(size);

    if (settings.contains("position")) {
        QRect prevrect(settings.value("position").toPoint(), size);
        if (!(available & prevrect).isEmpty()) {
            gui->move(prevrect.topLeft());
        }
    }

    if (settings.value("maximised", false).toBool()) {
        gui->setWindowState(Qt::WindowMaximized);
    }

    settings.endGroup();
    
    gui->show();

    // The MainWindow class seems to have trouble dealing with this if
    // it tries to adapt to this preference before the constructor is
    // complete.  As a lazy hack, apply it explicitly from here
    gui->preferenceChanged("Property Box Layout");

    application.m_readyForFiles = true; // Ready to receive files from e.g. Apple Events

    for (QStringList::iterator i = args.begin(); i != args.end(); ++i) {

        if (i == args.begin()) continue;
        if (i->startsWith('-')) continue;

        QString path = *i;

        application.handleFilepathArgument(path, splash);
    }
    
    for (QStringList::iterator i = application.m_filepathQueue.begin(); i != application.m_filepathQueue.end(); ++i) {
        QString path = *i;
        application.handleFilepathArgument(path, splash);
    }
    
#ifdef HAVE_FFTW3F
    settings.beginGroup("FFTWisdom");
    QString wisdom = settings.value("wisdom").toString();
    if (wisdom != "") {
        fftwf_import_wisdom_from_string(wisdom.toLocal8Bit().data());
    }
#ifdef HAVE_FFTW3
    wisdom = settings.value("wisdom_d").toString();
    if (wisdom != "") {
        fftw_import_wisdom_from_string(wisdom.toLocal8Bit().data());
    }
#endif
    settings.endGroup();
#endif

    int rv = application.exec();

    gui->hide();

    cleanupMutex.lock();

    if (!cleanedUp) {
        TransformFactory::deleteInstance();
        TempDirectory::getInstance()->cleanup();
        cleanedUp = true;
    }

    application.releaseMainWindow();

#ifdef HAVE_FFTW3F
    settings.beginGroup("FFTWisdom");
    char *cwisdom = fftwf_export_wisdom_to_string();
    if (cwisdom) {
        settings.setValue("wisdom", cwisdom);
        free(cwisdom);
    }
#ifdef HAVE_FFTW3
    cwisdom = fftw_export_wisdom_to_string();
    if (cwisdom) {
        settings.setValue("wisdom_d", cwisdom);
        free(cwisdom);
    }
#endif
    settings.endGroup();
#endif

    FileSource::debugReport();
    
    delete gui;

    cleanupMutex.unlock();

    return rv;
}

bool SVApplication::event(QEvent *event){

// Avoid warnings/errors with -Wextra because we aren't explicitly
// handling all event types (-Wall is OK with this because of the
// default but the stricter level insists)
#pragma GCC diagnostic ignored "-Wswitch-enum"

    QString thePath;

    switch (event->type()) {
    case QEvent::FileOpen:
        thePath = static_cast<QFileOpenEvent *>(event)->file();
        if(m_readyForFiles)
            handleFilepathArgument(thePath, NULL);
        else
            m_filepathQueue.append(thePath);
        return true;
    default:
        return QApplication::event(event);
    }
}

/** Application-global handler for filepaths passed in, e.g. as command-line arguments or apple events */
void SVApplication::handleFilepathArgument(QString path, SVSplash *splash){
    static bool haveSession = false;
    static bool haveMainModel = false;
    static bool havePriorCommandLineModel = false;

    MainWindow::FileOpenStatus status = MainWindow::FileOpenFailed;

#ifdef Q_OS_WIN32
    path.replace("\\", "/");
#endif

    if (path.endsWith("sv")) {
        if (!haveSession) {
            status = m_mainWindow->openSessionPath(path);
            if (status == MainWindow::FileOpenSucceeded) {
                haveSession = true;
                haveMainModel = true;
            }
        } else {
            cerr << "WARNING: Ignoring additional session file argument \"" << path << "\"" << endl;
            status = MainWindow::FileOpenSucceeded;
        }
    }
    if (status != MainWindow::FileOpenSucceeded) {
        if (!haveMainModel) {
            status = m_mainWindow->openPath(path, MainWindow::ReplaceSession);
            if (status == MainWindow::FileOpenSucceeded) {
                haveMainModel = true;
            }
        } else {
            if (haveSession && !havePriorCommandLineModel) {
                status = m_mainWindow->openPath(path, MainWindow::AskUser);
                if (status == MainWindow::FileOpenSucceeded) {
                    havePriorCommandLineModel = true;
                }
            } else {
                status = m_mainWindow->openPath(path, MainWindow::CreateAdditionalModel);
            }
        }
    }
    if (status == MainWindow::FileOpenFailed) {
        if (splash) splash->hide();
        QMessageBox::critical
            (m_mainWindow, QMessageBox::tr("Failed to open file"),
             QMessageBox::tr("File or URL \"%1\" could not be opened").arg(path));
    } else if (status == MainWindow::FileOpenWrongMode) {
        if (splash) splash->hide();
        QMessageBox::critical
            (m_mainWindow, QMessageBox::tr("Failed to open file"),
             QMessageBox::tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
    }
}
