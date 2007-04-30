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

#include "version.h"

#include "MainWindow.h"
#include "document/Document.h"
#include "PreferencesDialog.h"

#include "view/Pane.h"
#include "view/PaneStack.h"
#include "data/model/WaveFileModel.h"
#include "data/model/SparseOneDimensionalModel.h"
#include "view/ViewManager.h"
#include "base/Preferences.h"
#include "layer/WaveformLayer.h"
#include "layer/TimeRulerLayer.h"
#include "layer/TimeInstantLayer.h"
#include "layer/TimeValueLayer.h"
#include "layer/Colour3DPlotLayer.h"
#include "layer/SliceLayer.h"
#include "layer/SliceableLayer.h"
#include "widgets/Fader.h"
#include "view/Overview.h"
#include "widgets/PropertyBox.h"
#include "widgets/PropertyStack.h"
#include "widgets/AudioDial.h"
#include "widgets/LayerTree.h"
#include "widgets/ListInputDialog.h"
#include "widgets/SubdividingMenu.h"
#include "widgets/NotifyingPushButton.h"
#include "audioio/AudioCallbackPlaySource.h"
#include "audioio/AudioCallbackPlayTarget.h"
#include "audioio/AudioTargetFactory.h"
#include "audioio/PlaySpeedRangeMapper.h"
#include "data/fileio/AudioFileReaderFactory.h"
#include "data/fileio/DataFileReaderFactory.h"
#include "data/fileio/WavFileWriter.h"
#include "data/fileio/CSVFileWriter.h"
#include "data/fileio/BZipFileDevice.h"
#include "data/fileio/RemoteFile.h"
#include "data/fft/FFTDataServer.h"
#include "base/RecentFiles.h"
#include "transform/TransformFactory.h"
#include "base/PlayParameterRepository.h"
#include "base/XmlExportable.h"
#include "base/CommandHistory.h"
#include "base/Profiler.h"
#include "base/Clipboard.h"
#include "osc/OSCQueue.h"

// For version information
#include "vamp/vamp.h"
#include "vamp-sdk/PluginBase.h"
#include "plugin/api/ladspa.h"
#include "plugin/api/dssi.h"

#include <QApplication>
#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QStatusBar>
#include <QTreeView>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QShortcut>
#include <QSettings>
#include <QDateTime>
#include <QProcess>
#include <QCheckBox>
#include <QRegExp>

#include <iostream>
#include <cstdio>
#include <errno.h>

using std::cerr;
using std::endl;

using std::vector;
using std::map;
using std::set;


MainWindow::MainWindow(bool withAudioOutput, bool withOSCSupport) :
    m_document(0),
    m_paneStack(0),
    m_viewManager(0),
    m_overview(0),
    m_timeRulerLayer(0),
    m_audioOutput(withAudioOutput),
    m_playSource(0),
    m_playTarget(0),
    m_oscQueue(withOSCSupport ? new OSCQueue() : 0),
    m_recentFiles("RecentFiles", 20),
    m_recentTransforms("RecentTransforms", 20),
    m_mainMenusCreated(false),
    m_paneMenu(0),
    m_layerMenu(0),
    m_transformsMenu(0),
    m_existingLayersMenu(0),
    m_sliceMenu(0),
    m_recentFilesMenu(0),
    m_recentTransformsMenu(0),
    m_rightButtonMenu(0),
    m_rightButtonLayerMenu(0),
    m_rightButtonTransformsMenu(0),
    m_documentModified(false),
    m_openingAudioFile(false),
    m_abandoning(false),
    m_preferencesDialog(0)
{
    setWindowTitle(tr("Sonic Visualiser"));

    UnitDatabase::getInstance()->registerUnit("Hz");
    UnitDatabase::getInstance()->registerUnit("dB");

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
	    this, SLOT(documentModified()));
    connect(CommandHistory::getInstance(), SIGNAL(documentRestored()),
	    this, SLOT(documentRestored()));

    QFrame *frame = new QFrame;
    setCentralWidget(frame);

    QGridLayout *layout = new QGridLayout;
    
    m_viewManager = new ViewManager();
    connect(m_viewManager, SIGNAL(selectionChanged()),
	    this, SLOT(updateMenuStates()));
    connect(m_viewManager, SIGNAL(inProgressSelectionChanged()),
	    this, SLOT(inProgressSelectionChanged()));

    m_descriptionLabel = new QLabel;

    m_paneStack = new PaneStack(frame, m_viewManager);
    connect(m_paneStack, SIGNAL(currentPaneChanged(Pane *)),
	    this, SLOT(currentPaneChanged(Pane *)));
    connect(m_paneStack, SIGNAL(currentLayerChanged(Pane *, Layer *)),
	    this, SLOT(currentLayerChanged(Pane *, Layer *)));
    connect(m_paneStack, SIGNAL(rightButtonMenuRequested(Pane *, QPoint)),
            this, SLOT(rightButtonMenuRequested(Pane *, QPoint)));
    connect(m_paneStack, SIGNAL(propertyStacksResized()),
            this, SLOT(propertyStacksResized()));
    connect(m_paneStack, SIGNAL(contextHelpChanged(const QString &)),
            this, SLOT(contextHelpChanged(const QString &)));

    m_overview = new Overview(frame);
    m_overview->setViewManager(m_viewManager);
    m_overview->setFixedHeight(40);
    m_overview->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    connect(m_overview, SIGNAL(contextHelpChanged(const QString &)),
            this, SLOT(contextHelpChanged(const QString &)));

    m_panLayer = new WaveformLayer;
    m_panLayer->setChannelMode(WaveformLayer::MergeChannels);
//    m_panLayer->setScale(WaveformLayer::MeterScale);
//    m_panLayer->setAutoNormalize(true);
    m_panLayer->setBaseColour(Qt::darkGreen);
    m_panLayer->setAggressiveCacheing(true);
    m_overview->addLayer(m_panLayer);

    m_playSource = new AudioCallbackPlaySource(m_viewManager);

    connect(m_playSource, SIGNAL(sampleRateMismatch(size_t, size_t, bool)),
	    this,           SLOT(sampleRateMismatch(size_t, size_t, bool)));
    connect(m_playSource, SIGNAL(audioOverloadPluginDisabled()),
            this,           SLOT(audioOverloadPluginDisabled()));

    m_fader = new Fader(frame, false);
    connect(m_fader, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_fader, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    m_playSpeed = new AudioDial(frame);
    m_playSpeed->setMinimum(0);
    m_playSpeed->setMaximum(200);
    m_playSpeed->setValue(100);
    m_playSpeed->setFixedWidth(24);
    m_playSpeed->setFixedHeight(24);
    m_playSpeed->setNotchesVisible(true);
    m_playSpeed->setPageStep(10);
    m_playSpeed->setObjectName(tr("Playback Speedup"));
    m_playSpeed->setDefaultValue(100);
    m_playSpeed->setRangeMapper(new PlaySpeedRangeMapper(0, 200));
    m_playSpeed->setShowToolTip(true);
    connect(m_playSpeed, SIGNAL(valueChanged(int)),
	    this, SLOT(playSpeedChanged(int)));
    connect(m_playSpeed, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_playSpeed, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    m_playSharpen = new NotifyingPushButton(frame);
    m_playSharpen->setToolTip(tr("Sharpen percussive transients"));
    m_playSharpen->setFixedSize(20, 20);
//    m_playSharpen->setFlat(true);
    m_playSharpen->setEnabled(false);
    m_playSharpen->setCheckable(true);
    m_playSharpen->setChecked(false);
    m_playSharpen->setIcon(QIcon(":icons/sharpen.png"));
    connect(m_playSharpen, SIGNAL(clicked()), this, SLOT(playSharpenToggled()));
    connect(m_playSharpen, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_playSharpen, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    m_playMono = new NotifyingPushButton(frame);
    m_playMono->setToolTip(tr("Run time stretcher in mono only"));
    m_playMono->setFixedSize(20, 20);
//    m_playMono->setFlat(true);
    m_playMono->setEnabled(false);
    m_playMono->setCheckable(true);
    m_playMono->setChecked(false);
    m_playMono->setIcon(QIcon(":icons/mono.png"));
    connect(m_playMono, SIGNAL(clicked()), this, SLOT(playMonoToggled()));
    connect(m_playMono, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_playMono, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    QSettings settings;
    settings.beginGroup("MainWindow");
    m_playSharpen->setChecked(settings.value("playsharpen", true).toBool());
    m_playMono->setChecked(settings.value("playmono", false).toBool());
    settings.endGroup();

    layout->setSpacing(4);
    layout->addWidget(m_paneStack, 0, 0, 1, 5);
    layout->addWidget(m_overview, 1, 0);
    layout->addWidget(m_fader, 1, 1);
    layout->addWidget(m_playSpeed, 1, 2);
    layout->addWidget(m_playSharpen, 1, 3);
    layout->addWidget(m_playMono, 1, 4);

    m_paneStack->setPropertyStackMinWidth
        (m_fader->width() + m_playSpeed->width() + m_playSharpen->width() +
         m_playMono->width() + layout->spacing() * 4);

    layout->setColumnStretch(0, 10);

    frame->setLayout(layout);

    connect(m_viewManager, SIGNAL(outputLevelsChanged(float, float)),
	    this, SLOT(outputLevelsChanged(float, float)));

    connect(m_viewManager, SIGNAL(playbackFrameChanged(unsigned long)),
            this, SLOT(playbackFrameChanged(unsigned long)));

    connect(m_viewManager, SIGNAL(globalCentreFrameChanged(unsigned long)),
            this, SLOT(globalCentreFrameChanged(unsigned long)));

    connect(m_viewManager, SIGNAL(viewCentreFrameChanged(View *, unsigned long)),
            this, SLOT(viewCentreFrameChanged(View *, unsigned long)));

    connect(m_viewManager, SIGNAL(viewZoomLevelChanged(View *, unsigned long, bool)),
            this, SLOT(viewZoomLevelChanged(View *, unsigned long, bool)));

    connect(Preferences::getInstance(),
            SIGNAL(propertyChanged(PropertyContainer::PropertyName)),
            this,
            SLOT(preferenceChanged(PropertyContainer::PropertyName)));

//    preferenceChanged("Property Box Layout");

    if (m_oscQueue && m_oscQueue->isOK()) {
        connect(m_oscQueue, SIGNAL(messagesAvailable()), this, SLOT(pollOSC()));
        QTimer *oscTimer = new QTimer(this);
        connect(oscTimer, SIGNAL(timeout()), this, SLOT(pollOSC()));
        oscTimer->start(1000);
    }

    setupMenus();
    setupToolbars();

    statusBar();

    newSession();
}

MainWindow::~MainWindow()
{
//    std::cerr << "MainWindow::~MainWindow()" << std::endl;

    if (!m_abandoning) {
        closeSession();
    }
    delete m_playTarget;
    delete m_playSource;
    delete m_viewManager;
    delete m_oscQueue;
    Profiles::getInstance()->dump();
}

QString
MainWindow::getOpenFileName(FileFinder::FileType type)
{
    FileFinder *ff = FileFinder::getInstance();
    switch (type) {
    case FileFinder::SessionFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::AudioFile:
        return ff->getOpenFileName(type, m_audioFile);
    case FileFinder::LayerFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::SessionOrAudioFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::ImageFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::AnyFile:
        if (getMainModel() != 0 &&
            m_paneStack != 0 &&
            m_paneStack->getCurrentPane() != 0) { // can import a layer
            return ff->getOpenFileName(FileFinder::AnyFile, m_sessionFile);
        } else {
            return ff->getOpenFileName(FileFinder::SessionOrAudioFile,
                                       m_sessionFile);
        }
    }
    return "";
}

QString
MainWindow::getSaveFileName(FileFinder::FileType type)
{
    FileFinder *ff = FileFinder::getInstance();
    switch (type) {
    case FileFinder::SessionFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::AudioFile:
        return ff->getSaveFileName(type, m_audioFile);
    case FileFinder::LayerFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::SessionOrAudioFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::ImageFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::AnyFile:
        return ff->getSaveFileName(type, m_sessionFile);
    }
    return "";
}

void
MainWindow::registerLastOpenedFilePath(FileFinder::FileType type, QString path)
{
    FileFinder *ff = FileFinder::getInstance();
    ff->registerLastOpenedFilePath(type, path);
}

void
MainWindow::setupMenus()
{
    if (!m_mainMenusCreated) {
        m_rightButtonMenu = new QMenu();

        // No -- we don't want tear-off enabled on the right-button
        // menu.  If it is enabled, then simply right-clicking and
        // releasing will pop up the menu, activate the tear-off, and
        // leave the torn-off menu window in front of the main window.
        // That isn't desirable.  I'm not sure it ever would be, in a
        // context menu -- perhaps technically a Qt bug?
//        m_rightButtonMenu->setTearOffEnabled(true);
    }

    if (m_rightButtonLayerMenu) {
        m_rightButtonLayerMenu->clear();
    } else {
        m_rightButtonLayerMenu = m_rightButtonMenu->addMenu(tr("&Layer"));
        m_rightButtonLayerMenu->setTearOffEnabled(true);
        m_rightButtonMenu->addSeparator();
    }

    if (m_rightButtonTransformsMenu) {
        m_rightButtonTransformsMenu->clear();
    } else {
        m_rightButtonTransformsMenu = m_rightButtonMenu->addMenu(tr("&Transform"));
        m_rightButtonTransformsMenu->setTearOffEnabled(true);
        m_rightButtonMenu->addSeparator();
    }

    if (!m_mainMenusCreated) {
        CommandHistory::getInstance()->registerMenu(m_rightButtonMenu);
        m_rightButtonMenu->addSeparator();
    }

    setupFileMenu();
    setupEditMenu();
    setupViewMenu();
    setupPaneAndLayerMenus();
    setupTransformsMenu();
    setupHelpMenu();

    m_mainMenusCreated = true;
}

void
MainWindow::setupFileMenu()
{
    if (m_mainMenusCreated) return;

    QMenu *menu = menuBar()->addMenu(tr("&File"));
    menu->setTearOffEnabled(true);
    QToolBar *toolbar = addToolBar(tr("File Toolbar"));

    QIcon icon(":icons/filenew.png");
    icon.addFile(":icons/filenew-22.png");
    QAction *action = new QAction(icon, tr("&New Session"), this);
    action->setShortcut(tr("Ctrl+N"));
    action->setStatusTip(tr("Abandon the current Sonic Visualiser session and start a new one"));
    connect(action, SIGNAL(triggered()), this, SLOT(newSession()));
    menu->addAction(action);
    toolbar->addAction(action);

    icon = QIcon(":icons/fileopensession.png");
    action = new QAction(icon, tr("&Open Session..."), this);
    action->setShortcut(tr("Ctrl+O"));
    action->setStatusTip(tr("Open a previously saved Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(openSession()));
    menu->addAction(action);

    icon = QIcon(":icons/fileopen.png");
    icon.addFile(":icons/fileopen-22.png");

    action = new QAction(icon, tr("&Open..."), this);
    action->setStatusTip(tr("Open a session file, audio file, or layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(openSomething()));
    toolbar->addAction(action);

    icon = QIcon(":icons/filesave.png");
    icon.addFile(":icons/filesave-22.png");
    action = new QAction(icon, tr("&Save Session"), this);
    action->setShortcut(tr("Ctrl+S"));
    action->setStatusTip(tr("Save the current session into a Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(saveSession()));
    connect(this, SIGNAL(canSave(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    toolbar->addAction(action);
	
    icon = QIcon(":icons/filesaveas.png");
    icon.addFile(":icons/filesaveas-22.png");
    action = new QAction(icon, tr("Save Session &As..."), this);
    action->setStatusTip(tr("Save the current session into a new Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(saveSessionAs()));
    menu->addAction(action);
    toolbar->addAction(action);

    menu->addSeparator();

    icon = QIcon(":icons/fileopenaudio.png");
    action = new QAction(icon, tr("&Import Audio File..."), this);
    action->setShortcut(tr("Ctrl+I"));
    action->setStatusTip(tr("Import an existing audio file"));
    connect(action, SIGNAL(triggered()), this, SLOT(importAudio()));
    menu->addAction(action);

    action = new QAction(tr("Import Secondary Audio File..."), this);
    action->setShortcut(tr("Ctrl+Shift+I"));
    action->setStatusTip(tr("Import an extra audio file as a separate layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(importMoreAudio()));
    connect(this, SIGNAL(canImportMoreAudio(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(tr("&Export Audio File..."), this);
    action->setStatusTip(tr("Export selection as an audio file"));
    connect(action, SIGNAL(triggered()), this, SLOT(exportAudio()));
    connect(this, SIGNAL(canExportAudio(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("Import Annotation &Layer..."), this);
    action->setShortcut(tr("Ctrl+L"));
    action->setStatusTip(tr("Import layer data from an existing file"));
    connect(action, SIGNAL(triggered()), this, SLOT(importLayer()));
    connect(this, SIGNAL(canImportLayer(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(tr("Export Annotation Layer..."), this);
    action->setStatusTip(tr("Export layer data to a file"));
    connect(action, SIGNAL(triggered()), this, SLOT(exportLayer()));
    connect(this, SIGNAL(canExportLayer(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("Export Image File..."), this);
    action->setStatusTip(tr("Export a single pane to an image file"));
    connect(action, SIGNAL(triggered()), this, SLOT(exportImage()));
    connect(this, SIGNAL(canExportImage(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("Open Lo&cation..."), this);
    action->setShortcut(tr("Ctrl+Shift+O"));
    action->setStatusTip(tr("Open or import a file from a remote URL"));
    connect(action, SIGNAL(triggered()), this, SLOT(openLocation()));
    menu->addAction(action);

    menu->addSeparator();

    m_recentFilesMenu = menu->addMenu(tr("&Recent Files"));
    m_recentFilesMenu->setTearOffEnabled(true);
    setupRecentFilesMenu();
    connect(&m_recentFiles, SIGNAL(recentChanged()),
            this, SLOT(setupRecentFilesMenu()));

    menu->addSeparator();
    action = new QAction(tr("&Preferences..."), this);
    action->setStatusTip(tr("Adjust the application preferences"));
    connect(action, SIGNAL(triggered()), this, SLOT(preferences()));
    menu->addAction(action);
	
    /*!!!
      menu->addSeparator();
	
      action = new QAction(tr("Play / Pause"), this);
      action->setShortcut(tr("Space"));
      action->setStatusTip(tr("Start or stop playback from the current position"));
      connect(action, SIGNAL(triggered()), this, SLOT(play()));
      menu->addAction(action);
    */

    menu->addSeparator();
    action = new QAction(QIcon(":/icons/exit.png"),
                         tr("&Quit"), this);
    action->setShortcut(tr("Ctrl+Q"));
    action->setStatusTip(tr("Exit Sonic Visualiser"));
    connect(action, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(action);
}

void
MainWindow::setupEditMenu()
{
    if (m_mainMenusCreated) return;

    QMenu *menu = menuBar()->addMenu(tr("&Edit"));
    menu->setTearOffEnabled(true);
    CommandHistory::getInstance()->registerMenu(menu);

    menu->addSeparator();

    QAction *action = new QAction(QIcon(":/icons/editcut.png"),
                                  tr("Cu&t"), this);
    action->setShortcut(tr("Ctrl+X"));
    action->setStatusTip(tr("Cut the selection from the current layer to the clipboard"));
    connect(action, SIGNAL(triggered()), this, SLOT(cut()));
    connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    action = new QAction(QIcon(":/icons/editcopy.png"),
                         tr("&Copy"), this);
    action->setShortcut(tr("Ctrl+C"));
    action->setStatusTip(tr("Copy the selection from the current layer to the clipboard"));
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));
    connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    action = new QAction(QIcon(":/icons/editpaste.png"),
                         tr("&Paste"), this);
    action->setShortcut(tr("Ctrl+V"));
    action->setStatusTip(tr("Paste from the clipboard to the current layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(paste()));
    connect(this, SIGNAL(canPaste(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    action = new QAction(tr("&Delete Selected Items"), this);
    action->setShortcut(tr("Del"));
    action->setStatusTip(tr("Delete the selection from the current layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(deleteSelected()));
    connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    menu->addSeparator();
    m_rightButtonMenu->addSeparator();
	
    action = new QAction(tr("Select &All"), this);
    action->setShortcut(tr("Ctrl+A"));
    action->setStatusTip(tr("Select the whole duration of the current session"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);
	
    action = new QAction(tr("Select &Visible Range"), this);
    action->setShortcut(tr("Ctrl+Shift+A"));
    action->setStatusTip(tr("Select the time range corresponding to the current window width"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectVisible()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("Select to &Start"), this);
    action->setShortcut(tr("Shift+Left"));
    action->setStatusTip(tr("Select from the start of the session to the current playback position"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectToStart()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("Select to &End"), this);
    action->setShortcut(tr("Shift+Right"));
    action->setStatusTip(tr("Select from the current playback position to the end of the session"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectToEnd()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(tr("C&lear Selection"), this);
    action->setShortcut(tr("Esc"));
    action->setStatusTip(tr("Clear the selection"));
    connect(action, SIGNAL(triggered()), this, SLOT(clearSelection()));
    connect(this, SIGNAL(canClearSelection(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    menu->addSeparator();

    action = new QAction(tr("&Insert Instant at Playback Position"), this);
    action->setShortcut(tr("Enter"));
    action->setStatusTip(tr("Insert a new time instant at the current playback position, in a new layer if necessary"));
    connect(action, SIGNAL(triggered()), this, SLOT(insertInstant()));
    connect(this, SIGNAL(canInsertInstant(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(tr("Insert Instants at Selection &Boundaries"), this);
    action->setShortcut(tr("Shift+Enter"));
    action->setStatusTip(tr("Insert new time instants at the start and end of the current selection, in a new layer if necessary"));
    connect(action, SIGNAL(triggered()), this, SLOT(insertInstantsAtBoundaries()));
    connect(this, SIGNAL(canInsertInstantsAtBoundaries(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    // Laptop shortcut (no keypad Enter key)
    connect(new QShortcut(tr(";"), this), SIGNAL(activated()),
            this, SLOT(insertInstant()));
}

void
MainWindow::setupViewMenu()
{
    if (m_mainMenusCreated) return;

    QAction *action = 0;

    QMenu *menu = menuBar()->addMenu(tr("&View"));
    menu->setTearOffEnabled(true);
    action = new QAction(tr("Scroll &Left"), this);
    action->setShortcut(tr("Left"));
    action->setStatusTip(tr("Scroll the current pane to the left"));
    connect(action, SIGNAL(triggered()), this, SLOT(scrollLeft()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("Scroll &Right"), this);
    action->setShortcut(tr("Right"));
    action->setStatusTip(tr("Scroll the current pane to the right"));
    connect(action, SIGNAL(triggered()), this, SLOT(scrollRight()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("&Jump Left"), this);
    action->setShortcut(tr("Ctrl+Left"));
    action->setStatusTip(tr("Scroll the current pane a big step to the left"));
    connect(action, SIGNAL(triggered()), this, SLOT(jumpLeft()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("J&ump Right"), this);
    action->setShortcut(tr("Ctrl+Right"));
    action->setStatusTip(tr("Scroll the current pane a big step to the right"));
    connect(action, SIGNAL(triggered()), this, SLOT(jumpRight()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction(QIcon(":/icons/zoom-in.png"),
                         tr("Zoom &In"), this);
    action->setShortcut(tr("Up"));
    action->setStatusTip(tr("Increase the zoom level"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomIn()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(QIcon(":/icons/zoom-out.png"),
                         tr("Zoom &Out"), this);
    action->setShortcut(tr("Down"));
    action->setStatusTip(tr("Decrease the zoom level"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomOut()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
	
    action = new QAction(tr("Restore &Default Zoom"), this);
    action->setStatusTip(tr("Restore the zoom level to the default"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomDefault()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(QIcon(":/icons/zoom-fit.png"),
                         tr("Zoom to &Fit"), this);
    action->setStatusTip(tr("Zoom to show the whole file"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomToFit()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu->addSeparator();

    QActionGroup *overlayGroup = new QActionGroup(this);
        
    action = new QAction(tr("Show &No Overlays"), this);
    action->setShortcut(tr("0"));
    action->setStatusTip(tr("Hide centre indicator, frame times, layer names and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showNoOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &Minimal Overlays"), this);
    action->setShortcut(tr("9"));
    action->setStatusTip(tr("Show centre indicator only"));
    connect(action, SIGNAL(triggered()), this, SLOT(showMinimalOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &Standard Overlays"), this);
    action->setShortcut(tr("8"));
    action->setStatusTip(tr("Show centre indicator, frame times and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showStandardOverlays()));
    action->setCheckable(true);
    action->setChecked(true);
    overlayGroup->addAction(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &All Overlays"), this);
    action->setShortcut(tr("7"));
    action->setStatusTip(tr("Show all texts and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showAllOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    menu->addAction(action);
        
    menu->addSeparator();

    action = new QAction(tr("Show &Zoom Wheels"), this);
    action->setShortcut(tr("Z"));
    action->setStatusTip(tr("Show thumbwheels for zooming horizontally and vertically"));
    connect(action, SIGNAL(triggered()), this, SLOT(toggleZoomWheels()));
    action->setCheckable(true);
    action->setChecked(m_viewManager->getZoomWheelsEnabled());
    menu->addAction(action);
        
    action = new QAction(tr("Show Property Bo&xes"), this);
    action->setShortcut(tr("X"));
    action->setStatusTip(tr("Show the layer property boxes at the side of the main window"));
    connect(action, SIGNAL(triggered()), this, SLOT(togglePropertyBoxes()));
    action->setCheckable(true);
    action->setChecked(true);
    menu->addAction(action);

    action = new QAction(tr("Show Status &Bar"), this);
    action->setStatusTip(tr("Show context help information in the status bar at the bottom of the window"));
    connect(action, SIGNAL(triggered()), this, SLOT(toggleStatusBar()));
    action->setCheckable(true);
    action->setChecked(true);
    menu->addAction(action);

    QSettings settings;
    settings.beginGroup("MainWindow");
    bool sb = settings.value("showstatusbar", true).toBool();
    if (!sb) {
        action->setChecked(false);
        statusBar()->hide();
    }
    settings.endGroup();

/*!!! This one doesn't work properly yet

    menu->addSeparator();

    action = new QAction(tr("Show La&yer Hierarchy"), this);
    action->setShortcut(tr("Alt+L"));
    action->setStatusTip(tr("Open a window displaying the hierarchy of panes and layers in this session"));
    connect(action, SIGNAL(triggered()), this, SLOT(showLayerTree()));
    menu->addAction(action);
 */
}

void
MainWindow::setupPaneAndLayerMenus()
{
    if (m_paneMenu) {
	m_paneActions.clear();
	m_paneMenu->clear();
    } else {
	m_paneMenu = menuBar()->addMenu(tr("&Pane"));
        m_paneMenu->setTearOffEnabled(true);
    }

    if (m_layerMenu) {
	m_layerActions.clear();
	m_layerMenu->clear();
    } else {
	m_layerMenu = menuBar()->addMenu(tr("&Layer"));
        m_layerMenu->setTearOffEnabled(true);
    }

    QMenu *menu = m_paneMenu;

    QAction *action = new QAction(QIcon(":/icons/pane.png"), tr("Add &New Pane"), this);
    action->setShortcut(tr("Alt+N"));
    action->setStatusTip(tr("Add a new pane containing only a time ruler"));
    connect(action, SIGNAL(triggered()), this, SLOT(addPane()));
    connect(this, SIGNAL(canAddPane(bool)), action, SLOT(setEnabled(bool)));
    m_paneActions[action] = PaneConfiguration(LayerFactory::TimeRuler);
    menu->addAction(action);

    menu->addSeparator();

    menu = m_layerMenu;

//    menu->addSeparator();

    LayerFactory::LayerTypeSet emptyLayerTypes =
	LayerFactory::getInstance()->getValidEmptyLayerTypes();

    for (LayerFactory::LayerTypeSet::iterator i = emptyLayerTypes.begin();
	 i != emptyLayerTypes.end(); ++i) {
	
	QIcon icon;
	QString mainText, tipText, channelText;
	LayerFactory::LayerType type = *i;
	QString name = LayerFactory::getInstance()->getLayerPresentationName(type);
	
	icon = QIcon(QString(":/icons/%1.png")
		     .arg(LayerFactory::getInstance()->getLayerIconName(type)));

	mainText = tr("Add New %1 Layer").arg(name);
	tipText = tr("Add a new empty layer of type %1").arg(name);

	action = new QAction(icon, mainText, this);
	action->setStatusTip(tipText);

	if (type == LayerFactory::Text) {
	    action->setShortcut(tr("Alt+T"));
	}

	connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
	connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
	m_layerActions[action] = type;
	menu->addAction(action);
        m_rightButtonLayerMenu->addAction(action);
    }
    
    m_rightButtonLayerMenu->addSeparator();
    menu->addSeparator();

    LayerFactory::LayerType backgroundTypes[] = {
	LayerFactory::Waveform,
	LayerFactory::Spectrogram,
	LayerFactory::MelodicRangeSpectrogram,
	LayerFactory::PeakFrequencySpectrogram,
        LayerFactory::Spectrum
    };

    std::vector<Model *> models;
    if (m_document) models = m_document->getTransformInputModels(); //!!! not well named for this!
    bool plural = (models.size() > 1);
    if (models.empty()) {
        models.push_back(getMainModel()); // probably 0
    }

    for (unsigned int i = 0;
	 i < sizeof(backgroundTypes)/sizeof(backgroundTypes[0]); ++i) {

	for (int menuType = 0; menuType <= 1; ++menuType) { // pane, layer

	    if (menuType == 0) menu = m_paneMenu;
	    else menu = m_layerMenu;

	    QMenu *submenu = 0;

            QIcon icon;
            QString mainText, shortcutText, tipText, channelText;
            LayerFactory::LayerType type = backgroundTypes[i];
            bool mono = true;
            
            switch (type) {
                    
            case LayerFactory::Waveform:
                icon = QIcon(":/icons/waveform.png");
                mainText = tr("Add &Waveform");
                if (menuType == 0) {
                    shortcutText = tr("Alt+W");
                    tipText = tr("Add a new pane showing a waveform view");
                } else {
                    tipText = tr("Add a new layer showing a waveform view");
                }
                mono = false;
                break;
		
            case LayerFactory::Spectrogram:
                icon = QIcon(":/icons/spectrogram.png");
                mainText = tr("Add &Spectrogram");
                if (menuType == 0) {
                    shortcutText = tr("Alt+S");
                    tipText = tr("Add a new pane showing a spectrogram");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram");
                }
                break;
		
            case LayerFactory::MelodicRangeSpectrogram:
                icon = QIcon(":/icons/spectrogram.png");
                mainText = tr("Add &Melodic Range Spectrogram");
                if (menuType == 0) {
                    shortcutText = tr("Alt+M");
                    tipText = tr("Add a new pane showing a spectrogram set up for an overview of note pitches");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram set up for an overview of note pitches");
                }
                break;
		
            case LayerFactory::PeakFrequencySpectrogram:
                icon = QIcon(":/icons/spectrogram.png");
                mainText = tr("Add &Peak Frequency Spectrogram");
                if (menuType == 0) {
                    shortcutText = tr("Alt+P");
                    tipText = tr("Add a new pane showing a spectrogram set up for tracking frequencies");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram set up for tracking frequencies");
                }
                break;
                
            case LayerFactory::Spectrum:
                icon = QIcon(":/icons/spectrum.png");
                mainText = tr("Add Spectr&um");
                if (menuType == 0) {
                    shortcutText = tr("Alt+U");
                    tipText = tr("Add a new pane showing a frequency spectrum");
                } else {
                    tipText = tr("Add a new layer showing a frequency spectrum");
                }
                break;
                
            default: break;
            }

            std::vector<Model *> candidateModels;
            if (menuType == 0) {
                candidateModels = models;
            } else {
                candidateModels.push_back(0);
            }
            
            for (std::vector<Model *>::iterator mi =
                     candidateModels.begin();
                 mi != candidateModels.end(); ++mi) {
                
                Model *model = *mi;

                int channels = 0;
                if (model) {
                    DenseTimeValueModel *dtvm =
                        dynamic_cast<DenseTimeValueModel *>(model);
                    if (dtvm) channels = dtvm->getChannelCount();
                }
                if (channels < 1 && getMainModel()) {
                    channels = getMainModel()->getChannelCount();
                }
                if (channels < 1) channels = 1;

                for (int c = 0; c <= channels; ++c) {

                    if (c == 1 && channels == 1) continue;
                    bool isDefault = (c == 0);
                    bool isOnly = (isDefault && (channels == 1));

                    if (menuType == 1) {
                        if (isDefault) isOnly = true;
                        else continue;
                    }

                    if (isOnly && (!plural || menuType == 1)) {

                        if (menuType == 1 && type != LayerFactory::Waveform) {
                            action = new QAction(mainText, this);
                        } else {
                            action = new QAction(icon, mainText, this);
                        }                            

                        action->setShortcut(shortcutText);
                        action->setStatusTip(tipText);
                        if (menuType == 0) {
                            connect(action, SIGNAL(triggered()),
                                    this, SLOT(addPane()));
                            connect(this, SIGNAL(canAddPane(bool)),
                                    action, SLOT(setEnabled(bool)));
                            m_paneActions[action] = PaneConfiguration(type);
                        } else {
                            connect(action, SIGNAL(triggered()),
                                    this, SLOT(addLayer()));
                            connect(this, SIGNAL(canAddLayer(bool)),
                                    action, SLOT(setEnabled(bool)));
                            m_layerActions[action] = type;
                        }
                        menu->addAction(action);
                        
                    } else {
                        
                        if (!submenu) {
                            submenu = menu->addMenu(mainText);
                            submenu->setTearOffEnabled(true);
                        } else if (isDefault) {
                            submenu->addSeparator();
                        }

                        QString actionText;
                        if (c == 0) {
                            if (mono) {
                                actionText = tr("&All Channels Mixed");
                            } else {
                                actionText = tr("&All Channels");
                            }
                        } else {
                            actionText = tr("Channel &%1").arg(c);
                        }

                        if (model) {
                            actionText = tr("%1: %2")
                                .arg(model->objectName())
                                .arg(actionText);
                        }

                        if (isDefault) {
                            action = new QAction(icon, actionText, this);
                            if (!model || model == getMainModel()) {
                                action->setShortcut(shortcutText);
                            }
                        } else {
                            action = new QAction(actionText, this);
                        }

                        action->setStatusTip(tipText);

                        if (menuType == 0) {
                            connect(action, SIGNAL(triggered()),
                                    this, SLOT(addPane()));
                            connect(this, SIGNAL(canAddPane(bool)),
                                    action, SLOT(setEnabled(bool)));
                            m_paneActions[action] =
                                PaneConfiguration(type, model, c - 1);
                        } else {
                            connect(action, SIGNAL(triggered()),
                                    this, SLOT(addLayer()));
                            connect(this, SIGNAL(canAddLayer(bool)),
                                    action, SLOT(setEnabled(bool)));
                            m_layerActions[action] = type;
                        }

                        submenu->addAction(action);
                    }
		}
	    }
	}
    }

    menu = m_paneMenu;

    menu->addSeparator();

    action = new QAction(QIcon(":/icons/editdelete.png"), tr("&Delete Pane"), this);
    action->setShortcut(tr("Alt+D"));
    action->setStatusTip(tr("Delete the currently active pane"));
    connect(action, SIGNAL(triggered()), this, SLOT(deleteCurrentPane()));
    connect(this, SIGNAL(canDeleteCurrentPane(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    menu = m_layerMenu;

    action = new QAction(QIcon(":/icons/timeruler.png"), tr("Add &Time Ruler"), this);
    action->setStatusTip(tr("Add a new layer showing a time ruler"));
    connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
    connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
    m_layerActions[action] = LayerFactory::TimeRuler;
    menu->addAction(action);

    menu->addSeparator();

    m_existingLayersMenu = menu->addMenu(tr("Add &Existing Layer"));
    m_existingLayersMenu->setTearOffEnabled(true);
    m_rightButtonLayerMenu->addMenu(m_existingLayersMenu);

    m_sliceMenu = menu->addMenu(tr("Add S&lice of Layer"));
    m_sliceMenu->setTearOffEnabled(true);
    m_rightButtonLayerMenu->addMenu(m_sliceMenu);

    setupExistingLayersMenus();

    m_rightButtonLayerMenu->addSeparator();
    menu->addSeparator();

    action = new QAction(tr("&Rename Layer..."), this);
    action->setShortcut(tr("Alt+R"));
    action->setStatusTip(tr("Rename the currently active layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(renameCurrentLayer()));
    connect(this, SIGNAL(canRenameLayer(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonLayerMenu->addAction(action);

    action = new QAction(QIcon(":/icons/editdelete.png"), tr("&Delete Layer"), this);
    action->setShortcut(tr("Alt+Shift+D"));
    action->setStatusTip(tr("Delete the currently active layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(deleteCurrentLayer()));
    connect(this, SIGNAL(canDeleteCurrentLayer(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);
    m_rightButtonLayerMenu->addAction(action);
}

void
MainWindow::setupTransformsMenu()
{
    if (m_transformsMenu) {
        m_transformActions.clear();
        m_transformActionsReverse.clear();
        m_transformsMenu->clear();
    } else {
	m_transformsMenu = menuBar()->addMenu(tr("&Transform")); 
        m_transformsMenu->setTearOffEnabled(true);
   }

    TransformFactory::TransformList transforms =
	TransformFactory::getInstance()->getAllTransforms();

    vector<QString> types =
        TransformFactory::getInstance()->getAllTransformTypes();

    map<QString, map<QString, SubdividingMenu *> > categoryMenus;
    map<QString, map<QString, SubdividingMenu *> > makerMenus;

    map<QString, SubdividingMenu *> byPluginNameMenus;
    map<QString, map<QString, QMenu *> > pluginNameMenus;

    set<SubdividingMenu *> pendingMenus;

    m_recentTransformsMenu = m_transformsMenu->addMenu(tr("&Recent Transforms"));
    m_recentTransformsMenu->setTearOffEnabled(true);
    m_rightButtonTransformsMenu->addMenu(m_recentTransformsMenu);
    connect(&m_recentTransforms, SIGNAL(recentChanged()),
            this, SLOT(setupRecentTransformsMenu()));

    m_transformsMenu->addSeparator();
    m_rightButtonTransformsMenu->addSeparator();
    
    for (vector<QString>::iterator i = types.begin(); i != types.end(); ++i) {

        if (i != types.begin()) {
            m_transformsMenu->addSeparator();
            m_rightButtonTransformsMenu->addSeparator();
        }

        QString byCategoryLabel = tr("%1 by Category").arg(*i);
        SubdividingMenu *byCategoryMenu = new SubdividingMenu(byCategoryLabel,
                                                              20, 40);
        byCategoryMenu->setTearOffEnabled(true);
        m_transformsMenu->addMenu(byCategoryMenu);
        m_rightButtonTransformsMenu->addMenu(byCategoryMenu);
        pendingMenus.insert(byCategoryMenu);

        vector<QString> categories = 
            TransformFactory::getInstance()->getTransformCategories(*i);

        for (vector<QString>::iterator j = categories.begin();
             j != categories.end(); ++j) {

            QString category = *j;
            if (category == "") category = tr("Unclassified");

            if (categories.size() < 2) {
                categoryMenus[*i][category] = byCategoryMenu;
                continue;
            }

            QStringList components = category.split(" > ");
            QString key;

            for (QStringList::iterator k = components.begin();
                 k != components.end(); ++k) {

                QString parentKey = key;
                if (key != "") key += " > ";
                key += *k;

                if (categoryMenus[*i].find(key) == categoryMenus[*i].end()) {
                    SubdividingMenu *m = new SubdividingMenu(*k, 20, 40);
                    m->setTearOffEnabled(true);
                    pendingMenus.insert(m);
                    categoryMenus[*i][key] = m;
                    if (parentKey == "") {
                        byCategoryMenu->addMenu(m);
                    } else {
                        categoryMenus[*i][parentKey]->addMenu(m);
                    }
                }
            }
        }

        QString byPluginNameLabel = tr("%1 by Plugin Name").arg(*i);
        byPluginNameMenus[*i] = new SubdividingMenu(byPluginNameLabel);
        byPluginNameMenus[*i]->setTearOffEnabled(true);
        m_transformsMenu->addMenu(byPluginNameMenus[*i]);
        m_rightButtonTransformsMenu->addMenu(byPluginNameMenus[*i]);
        pendingMenus.insert(byPluginNameMenus[*i]);

        QString byMakerLabel = tr("%1 by Maker").arg(*i);
        SubdividingMenu *byMakerMenu = new SubdividingMenu(byMakerLabel, 20, 40);
        byMakerMenu->setTearOffEnabled(true);
        m_transformsMenu->addMenu(byMakerMenu);
        m_rightButtonTransformsMenu->addMenu(byMakerMenu);
        pendingMenus.insert(byMakerMenu);

        vector<QString> makers = 
            TransformFactory::getInstance()->getTransformMakers(*i);

        for (vector<QString>::iterator j = makers.begin();
             j != makers.end(); ++j) {

            QString maker = *j;
            if (maker == "") maker = tr("Unknown");
            maker.replace(QRegExp(tr(" [\\(<].*$")), "");

            makerMenus[*i][maker] = new SubdividingMenu(maker, 30, 40);
            makerMenus[*i][maker]->setTearOffEnabled(true);
            byMakerMenu->addMenu(makerMenus[*i][maker]);
            pendingMenus.insert(makerMenus[*i][maker]);
        }
    }

    for (unsigned int i = 0; i < transforms.size(); ++i) {
	
	QString name = transforms[i].name;
	if (name == "") name = transforms[i].identifier;

//        std::cerr << "Plugin Name: " << name.toStdString() << std::endl;

        QString type = transforms[i].type;

        QString category = transforms[i].category;
        if (category == "") category = tr("Unclassified");

        QString maker = transforms[i].maker;
        if (maker == "") maker = tr("Unknown");
        maker.replace(QRegExp(tr(" [\\(<].*$")), "");

        QString pluginName = name.section(": ", 0, 0);
        QString output = name.section(": ", 1);

	QAction *action = new QAction(tr("%1...").arg(name), this);
	connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
	m_transformActions[action] = transforms[i].identifier;
        m_transformActionsReverse[transforms[i].identifier] = action;
	connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));

        action->setStatusTip(transforms[i].description);

        if (categoryMenus[type].find(category) == categoryMenus[type].end()) {
            std::cerr << "WARNING: MainWindow::setupMenus: Internal error: "
                      << "No category menu for transform \""
                      << name.toStdString() << "\" (category = \""
                      << category.toStdString() << "\")" << std::endl;
        } else {
            categoryMenus[type][category]->addAction(action);
        }

        if (makerMenus[type].find(maker) == makerMenus[type].end()) {
            std::cerr << "WARNING: MainWindow::setupMenus: Internal error: "
                      << "No maker menu for transform \""
                      << name.toStdString() << "\" (maker = \""
                      << maker.toStdString() << "\")" << std::endl;
        } else {
            makerMenus[type][maker]->addAction(action);
        }

        action = new QAction(tr("%1...").arg(output == "" ? pluginName : output), this);
        connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
        m_transformActions[action] = transforms[i].identifier;
        connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
        action->setStatusTip(transforms[i].description);

//        cerr << "Transform: \"" << name.toStdString() << "\": plugin name \"" << pluginName.toStdString() << "\"" << endl;

        if (pluginNameMenus[type].find(pluginName) ==
            pluginNameMenus[type].end()) {

            SubdividingMenu *parentMenu = byPluginNameMenus[type];
            parentMenu->setTearOffEnabled(true);

            if (output == "") {
                parentMenu->addAction(pluginName, action);
            } else {
                pluginNameMenus[type][pluginName] = 
                    parentMenu->addMenu(pluginName);
                connect(this, SIGNAL(canAddLayer(bool)),
                        pluginNameMenus[type][pluginName],
                        SLOT(setEnabled(bool)));
            }
        }

        if (pluginNameMenus[type].find(pluginName) !=
            pluginNameMenus[type].end()) {
            pluginNameMenus[type][pluginName]->addAction(action);
        }
    }

    for (set<SubdividingMenu *>::iterator i = pendingMenus.begin();
         i != pendingMenus.end(); ++i) {
        (*i)->entriesAdded();
    }

    setupRecentTransformsMenu();
}

void
MainWindow::setupHelpMenu()
{
    if (m_mainMenusCreated) return;

    QMenu *menu = menuBar()->addMenu(tr("&Help"));
    menu->setTearOffEnabled(true);
    
    QAction *action = new QAction(QIcon(":icons/help.png"),
                                  tr("&Help Reference"), this); 
    action->setStatusTip(tr("Open the Sonic Visualiser reference manual")); 
    connect(action, SIGNAL(triggered()), this, SLOT(help()));
    menu->addAction(action);
    
    action = new QAction(tr("Sonic Visualiser on the &Web"), this); 
    action->setStatusTip(tr("Open the Sonic Visualiser website")); 
    connect(action, SIGNAL(triggered()), this, SLOT(website()));
    menu->addAction(action);
    
    action = new QAction(tr("&About Sonic Visualiser"), this); 
    action->setStatusTip(tr("Show information about Sonic Visualiser")); 
    connect(action, SIGNAL(triggered()), this, SLOT(about()));
    menu->addAction(action);
}

void
MainWindow::setupRecentFilesMenu()
{
    m_recentFilesMenu->clear();
    vector<QString> files = m_recentFiles.getRecent();
    for (size_t i = 0; i < files.size(); ++i) {
	QAction *action = new QAction(files[i], this);
	connect(action, SIGNAL(triggered()), this, SLOT(openRecentFile()));
	m_recentFilesMenu->addAction(action);
    }
}

void
MainWindow::setupRecentTransformsMenu()
{
    m_recentTransformsMenu->clear();
    vector<QString> transforms = m_recentTransforms.getRecent();
    for (size_t i = 0; i < transforms.size(); ++i) {
        TransformActionReverseMap::iterator ti =
            m_transformActionsReverse.find(transforms[i]);
        if (ti == m_transformActionsReverse.end()) {
            std::cerr << "WARNING: MainWindow::setupRecentTransformsMenu: "
                      << "Unknown transform \"" << transforms[i].toStdString()
                      << "\" in recent transforms list" << std::endl;
            continue;
        }
	m_recentTransformsMenu->addAction(ti->second);
    }
}

void
MainWindow::setupExistingLayersMenus()
{
    if (!m_existingLayersMenu) return; // should have been created by setupMenus

//    std::cerr << "MainWindow::setupExistingLayersMenu" << std::endl;

    m_existingLayersMenu->clear();
    m_existingLayerActions.clear();

    m_sliceMenu->clear();
    m_sliceActions.clear();

    vector<Layer *> orderedLayers;
    set<Layer *> observedLayers;
    set<Layer *> sliceableLayers;

    LayerFactory *factory = LayerFactory::getInstance();

    for (int i = 0; i < m_paneStack->getPaneCount(); ++i) {

	Pane *pane = m_paneStack->getPane(i);
	if (!pane) continue;

	for (int j = 0; j < pane->getLayerCount(); ++j) {

	    Layer *layer = pane->getLayer(j);
	    if (!layer) continue;
	    if (observedLayers.find(layer) != observedLayers.end()) {
//		std::cerr << "found duplicate layer " << layer << std::endl;
		continue;
	    }

//	    std::cerr << "found new layer " << layer << " (name = " 
//		      << layer->getLayerPresentationName().toStdString() << ")" << std::endl;

	    orderedLayers.push_back(layer);
	    observedLayers.insert(layer);

            if (factory->isLayerSliceable(layer)) {
                sliceableLayers.insert(layer);
            }
	}
    }

    map<QString, int> observedNames;

    for (size_t i = 0; i < orderedLayers.size(); ++i) {
	
        Layer *layer = orderedLayers[i];

	QString name = layer->getLayerPresentationName();
	int n = ++observedNames[name];
	if (n > 1) name = QString("%1 <%2>").arg(name).arg(n);

	QIcon icon = QIcon(QString(":/icons/%1.png")
                           .arg(factory->getLayerIconName
                                (factory->getLayerType(layer))));

	QAction *action = new QAction(icon, name, this);
	connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
	connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
	m_existingLayerActions[action] = layer;

	m_existingLayersMenu->addAction(action);

        if (sliceableLayers.find(layer) != sliceableLayers.end()) {
            action = new QAction(icon, name, this);
            connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
            connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
            m_sliceActions[action] = layer;
            m_sliceMenu->addAction(action);
        }
    }

    m_sliceMenu->setEnabled(!m_sliceActions.empty());
}

void
MainWindow::setupToolbars()
{
    QToolBar *toolbar = addToolBar(tr("Transport Toolbar"));

    QAction *action = toolbar->addAction(QIcon(":/icons/rewind-start.png"),
					 tr("Rewind to Start"));
    action->setShortcut(tr("Home"));
    action->setStatusTip(tr("Rewind to the start"));
    connect(action, SIGNAL(triggered()), this, SLOT(rewindStart()));
    connect(this, SIGNAL(canPlay(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/rewind.png"),
				tr("Rewind"));
    action->setShortcut(tr("PageUp"));
    action->setStatusTip(tr("Rewind to the previous time instant in the current layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(rewind()));
    connect(this, SIGNAL(canRewind(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/playpause.png"),
				tr("Play / Pause"));
    action->setCheckable(true);
    action->setShortcut(tr("Space"));
    action->setStatusTip(tr("Start or stop playback from the current position"));
    connect(action, SIGNAL(triggered()), this, SLOT(play()));
    connect(m_playSource, SIGNAL(playStatusChanged(bool)),
	    action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(canPlay(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/ffwd.png"),
				tr("Fast Forward"));
    action->setShortcut(tr("PageDown"));
    action->setStatusTip(tr("Fast forward to the next time instant in the current layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(ffwd()));
    connect(this, SIGNAL(canFfwd(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/ffwd-end.png"),
				tr("Fast Forward to End"));
    action->setShortcut(tr("End"));
    action->setStatusTip(tr("Fast-forward to the end"));
    connect(action, SIGNAL(triggered()), this, SLOT(ffwdEnd()));
    connect(this, SIGNAL(canPlay(bool)), action, SLOT(setEnabled(bool)));

    toolbar = addToolBar(tr("Play Mode Toolbar"));

    action = toolbar->addAction(QIcon(":/icons/playselection.png"),
				tr("Constrain Playback to Selection"));
    action->setCheckable(true);
    action->setChecked(m_viewManager->getPlaySelectionMode());
    action->setShortcut(tr("s"));
    action->setStatusTip(tr("Constrain playback to the selected area"));
    connect(m_viewManager, SIGNAL(playSelectionModeChanged(bool)),
            action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(triggered()), this, SLOT(playSelectionToggled()));
    connect(this, SIGNAL(canPlaySelection(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/playloop.png"),
				tr("Loop Playback"));
    action->setCheckable(true);
    action->setChecked(m_viewManager->getPlayLoopMode());
    action->setShortcut(tr("l"));
    action->setStatusTip(tr("Loop playback"));
    connect(m_viewManager, SIGNAL(playLoopModeChanged(bool)),
            action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(triggered()), this, SLOT(playLoopToggled()));
    connect(this, SIGNAL(canPlay(bool)), action, SLOT(setEnabled(bool)));

    toolbar = addToolBar(tr("Edit Toolbar"));
    CommandHistory::getInstance()->registerToolbar(toolbar);

    toolbar = addToolBar(tr("Tools Toolbar"));
    QActionGroup *group = new QActionGroup(this);

    action = toolbar->addAction(QIcon(":/icons/navigate.png"),
				tr("Navigate"));
    action->setCheckable(true);
    action->setChecked(true);
    action->setShortcut(tr("1"));
    action->setStatusTip(tr("Navigate"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolNavigateSelected()));
    group->addAction(action);
    m_toolActions[ViewManager::NavigateMode] = action;

    action = toolbar->addAction(QIcon(":/icons/select.png"),
				tr("Select"));
    action->setCheckable(true);
    action->setShortcut(tr("2"));
    action->setStatusTip(tr("Select ranges"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolSelectSelected()));
    group->addAction(action);
    m_toolActions[ViewManager::SelectMode] = action;

    action = toolbar->addAction(QIcon(":/icons/move.png"),
				tr("Edit"));
    action->setCheckable(true);
    action->setShortcut(tr("3"));
    action->setStatusTip(tr("Edit items in layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolEditSelected()));
    connect(this, SIGNAL(canEditLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_toolActions[ViewManager::EditMode] = action;

    action = toolbar->addAction(QIcon(":/icons/draw.png"),
				tr("Draw"));
    action->setCheckable(true);
    action->setShortcut(tr("4"));
    action->setStatusTip(tr("Draw new items in layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolDrawSelected()));
    connect(this, SIGNAL(canEditLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_toolActions[ViewManager::DrawMode] = action;

//    action = toolbar->addAction(QIcon(":/icons/text.png"),
//				tr("Text"));
//    action->setCheckable(true);
//    action->setShortcut(tr("5"));
//    connect(action, SIGNAL(triggered()), this, SLOT(toolTextSelected()));
//    group->addAction(action);
//    m_toolActions[ViewManager::TextMode] = action;

    toolNavigateSelected();
}

void
MainWindow::updateMenuStates()
{
    Pane *currentPane = 0;
    Layer *currentLayer = 0;

    if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentLayer = currentPane->getSelectedLayer();

    bool haveCurrentPane =
        (currentPane != 0);
    bool haveCurrentLayer =
        (haveCurrentPane &&
         (currentLayer != 0));
    bool haveMainModel =
	(getMainModel() != 0);
    bool havePlayTarget =
	(m_playTarget != 0);
    bool haveSelection = 
	(m_viewManager &&
	 !m_viewManager->getSelections().empty());
    bool haveCurrentEditableLayer =
	(haveCurrentLayer &&
	 currentLayer->isLayerEditable());
    bool haveCurrentTimeInstantsLayer = 
	(haveCurrentLayer &&
	 dynamic_cast<TimeInstantLayer *>(currentLayer));
    bool haveCurrentTimeValueLayer = 
	(haveCurrentLayer &&
	 dynamic_cast<TimeValueLayer *>(currentLayer));
    bool haveCurrentColour3DPlot =
        (haveCurrentLayer &&
         dynamic_cast<Colour3DPlotLayer *>(currentLayer));
    bool haveClipboardContents =
        (m_viewManager &&
         !m_viewManager->getClipboard().empty());

    emit canAddPane(haveMainModel);
    emit canDeleteCurrentPane(haveCurrentPane);
    emit canZoom(haveMainModel && haveCurrentPane);
    emit canScroll(haveMainModel && haveCurrentPane);
    emit canAddLayer(haveMainModel && haveCurrentPane);
    emit canImportMoreAudio(haveMainModel);
    emit canImportLayer(haveMainModel && haveCurrentPane);
    emit canExportAudio(haveMainModel);
    emit canExportLayer(haveMainModel &&
                        (haveCurrentEditableLayer || haveCurrentColour3DPlot));
    emit canExportImage(haveMainModel && haveCurrentPane);
    emit canDeleteCurrentLayer(haveCurrentLayer);
    emit canRenameLayer(haveCurrentLayer);
    emit canEditLayer(haveCurrentEditableLayer);
    emit canSelect(haveMainModel && haveCurrentPane);
    emit canPlay(/*!!! haveMainModel && */ havePlayTarget);
    emit canFfwd(haveCurrentTimeInstantsLayer || haveCurrentTimeValueLayer);
    emit canRewind(haveCurrentTimeInstantsLayer || haveCurrentTimeValueLayer);
    emit canPaste(haveCurrentEditableLayer && haveClipboardContents);
    emit canInsertInstant(haveCurrentPane);
    emit canInsertInstantsAtBoundaries(haveCurrentPane && haveSelection);
    emit canPlaySelection(haveMainModel && havePlayTarget && haveSelection);
    emit canClearSelection(haveSelection);
    emit canEditSelection(haveSelection && haveCurrentEditableLayer);
    emit canSave(m_sessionFile != "" && m_documentModified);
}

void
MainWindow::updateDescriptionLabel()
{
    if (!getMainModel()) {
	m_descriptionLabel->setText(tr("No audio file loaded."));
	return;
    }

    QString description;

    size_t ssr = getMainModel()->getSampleRate();
    size_t tsr = ssr;
    if (m_playSource) tsr = m_playSource->getTargetSampleRate();

    if (ssr != tsr) {
	description = tr("%1Hz (resampling to %2Hz)").arg(ssr).arg(tsr);
    } else {
	description = QString("%1Hz").arg(ssr);
    }

    description = QString("%1 - %2")
	.arg(RealTime::frame2RealTime(getMainModel()->getEndFrame(), ssr)
	     .toText(false).c_str())
	.arg(description);

    m_descriptionLabel->setText(description);
}

void
MainWindow::documentModified()
{
//    std::cerr << "MainWindow::documentModified" << std::endl;

    if (!m_documentModified) {
	setWindowTitle(tr("%1 (modified)").arg(windowTitle()));
    }

    m_documentModified = true;
    updateMenuStates();
}

void
MainWindow::documentRestored()
{
//    std::cerr << "MainWindow::documentRestored" << std::endl;

    if (m_documentModified) {
	QString wt(windowTitle());
	wt.replace(tr(" (modified)"), "");
	setWindowTitle(wt);
    }

    m_documentModified = false;
    updateMenuStates();
}

void
MainWindow::playLoopToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (action) {
	m_viewManager->setPlayLoopMode(action->isChecked());
    } else {
	m_viewManager->setPlayLoopMode(!m_viewManager->getPlayLoopMode());
    }
}

void
MainWindow::playSelectionToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (action) {
	m_viewManager->setPlaySelectionMode(action->isChecked());
    } else {
	m_viewManager->setPlaySelectionMode(!m_viewManager->getPlaySelectionMode());
    }
}

void
MainWindow::currentPaneChanged(Pane *p)
{
    updateMenuStates();
    updateVisibleRangeDisplay(p);
}

void
MainWindow::currentLayerChanged(Pane *p, Layer *)
{
    updateMenuStates();
    updateVisibleRangeDisplay(p);
}

void
MainWindow::toolNavigateSelected()
{
    m_viewManager->setToolMode(ViewManager::NavigateMode);
}

void
MainWindow::toolSelectSelected()
{
    m_viewManager->setToolMode(ViewManager::SelectMode);
}

void
MainWindow::toolEditSelected()
{
    m_viewManager->setToolMode(ViewManager::EditMode);
}

void
MainWindow::toolDrawSelected()
{
    m_viewManager->setToolMode(ViewManager::DrawMode);
}

//void
//MainWindow::toolTextSelected()
//{
//    m_viewManager->setToolMode(ViewManager::TextMode);
//}

void
MainWindow::selectAll()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(getMainModel()->getStartFrame(),
					  getMainModel()->getEndFrame()));
}

void
MainWindow::selectToStart()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(getMainModel()->getStartFrame(),
					  m_viewManager->getGlobalCentreFrame()));
}

void
MainWindow::selectToEnd()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(m_viewManager->getGlobalCentreFrame(),
					  getMainModel()->getEndFrame()));
}

void
MainWindow::selectVisible()
{
    Model *model = getMainModel();
    if (!model) return;

    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    size_t startFrame, endFrame;

    if (currentPane->getStartFrame() < 0) startFrame = 0;
    else startFrame = currentPane->getStartFrame();

    if (currentPane->getEndFrame() > model->getEndFrame()) endFrame = model->getEndFrame();
    else endFrame = currentPane->getEndFrame();

    m_viewManager->setSelection(Selection(startFrame, endFrame));
}

void
MainWindow::clearSelection()
{
    m_viewManager->clearSelections();
}

void
MainWindow::cut()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    Layer *layer = currentPane->getSelectedLayer();
    if (!layer) return;

    Clipboard &clipboard = m_viewManager->getClipboard();
    clipboard.clear();

    MultiSelection::SelectionList selections = m_viewManager->getSelections();

    CommandHistory::getInstance()->startCompoundOperation(tr("Cut"), true);

    for (MultiSelection::SelectionList::iterator i = selections.begin();
         i != selections.end(); ++i) {
        layer->copy(*i, clipboard);
        layer->deleteSelection(*i);
    }

    CommandHistory::getInstance()->endCompoundOperation();
}

void
MainWindow::copy()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    Layer *layer = currentPane->getSelectedLayer();
    if (!layer) return;

    Clipboard &clipboard = m_viewManager->getClipboard();
    clipboard.clear();

    MultiSelection::SelectionList selections = m_viewManager->getSelections();

    for (MultiSelection::SelectionList::iterator i = selections.begin();
         i != selections.end(); ++i) {
        layer->copy(*i, clipboard);
    }
}

void
MainWindow::paste()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    //!!! if we have no current layer, we should create one of the most
    // appropriate type

    Layer *layer = currentPane->getSelectedLayer();
    if (!layer) return;

    Clipboard &clipboard = m_viewManager->getClipboard();
    Clipboard::PointList contents = clipboard.getPoints();
/*
    long minFrame = 0;
    bool have = false;
    for (int i = 0; i < contents.size(); ++i) {
        if (!contents[i].haveFrame()) continue;
        if (!have || contents[i].getFrame() < minFrame) {
            minFrame = contents[i].getFrame();
            have = true;
        }
    }

    long frameOffset = long(m_viewManager->getGlobalCentreFrame()) - minFrame;

    layer->paste(clipboard, frameOffset);
*/
    layer->paste(clipboard, 0, true);
}

void
MainWindow::deleteSelected()
{
    if (m_paneStack->getCurrentPane() &&
	m_paneStack->getCurrentPane()->getSelectedLayer()) {

	MultiSelection::SelectionList selections =
	    m_viewManager->getSelections();

	for (MultiSelection::SelectionList::iterator i = selections.begin();
	     i != selections.end(); ++i) {

	    m_paneStack->getCurrentPane()->getSelectedLayer()->deleteSelection(*i);
	}
    }
}

void
MainWindow::insertInstant()
{
    int frame = m_viewManager->getPlaybackFrame();
    insertInstantAt(frame);
}

void
MainWindow::insertInstantsAtBoundaries()
{
    MultiSelection::SelectionList selections = m_viewManager->getSelections();
    for (MultiSelection::SelectionList::iterator i = selections.begin();
         i != selections.end(); ++i) {
        size_t start = i->getStartFrame();
        size_t end = i->getEndFrame();
        if (start != end) {
            insertInstantAt(i->getStartFrame());
            insertInstantAt(i->getEndFrame());
        }
    }
}

void
MainWindow::insertInstantAt(size_t frame)
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) {
        return;
    }

    Layer *layer = dynamic_cast<TimeInstantLayer *>
        (pane->getSelectedLayer());

    if (!layer) {
        for (int i = pane->getLayerCount(); i > 0; --i) {
            layer = dynamic_cast<TimeInstantLayer *>(pane->getLayer(i - 1));
            if (layer) break;
        }

        if (!layer) {
            CommandHistory::getInstance()->startCompoundOperation
                (tr("Add Point"), true);
            layer = m_document->createEmptyLayer(LayerFactory::TimeInstants);
            if (layer) {
                m_document->addLayerToView(pane, layer);
                m_paneStack->setCurrentLayer(pane, layer);
            }
            CommandHistory::getInstance()->endCompoundOperation();
        }
    }

    if (layer) {
    
        Model *model = layer->getModel();
        SparseOneDimensionalModel *sodm = dynamic_cast<SparseOneDimensionalModel *>
            (model);

        if (sodm) {
            SparseOneDimensionalModel::Point point
                (frame, QString("%1").arg(sodm->getPointCount() + 1));
            CommandHistory::getInstance()->addCommand
                (new SparseOneDimensionalModel::AddPointCommand(sodm, point,
                                                                tr("Add Points")),
                 true, true); // bundled
        }
    }
}

void
MainWindow::importAudio()
{
    QString path = getOpenFileName(FileFinder::AudioFile);

    if (path != "") {
	if (openAudioFile(path, ReplaceMainModel) == FileOpenFailed) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("Audio file \"%1\" could not be opened").arg(path));
	}
    }
}

void
MainWindow::importMoreAudio()
{
    QString path = getOpenFileName(FileFinder::AudioFile);

    if (path != "") {
	if (openAudioFile(path, CreateAdditionalModel) == FileOpenFailed) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("Audio file \"%1\" could not be opened").arg(path));
	}
    }
}

void
MainWindow::exportAudio()
{
    if (!getMainModel()) return;

    QString path = getSaveFileName(FileFinder::AudioFile);
    
    if (path == "") return;

    bool ok = false;
    QString error;

    MultiSelection ms = m_viewManager->getSelection();
    MultiSelection::SelectionList selections = m_viewManager->getSelections();

    bool multiple = false;

    MultiSelection *selectionToWrite = 0;

    if (selections.size() == 1) {

	QStringList items;
	items << tr("Export the selected region only")
	      << tr("Export the whole audio file");
	
	bool ok = false;
	QString item = ListInputDialog::getItem
	    (this, tr("Select region to export"),
	     tr("Which region from the original audio file do you want to export?"),
	     items, 0, &ok);
	
	if (!ok || item.isEmpty()) return;
	
	if (item == items[0]) selectionToWrite = &ms;

    } else if (selections.size() > 1) {

	QStringList items;
	items << tr("Export the selected regions into a single audio file")
	      << tr("Export the selected regions into separate files")
	      << tr("Export the whole audio file");

	QString item = ListInputDialog::getItem
	    (this, tr("Select region to export"),
	     tr("Multiple regions of the original audio file are selected.\nWhat do you want to export?"),
	     items, 0, &ok);
	    
	if (!ok || item.isEmpty()) return;

	if (item == items[0]) {

            selectionToWrite = &ms;

        } else if (item == items[1]) {

            multiple = true;

	    int n = 1;
	    QString base = path;
	    base.replace(".wav", "");

	    for (MultiSelection::SelectionList::iterator i = selections.begin();
		 i != selections.end(); ++i) {

		MultiSelection subms;
		subms.setSelection(*i);

		QString subpath = QString("%1.%2.wav").arg(base).arg(n);
		++n;

		if (QFileInfo(subpath).exists()) {
		    error = tr("Fragment file %1 already exists, aborting").arg(subpath);
		    break;
		}

		WavFileWriter subwriter(subpath,
                                        getMainModel()->getSampleRate(),
                                        getMainModel()->getChannelCount());
                subwriter.writeModel(getMainModel(), &subms);
		ok = subwriter.isOK();

		if (!ok) {
		    error = subwriter.getError();
		    break;
		}
	    }
	}
    }

    if (!multiple) {
        WavFileWriter writer(path,
                             getMainModel()->getSampleRate(),
                             getMainModel()->getChannelCount());
        writer.writeModel(getMainModel(), selectionToWrite);
	ok = writer.isOK();
	error = writer.getError();
    }

    if (ok) {
        if (!multiple) {
            m_recentFiles.addFile(path);
        }
    } else {
	QMessageBox::critical(this, tr("Failed to write file"), error);
    }
}

void
MainWindow::importLayer()
{
    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::importLayer: no current pane" << std::endl;
	return;
    }

    if (!getMainModel()) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::importLayer: No main model -- hence no default sample rate available" << std::endl;
	return;
    }

    QString path = getOpenFileName(FileFinder::LayerFile);

    if (path != "") {

        if (openLayerFile(path) == FileOpenFailed) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("File %1 could not be opened.").arg(path));
            return;
        }
    }
}

MainWindow::FileOpenStatus
MainWindow::openLayerFile(QString path)
{
    return openLayerFile(path, path);
}

MainWindow::FileOpenStatus
MainWindow::openLayerFile(QString path, QString location)
{
    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::openLayerFile: no current pane" << std::endl;
	return FileOpenFailed;
    }

    if (!getMainModel()) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::openLayerFile: No main model -- hence no default sample rate available" << std::endl;
	return FileOpenFailed;
    }

    bool realFile = (location == path);

    if (path.endsWith(".svl") || path.endsWith(".xml")) {

        PaneCallback callback(this);
        QFile file(path);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << "ERROR: MainWindow::openLayerFile("
                      << location.toStdString()
                      << "): Failed to open file for reading" << std::endl;
            return FileOpenFailed;
        }
        
        SVFileReader reader(m_document, callback, location);
        reader.setCurrentPane(pane);
        
        QXmlInputSource inputSource(&file);
        reader.parse(inputSource);
        
        if (!reader.isOK()) {
            std::cerr << "ERROR: MainWindow::openLayerFile("
                      << location.toStdString()
                      << "): Failed to read XML file: "
                      << reader.getErrorString().toStdString() << std::endl;
            return FileOpenFailed;
        }

        m_recentFiles.addFile(location);

        if (realFile) {
            registerLastOpenedFilePath(FileFinder::LayerFile, path); // for file dialog
        }

        return FileOpenSucceeded;
        
    } else {
        
        Model *model = DataFileReaderFactory::load(path, getMainModel()->getSampleRate());
        
        if (model) {

            Layer *newLayer = m_document->createImportedLayer(model);

            if (newLayer) {

                m_document->addLayerToView(pane, newLayer);
                m_recentFiles.addFile(location);

                if (realFile) {
                    registerLastOpenedFilePath(FileFinder::LayerFile, path); // for file dialog
                }

                return FileOpenSucceeded;
            }
        }
    }

    return FileOpenFailed;
}

void
MainWindow::exportLayer()
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return;

    Layer *layer = pane->getSelectedLayer();
    if (!layer) return;

    Model *model = layer->getModel();
    if (!model) return;

    QString path = getSaveFileName(FileFinder::LayerFile);

    if (path == "") return;

    if (QFileInfo(path).suffix() == "") path += ".svl";

    QString error;

    if (path.endsWith(".xml") || path.endsWith(".svl")) {

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            error = tr("Failed to open file %1 for writing").arg(path);
        } else {
            QTextStream out(&file);
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                << "<!DOCTYPE sonic-visualiser>\n"
                << "<sv>\n"
                << "  <data>\n";

            model->toXml(out, "    ");

            out << "  </data>\n"
                << "  <display>\n";

            layer->toXml(out, "    ");

            out << "  </display>\n"
                << "</sv>\n";
        }

    } else {

        CSVFileWriter writer(path, model,
                             (path.endsWith(".csv") ? "," : "\t"));
        writer.write();

        if (!writer.isOK()) {
            error = writer.getError();
        }
    }

    if (error != "") {
        QMessageBox::critical(this, tr("Failed to write file"), error);
    } else {
        m_recentFiles.addFile(path);
    }
}

void
MainWindow::exportImage()
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return;
    
    QString path = getSaveFileName(FileFinder::ImageFile);

    if (path == "") return;

    if (QFileInfo(path).suffix() == "") path += ".png";

    bool haveSelection = m_viewManager && !m_viewManager->getSelections().empty();

    QSize total, visible, selected;
    total = pane->getImageSize();
    visible = pane->getImageSize(pane->getFirstVisibleFrame(),
                                 pane->getLastVisibleFrame());

    size_t sf0 = 0, sf1 = 0;
 
    if (haveSelection) {
        MultiSelection::SelectionList selections = m_viewManager->getSelections();
        sf0 = selections.begin()->getStartFrame();
        MultiSelection::SelectionList::iterator e = selections.end();
        --e;
        sf1 = e->getEndFrame();
        selected = pane->getImageSize(sf0, sf1);
    }

    QStringList items;
    items << tr("Export the whole pane (%1x%2 pixels)")
        .arg(total.width()).arg(total.height());
    items << tr("Export the visible area only (%1x%2 pixels)")
        .arg(visible.width()).arg(visible.height());
    if (haveSelection) {
        items << tr("Export the selection extent (%1x%2 pixels)")
            .arg(selected.width()).arg(selected.height());
    } else {
        items << tr("Export the selection extent");
    }

    QSettings settings;
    settings.beginGroup("MainWindow");
    int deflt = settings.value("lastimageexportregion", 0).toInt();
    if (deflt == 2 && !haveSelection) deflt = 1;
    if (deflt == 0 && total.width() > 32767) deflt = 1;

    ListInputDialog *lid = new ListInputDialog
        (this, tr("Select region to export"),
         tr("Which region of the current pane do you want to export as an image?"),
         items, deflt);

    if (!haveSelection) {
        lid->setItemAvailability(2, false);
    }
    if (total.width() > 32767) { // appears to be the limit of a QImage
        lid->setItemAvailability(0, false);
        lid->setFootnote(tr("Note: the whole pane is too wide to be exported as a single image."));
    }

    bool ok = lid->exec();
    QString item = lid->getCurrentString();
    delete lid;
	    
    if (!ok || item.isEmpty()) return;

    settings.setValue("lastimageexportregion", deflt);

    QImage *image = 0;

    if (item == items[0]) {
        image = pane->toNewImage();
    } else if (item == items[1]) {
        image = pane->toNewImage(pane->getFirstVisibleFrame(),
                                 pane->getLastVisibleFrame());
    } else if (haveSelection) {
        image = pane->toNewImage(sf0, sf1);
    }

    if (!image) return;

    if (!image->save(path, "PNG")) {
        QMessageBox::critical(this, tr("Failed to save image file"),
                              tr("Failed to save image file %1").arg(path));
    }
    
    delete image;
}

MainWindow::FileOpenStatus
MainWindow::openAudioFile(QString path, AudioFileOpenMode mode)
{
    return openAudioFile(path, path, mode);
}

MainWindow::FileOpenStatus
MainWindow::openAudioFile(QString path, QString location, AudioFileOpenMode mode)
{
    if (!(QFileInfo(path).exists() &&
	  QFileInfo(path).isFile() &&
	  QFileInfo(path).isReadable())) {
	return FileOpenFailed;
    }

    m_openingAudioFile = true;

    WaveFileModel *newModel = new WaveFileModel(path, location);

    if (!newModel->isOK()) {
	delete newModel;
        m_openingAudioFile = false;
	return FileOpenFailed;
    }

    bool setAsMain = true;
    static bool prevSetAsMain = true;

    bool realFile = (location == path);

    if (mode == CreateAdditionalModel) setAsMain = false;
    else if (mode == AskUser) {
        if (m_document->getMainModel()) {

            QStringList items;
            items << tr("Replace the existing main waveform")
                  << tr("Load this file into a new waveform pane");

            bool ok = false;
            QString item = ListInputDialog::getItem
                (this, tr("Select target for import"),
                 tr("You already have an audio waveform loaded.\nWhat would you like to do with the new audio file?"),
                 items, prevSetAsMain ? 0 : 1, &ok);
            
            if (!ok || item.isEmpty()) {
                delete newModel;
                m_openingAudioFile = false;
                return FileOpenCancelled;
            }
            
            setAsMain = (item == items[0]);
            prevSetAsMain = setAsMain;
        }
    }

    if (setAsMain) {

        Model *prevMain = getMainModel();
        if (prevMain) {
            m_playSource->removeModel(prevMain);
            PlayParameterRepository::getInstance()->removeModel(prevMain);
        }

        PlayParameterRepository::getInstance()->addModel(newModel);

	m_document->setMainModel(newModel);
	setupMenus();

	if (m_sessionFile == "") {
	    setWindowTitle(tr("Sonic Visualiser: %1")
			   .arg(QFileInfo(location).fileName()));
	    CommandHistory::getInstance()->clear();
	    CommandHistory::getInstance()->documentSaved();
	    m_documentModified = false;
	} else {
	    setWindowTitle(tr("Sonic Visualiser: %1 [%2]")
			   .arg(QFileInfo(m_sessionFile).fileName())
			   .arg(QFileInfo(location).fileName()));
	    if (m_documentModified) {
		m_documentModified = false;
		documentModified(); // so as to restore "(modified)" window title
	    }
	}

        if (realFile) m_audioFile = path;

    } else { // !setAsMain

	CommandHistory::getInstance()->startCompoundOperation
	    (tr("Import \"%1\"").arg(QFileInfo(location).fileName()), true);

	m_document->addImportedModel(newModel);

	AddPaneCommand *command = new AddPaneCommand(this);
	CommandHistory::getInstance()->addCommand(command);

	Pane *pane = command->getPane();

	if (!m_timeRulerLayer) {
	    m_timeRulerLayer = m_document->createMainModelLayer
		(LayerFactory::TimeRuler);
	}

	m_document->addLayerToView(pane, m_timeRulerLayer);

	Layer *newLayer = m_document->createImportedLayer(newModel);

	if (newLayer) {
	    m_document->addLayerToView(pane, newLayer);
	}
	
	CommandHistory::getInstance()->endCompoundOperation();
    }

    updateMenuStates();
    m_recentFiles.addFile(location);
    if (realFile) {
        registerLastOpenedFilePath(FileFinder::AudioFile, path); // for file dialog
    }
    m_openingAudioFile = false;

    return FileOpenSucceeded;
}

void
MainWindow::createPlayTarget()
{
    if (m_playTarget) return;

    m_playTarget = AudioTargetFactory::createCallbackTarget(m_playSource);
    if (!m_playTarget) {
	QMessageBox::warning
	    (this, tr("Couldn't open audio device"),
	     tr("Could not open an audio device for playback.\nAudio playback will not be available during this session.\n"),
	     QMessageBox::Ok, 0);
    }
    connect(m_fader, SIGNAL(valueChanged(float)),
	    m_playTarget, SLOT(setOutputGain(float)));
}

WaveFileModel *
MainWindow::getMainModel()
{
    if (!m_document) return 0;
    return m_document->getMainModel();
}

const WaveFileModel *
MainWindow::getMainModel() const
{
    if (!m_document) return 0;
    return m_document->getMainModel();
}

void
MainWindow::newSession()
{
    if (!checkSaveModified()) return;

    closeSession();
    createDocument();

    Pane *pane = m_paneStack->addPane();

    connect(pane, SIGNAL(contextHelpChanged(const QString &)),
            this, SLOT(contextHelpChanged(const QString &)));

    if (!m_timeRulerLayer) {
	m_timeRulerLayer = m_document->createMainModelLayer
	    (LayerFactory::TimeRuler);
    }

    m_document->addLayerToView(pane, m_timeRulerLayer);

    Layer *waveform = m_document->createMainModelLayer(LayerFactory::Waveform);
    m_document->addLayerToView(pane, waveform);

    m_overview->registerView(pane);

    CommandHistory::getInstance()->clear();
    CommandHistory::getInstance()->documentSaved();
    documentRestored();
    updateMenuStates();
}

void
MainWindow::createDocument()
{
    m_document = new Document;

    connect(m_document, SIGNAL(layerAdded(Layer *)),
	    this, SLOT(layerAdded(Layer *)));
    connect(m_document, SIGNAL(layerRemoved(Layer *)),
	    this, SLOT(layerRemoved(Layer *)));
    connect(m_document, SIGNAL(layerAboutToBeDeleted(Layer *)),
	    this, SLOT(layerAboutToBeDeleted(Layer *)));
    connect(m_document, SIGNAL(layerInAView(Layer *, bool)),
	    this, SLOT(layerInAView(Layer *, bool)));

    connect(m_document, SIGNAL(modelAdded(Model *)),
	    this, SLOT(modelAdded(Model *)));
    connect(m_document, SIGNAL(mainModelChanged(WaveFileModel *)),
	    this, SLOT(mainModelChanged(WaveFileModel *)));
    connect(m_document, SIGNAL(modelAboutToBeDeleted(Model *)),
	    this, SLOT(modelAboutToBeDeleted(Model *)));

    connect(m_document, SIGNAL(modelGenerationFailed(QString)),
            this, SLOT(modelGenerationFailed(QString)));
    connect(m_document, SIGNAL(modelRegenerationFailed(QString, QString)),
            this, SLOT(modelRegenerationFailed(QString, QString)));
}

void
MainWindow::closeSession()
{
    if (!checkSaveModified()) return;

    while (m_paneStack->getPaneCount() > 0) {

	Pane *pane = m_paneStack->getPane(m_paneStack->getPaneCount() - 1);

	while (pane->getLayerCount() > 0) {
	    m_document->removeLayerFromView
		(pane, pane->getLayer(pane->getLayerCount() - 1));
	}

	m_overview->unregisterView(pane);
	m_paneStack->deletePane(pane);
    }

    while (m_paneStack->getHiddenPaneCount() > 0) {

	Pane *pane = m_paneStack->getHiddenPane
	    (m_paneStack->getHiddenPaneCount() - 1);

	while (pane->getLayerCount() > 0) {
	    m_document->removeLayerFromView
		(pane, pane->getLayer(pane->getLayerCount() - 1));
	}

	m_overview->unregisterView(pane);
	m_paneStack->deletePane(pane);
    }

    delete m_document;
    m_document = 0;
    m_viewManager->clearSelections();
    m_timeRulerLayer = 0; // document owned this

    m_sessionFile = "";
    setWindowTitle(tr("Sonic Visualiser"));

    CommandHistory::getInstance()->clear();
    CommandHistory::getInstance()->documentSaved();
    documentRestored();
}

void
MainWindow::openSession()
{
    if (!checkSaveModified()) return;

    QString orig = m_audioFile;
    if (orig == "") orig = ".";
    else orig = QFileInfo(orig).absoluteDir().canonicalPath();

    QString path = getOpenFileName(FileFinder::SessionFile);

    if (path.isEmpty()) return;

    if (openSessionFile(path) == FileOpenFailed) {
	QMessageBox::critical(this, tr("Failed to open file"),
			      tr("Session file \"%1\" could not be opened").arg(path));
    }
}

void
MainWindow::openSomething()
{
    QString orig = m_audioFile;
    if (orig == "") orig = ".";
    else orig = QFileInfo(orig).absoluteDir().canonicalPath();

    bool canImportLayer = (getMainModel() != 0 &&
                           m_paneStack != 0 &&
                           m_paneStack->getCurrentPane() != 0);

    QString path = getOpenFileName(FileFinder::AnyFile);

    if (path.isEmpty()) return;

    if (path.endsWith(".sv")) {

        if (!checkSaveModified()) return;

        if (openSessionFile(path) == FileOpenFailed) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("Session file \"%1\" could not be opened").arg(path));
        }

    } else {

        if (openAudioFile(path, AskUser) == FileOpenFailed) {

            if (!canImportLayer || (openLayerFile(path) == FileOpenFailed)) {

                QMessageBox::critical(this, tr("Failed to open file"),
                                      tr("File \"%1\" could not be opened").arg(path));
            }
        }
    }
}

void
MainWindow::openLocation()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    QString lastLocation = settings.value("lastremote", "").toString();

    bool ok = false;
    QString text = QInputDialog::getText
        (this, tr("Open Location"),
         tr("Please enter the URL of the location to open:"),
         QLineEdit::Normal, lastLocation, &ok);

    if (!ok) return;

    settings.setValue("lastremote", text);

    if (text.isEmpty()) return;

    if (openURL(QUrl(text)) == FileOpenFailed) {
        QMessageBox::critical(this, tr("Failed to open location"),
                              tr("URL \"%1\" could not be opened").arg(text));
    }
}

void
MainWindow::openRecentFile()
{
    QObject *obj = sender();
    QAction *action = dynamic_cast<QAction *>(obj);
    
    if (!action) {
	std::cerr << "WARNING: MainWindow::openRecentFile: sender is not an action"
		  << std::endl;
	return;
    }

    QString path = action->text();
    if (path == "") return;

    QUrl url(path);
    if (RemoteFile::canHandleScheme(url)) {
        openURL(url);
        return;
    }

    if (path.endsWith("sv")) {

        if (!checkSaveModified()) return;

        if (openSessionFile(path) == FileOpenFailed) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("Session file \"%1\" could not be opened").arg(path));
        }

    } else {

        if (openAudioFile(path, AskUser) == FileOpenFailed) {

            bool canImportLayer = (getMainModel() != 0 &&
                                   m_paneStack != 0 &&
                                   m_paneStack->getCurrentPane() != 0);

            if (!canImportLayer || (openLayerFile(path) == FileOpenFailed)) {

                QMessageBox::critical(this, tr("Failed to open file"),
                                      tr("File \"%1\" could not be opened").arg(path));
            }
        }
    }
}

MainWindow::FileOpenStatus
MainWindow::openURL(QUrl url)
{
    if (url.scheme().toLower() == "file") {
        return openSomeFile(url.toLocalFile());
    } else if (!RemoteFile::canHandleScheme(url)) {
        QMessageBox::critical(this, tr("Unsupported scheme in URL"),
                              tr("The URL scheme \"%1\" is not supported")
                              .arg(url.scheme()));
        return FileOpenFailed;
    } else {
        RemoteFile rf(url);
        rf.wait();
        if (!rf.isOK()) {
            QMessageBox::critical(this, tr("File download failed"),
                                  tr("Failed to download URL \"%1\": %2")
                                  .arg(url.toString()).arg(rf.getErrorString()));
            return FileOpenFailed;
        }
        FileOpenStatus status;
        if ((status = openSomeFile(rf.getLocalFilename(), url.toString())) !=
            FileOpenSucceeded) {
            rf.deleteLocalFile();
        }
        return status;
    }
}

MainWindow::FileOpenStatus
MainWindow::openSomeFile(QString path, AudioFileOpenMode mode)
{
    return openSomeFile(path, path, mode);
}

MainWindow::FileOpenStatus
MainWindow::openSomeFile(QString path, QString location,
                         AudioFileOpenMode mode)
{
    FileOpenStatus status;

    bool canImportLayer = (getMainModel() != 0 &&
                           m_paneStack != 0 &&
                           m_paneStack->getCurrentPane() != 0);

    if ((status = openAudioFile(path, location, mode)) != FileOpenFailed) {
        return status;
    } else if ((status = openSessionFile(path, location)) != FileOpenFailed) {
	return status;
    } else if (!canImportLayer) {
        return FileOpenFailed;
    } else if ((status = openLayerFile(path, location)) != FileOpenFailed) {
        return status;
    } else {
	return FileOpenFailed;
    }
}

MainWindow::FileOpenStatus
MainWindow::openSessionFile(QString path)
{
    return openSessionFile(path, path);
}

MainWindow::FileOpenStatus
MainWindow::openSessionFile(QString path, QString location)
{
    BZipFileDevice bzFile(path);
    if (!bzFile.open(QIODevice::ReadOnly)) {
        std::cerr << "Failed to open session file \"" << location.toStdString()
                  << "\": " << bzFile.errorString().toStdString() << std::endl;
        return FileOpenFailed;
    }

    if (!checkSaveModified()) return FileOpenCancelled;

    QString error;
    closeSession();
    createDocument();

    PaneCallback callback(this);
    m_viewManager->clearSelections();

    SVFileReader reader(m_document, callback, location);
    QXmlInputSource inputSource(&bzFile);
    reader.parse(inputSource);
    
    if (!reader.isOK()) {
        error = tr("SV XML file read error:\n%1").arg(reader.getErrorString());
    }
    
    bzFile.close();

    bool ok = (error == "");

    bool realFile = (location == path);
    
    if (ok) {

	setWindowTitle(tr("Sonic Visualiser: %1")
		       .arg(QFileInfo(location).fileName()));

	if (realFile) m_sessionFile = path;

	setupMenus();
	CommandHistory::getInstance()->clear();
	CommandHistory::getInstance()->documentSaved();
	m_documentModified = false;
	updateMenuStates();

        m_recentFiles.addFile(location);

        if (realFile) {
            registerLastOpenedFilePath(FileFinder::SessionFile, path); // for file dialog
        }

    } else {
	setWindowTitle(tr("Sonic Visualiser"));
    }

    return ok ? FileOpenSucceeded : FileOpenFailed;
}

void
MainWindow::closeEvent(QCloseEvent *e)
{
//    std::cerr << "MainWindow::closeEvent" << std::endl;

    if (m_openingAudioFile) {
//        std::cerr << "Busy - ignoring close event" << std::endl;
	e->ignore();
	return;
    }

    if (!m_abandoning && !checkSaveModified()) {
//        std::cerr << "Ignoring close event" << std::endl;
	e->ignore();
	return;
    }

    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("position", pos());
    settings.endGroup();

    e->accept();
    return;
}

bool
MainWindow::commitData(bool mayAskUser)
{
    if (mayAskUser) {
        return checkSaveModified();
    } else {
        if (!m_documentModified) return true;

        // If we can't check with the user first, then we can't save
        // to the original session file (even if we have it) -- have
        // to use a temporary file

        QString svDirBase = ".sv1";
        QString svDir = QDir::home().filePath(svDirBase);

        if (!QFileInfo(svDir).exists()) {
            if (!QDir::home().mkdir(svDirBase)) return false;
        } else {
            if (!QFileInfo(svDir).isDir()) return false;
        }
        
        // This name doesn't have to be unguessable
#ifndef _WIN32
        QString fname = QString("tmp-%1-%2.sv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
            .arg(QProcess().pid());
#else
        QString fname = QString("tmp-%1.sv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"));
#endif
        QString fpath = QDir(svDir).filePath(fname);
        if (saveSessionFile(fpath)) {
            m_recentFiles.addFile(fpath);
            return true;
        } else {
            return false;
        }
    }
}

bool
MainWindow::checkSaveModified()
{
    // Called before some destructive operation (e.g. new session,
    // exit program).  Return true if we can safely proceed, false to
    // cancel.

    if (!m_documentModified) return true;

    int button = 
	QMessageBox::warning(this,
			     tr("Session modified"),
			     tr("The current session has been modified.\nDo you want to save it?"),
			     QMessageBox::Yes,
			     QMessageBox::No,
			     QMessageBox::Cancel);

    if (button == QMessageBox::Yes) {
	saveSession();
	if (m_documentModified) { // save failed -- don't proceed!
	    return false;
	} else {
            return true; // saved, so it's safe to continue now
        }
    } else if (button == QMessageBox::No) {
	m_documentModified = false; // so we know to abandon it
	return true;
    }

    // else cancel
    return false;
}

void
MainWindow::saveSession()
{
    if (m_sessionFile != "") {
	if (!saveSessionFile(m_sessionFile)) {
	    QMessageBox::critical(this, tr("Failed to save file"),
				  tr("Session file \"%1\" could not be saved.").arg(m_sessionFile));
	} else {
	    CommandHistory::getInstance()->documentSaved();
	    documentRestored();
	}
    } else {
	saveSessionAs();
    }
}

void
MainWindow::saveSessionAs()
{
    QString orig = m_audioFile;
    if (orig == "") orig = ".";
    else orig = QFileInfo(orig).absoluteDir().canonicalPath();

    QString path = getSaveFileName(FileFinder::SessionFile);

    if (path == "") return;

    if (!saveSessionFile(path)) {
	QMessageBox::critical(this, tr("Failed to save file"),
			      tr("Session file \"%1\" could not be saved.").arg(path));
    } else {
	setWindowTitle(tr("Sonic Visualiser: %1")
		       .arg(QFileInfo(path).fileName()));
	m_sessionFile = path;
	CommandHistory::getInstance()->documentSaved();
	documentRestored();
        m_recentFiles.addFile(path);
    }
}

bool
MainWindow::saveSessionFile(QString path)
{
    BZipFileDevice bzFile(path);
    if (!bzFile.open(QIODevice::WriteOnly)) {
        std::cerr << "Failed to open session file \"" << path.toStdString()
                  << "\" for writing: "
                  << bzFile.errorString().toStdString() << std::endl;
        return false;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QTextStream out(&bzFile);
    toXml(out);
    out.flush();

    QApplication::restoreOverrideCursor();

    if (!bzFile.isOK()) {
	QMessageBox::critical(this, tr("Failed to write file"),
			      tr("Failed to write to file \"%1\": %2")
			      .arg(path).arg(bzFile.errorString()));
        bzFile.close();
	return false;
    }

    bzFile.close();
    return true;
}

void
MainWindow::toXml(QTextStream &out)
{
    QString indent("  ");

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<!DOCTYPE sonic-visualiser>\n";
    out << "<sv>\n";

    m_document->toXml(out, "", "");

    out << "<display>\n";

    out << QString("  <window width=\"%1\" height=\"%2\"/>\n")
	.arg(width()).arg(height());

    for (int i = 0; i < m_paneStack->getPaneCount(); ++i) {

	Pane *pane = m_paneStack->getPane(i);

	if (pane) {
            pane->toXml(out, indent);
	}
    }

    out << "</display>\n";

    m_viewManager->getSelection().toXml(out);

    out << "</sv>\n";
}

Pane *
MainWindow::addPaneToStack()
{
    AddPaneCommand *command = new AddPaneCommand(this);
    CommandHistory::getInstance()->addCommand(command);
    return command->getPane();
}

void
MainWindow::zoomIn()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->zoom(true);
}

void
MainWindow::zoomOut()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->zoom(false);
}

void
MainWindow::zoomToFit()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    Model *model = getMainModel();
    if (!model) return;
    
    size_t start = model->getStartFrame();
    size_t end = model->getEndFrame();
    size_t pixels = currentPane->width();
    size_t zoomLevel = (end - start) / pixels;

    currentPane->setZoomLevel(zoomLevel);
    currentPane->setStartFrame(start);
}

void
MainWindow::zoomDefault()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->setZoomLevel(1024);
}

void
MainWindow::scrollLeft()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(false, false);
}

void
MainWindow::jumpLeft()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(false, true);
}

void
MainWindow::scrollRight()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(true, false);
}

void
MainWindow::jumpRight()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(true, true);
}

void
MainWindow::showNoOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::NoOverlays);
}

void
MainWindow::showMinimalOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::MinimalOverlays);
}

void
MainWindow::showStandardOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::StandardOverlays);
}

void
MainWindow::showAllOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::AllOverlays);
}

void
MainWindow::toggleZoomWheels()
{
    if (m_viewManager->getZoomWheelsEnabled()) {
        m_viewManager->setZoomWheelsEnabled(false);
    } else {
        m_viewManager->setZoomWheelsEnabled(true);
    }
}

void
MainWindow::togglePropertyBoxes()
{
    if (m_paneStack->getLayoutStyle() == PaneStack::NoPropertyStacks) {
        if (Preferences::getInstance()->getPropertyBoxLayout() ==
            Preferences::VerticallyStacked) {
            m_paneStack->setLayoutStyle(PaneStack::PropertyStackPerPaneLayout);
        } else {
            m_paneStack->setLayoutStyle(PaneStack::SinglePropertyStackLayout);
        }
    } else {
        m_paneStack->setLayoutStyle(PaneStack::NoPropertyStacks);
    }
}

void
MainWindow::toggleStatusBar()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    bool sb = settings.value("showstatusbar", true).toBool();

    if (sb) {
        statusBar()->hide();
    } else {
        statusBar()->show();
    }

    settings.setValue("showstatusbar", !sb);

    settings.endGroup();
}

void
MainWindow::preferenceChanged(PropertyContainer::PropertyName name)
{
    if (name == "Property Box Layout") {
        if (m_paneStack->getLayoutStyle() != PaneStack::NoPropertyStacks) {
            if (Preferences::getInstance()->getPropertyBoxLayout() ==
                Preferences::VerticallyStacked) {
                m_paneStack->setLayoutStyle(PaneStack::PropertyStackPerPaneLayout);
            } else {
                m_paneStack->setLayoutStyle(PaneStack::SinglePropertyStackLayout);
            }
        }
    }
}

void
MainWindow::play()
{
    if (m_playSource->isPlaying()) {
        stop();
    } else {
        playbackFrameChanged(m_viewManager->getPlaybackFrame());
	m_playSource->play(m_viewManager->getPlaybackFrame());
    }
}

void
MainWindow::ffwd()
{
    if (!getMainModel()) return;

    int frame = m_viewManager->getPlaybackFrame();
    ++frame;

    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return;

    Layer *layer = pane->getSelectedLayer();

    if (!dynamic_cast<TimeInstantLayer *>(layer) &&
        !dynamic_cast<TimeValueLayer *>(layer)) return;

    size_t resolution = 0;
    if (!layer->snapToFeatureFrame(pane, frame, resolution, Layer::SnapRight)) {
        frame = getMainModel()->getEndFrame();
    }
    
    m_viewManager->setPlaybackFrame(frame);
}

void
MainWindow::ffwdEnd()
{
    if (!getMainModel()) return;
    m_viewManager->setPlaybackFrame(getMainModel()->getEndFrame());
}

void
MainWindow::rewind()
{
    if (!getMainModel()) return;

    int frame = m_viewManager->getPlaybackFrame();
    if (frame > 0) --frame;

    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return;

    Layer *layer = pane->getSelectedLayer();

    if (!dynamic_cast<TimeInstantLayer *>(layer) &&
        !dynamic_cast<TimeValueLayer *>(layer)) return;

    size_t resolution = 0;
    if (!layer->snapToFeatureFrame(pane, frame, resolution, Layer::SnapLeft)) {
        frame = getMainModel()->getEndFrame();
    }
    
    m_viewManager->setPlaybackFrame(frame);
}

void
MainWindow::rewindStart()
{
    if (!getMainModel()) return;
    m_viewManager->setPlaybackFrame(getMainModel()->getStartFrame());
}

void
MainWindow::stop()
{
    m_playSource->stop();

    if (m_paneStack && m_paneStack->getCurrentPane()) {
        updateVisibleRangeDisplay(m_paneStack->getCurrentPane());
    } else {
        m_myStatusMessage = "";
        statusBar()->showMessage("");
    }
}

void
MainWindow::addPane()
{
    QObject *s = sender();
    QAction *action = dynamic_cast<QAction *>(s);
    
    if (!action) {
	std::cerr << "WARNING: MainWindow::addPane: sender is not an action"
		  << std::endl;
	return;
    }

    PaneActionMap::iterator i = m_paneActions.find(action);

    if (i == m_paneActions.end()) {
	std::cerr << "WARNING: MainWindow::addPane: unknown action "
		  << action->objectName().toStdString() << std::endl;
	return;
    }

    addPane(i->second, action->text());
}

void
MainWindow::addPane(const PaneConfiguration &configuration, QString text)
{
    CommandHistory::getInstance()->startCompoundOperation(text, true);

    AddPaneCommand *command = new AddPaneCommand(this);
    CommandHistory::getInstance()->addCommand(command);

    Pane *pane = command->getPane();

    if (configuration.layer == LayerFactory::Spectrum) {
        pane->setPlaybackFollow(PlaybackScrollContinuous);
        pane->setFollowGlobalZoom(false);
        pane->setZoomLevel(512);
    }

    if (configuration.layer != LayerFactory::TimeRuler &&
        configuration.layer != LayerFactory::Spectrum) {

	if (!m_timeRulerLayer) {
//	    std::cerr << "no time ruler layer, creating one" << std::endl;
	    m_timeRulerLayer = m_document->createMainModelLayer
		(LayerFactory::TimeRuler);
	}

//	std::cerr << "adding time ruler layer " << m_timeRulerLayer << std::endl;

	m_document->addLayerToView(pane, m_timeRulerLayer);
    }

    Layer *newLayer = m_document->createLayer(configuration.layer);

    Model *suggestedModel = configuration.sourceModel;
    Model *model = 0;

    if (suggestedModel) {

        // check its validity
        std::vector<Model *> inputModels = m_document->getTransformInputModels();
        for (size_t j = 0; j < inputModels.size(); ++j) {
            if (inputModels[j] == suggestedModel) {
                model = suggestedModel;
                break;
            }
        }

        if (!model) {
            std::cerr << "WARNING: Model " << (void *)suggestedModel
                      << " appears in pane action map, but is not reported "
                      << "by document as a valid transform source" << std::endl;
        }
    }

    if (!model) model = m_document->getMainModel();

    m_document->setModel(newLayer, model);

    m_document->setChannel(newLayer, configuration.channel);
    m_document->addLayerToView(pane, newLayer);

    m_paneStack->setCurrentPane(pane);
    m_paneStack->setCurrentLayer(pane, newLayer);

//    std::cerr << "MainWindow::addPane: global centre frame is "
//              << m_viewManager->getGlobalCentreFrame() << std::endl;
//    pane->setCentreFrame(m_viewManager->getGlobalCentreFrame());

    CommandHistory::getInstance()->endCompoundOperation();

    updateMenuStates();
}

MainWindow::AddPaneCommand::AddPaneCommand(MainWindow *mw) :
    m_mw(mw),
    m_pane(0),
    m_prevCurrentPane(0),
    m_added(false)
{
}

MainWindow::AddPaneCommand::~AddPaneCommand()
{
    if (m_pane && !m_added) {
	m_mw->m_paneStack->deletePane(m_pane);
    }
}

QString
MainWindow::AddPaneCommand::getName() const
{
    return tr("Add Pane");
}

void
MainWindow::AddPaneCommand::execute()
{
    if (!m_pane) {
	m_prevCurrentPane = m_mw->m_paneStack->getCurrentPane();
	m_pane = m_mw->m_paneStack->addPane();

        connect(m_pane, SIGNAL(contextHelpChanged(const QString &)),
                m_mw, SLOT(contextHelpChanged(const QString &)));
    } else {
	m_mw->m_paneStack->showPane(m_pane);
    }

    m_mw->m_paneStack->setCurrentPane(m_pane);
    m_mw->m_overview->registerView(m_pane);
    m_added = true;
}

void
MainWindow::AddPaneCommand::unexecute()
{
    m_mw->m_paneStack->hidePane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_mw->m_overview->unregisterView(m_pane); 
    m_added = false;
}

MainWindow::RemovePaneCommand::RemovePaneCommand(MainWindow *mw, Pane *pane) :
    m_mw(mw),
    m_pane(pane),
    m_added(true)
{
}

MainWindow::RemovePaneCommand::~RemovePaneCommand()
{
    if (m_pane && !m_added) {
	m_mw->m_paneStack->deletePane(m_pane);
    }
}

QString
MainWindow::RemovePaneCommand::getName() const
{
    return tr("Remove Pane");
}

void
MainWindow::RemovePaneCommand::execute()
{
    m_prevCurrentPane = m_mw->m_paneStack->getCurrentPane();
    m_mw->m_paneStack->hidePane(m_pane);
    m_mw->m_overview->unregisterView(m_pane);
    m_added = false;
}

void
MainWindow::RemovePaneCommand::unexecute()
{
    m_mw->m_paneStack->showPane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_mw->m_overview->registerView(m_pane);
    m_added = true;
}

void
MainWindow::addLayer()
{
    QObject *s = sender();
    QAction *action = dynamic_cast<QAction *>(s);
    
    if (!action) {
	std::cerr << "WARNING: MainWindow::addLayer: sender is not an action"
		  << std::endl;
	return;
    }

    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	std::cerr << "WARNING: MainWindow::addLayer: no current pane" << std::endl;
	return;
    }

    ExistingLayerActionMap::iterator ei = m_existingLayerActions.find(action);

    if (ei != m_existingLayerActions.end()) {
	Layer *newLayer = ei->second;
	m_document->addLayerToView(pane, newLayer);
	m_paneStack->setCurrentLayer(pane, newLayer);
	return;
    }

    ei = m_sliceActions.find(action);

    if (ei != m_sliceActions.end()) {
        Layer *newLayer = m_document->createLayer(LayerFactory::Slice);
//        document->setModel(newLayer, ei->second->getModel());
        SliceableLayer *source = dynamic_cast<SliceableLayer *>(ei->second);
        SliceLayer *dest = dynamic_cast<SliceLayer *>(newLayer);
        if (source && dest) {
            dest->setSliceableModel(source->getSliceableModel());
            connect(source, SIGNAL(sliceableModelReplaced(const Model *, const Model *)),
                    dest, SLOT(sliceableModelReplaced(const Model *, const Model *)));
            connect(m_document, SIGNAL(modelAboutToBeDeleted(Model *)),
                    dest, SLOT(modelAboutToBeDeleted(Model *)));
        }
	m_document->addLayerToView(pane, newLayer);
	m_paneStack->setCurrentLayer(pane, newLayer);
	return;
    }

    TransformActionMap::iterator i = m_transformActions.find(action);

    if (i == m_transformActions.end()) {

	LayerActionMap::iterator i = m_layerActions.find(action);
	
	if (i == m_layerActions.end()) {
	    std::cerr << "WARNING: MainWindow::addLayer: unknown action "
		      << action->objectName().toStdString() << std::endl;
	    return;
	}

	LayerFactory::LayerType type = i->second;
	
	LayerFactory::LayerTypeSet emptyTypes =
	    LayerFactory::getInstance()->getValidEmptyLayerTypes();

	Layer *newLayer;

	if (emptyTypes.find(type) != emptyTypes.end()) {

	    newLayer = m_document->createEmptyLayer(type);
	    m_toolActions[ViewManager::DrawMode]->trigger();

	} else {

	    newLayer = m_document->createMainModelLayer(type);
	}

	m_document->addLayerToView(pane, newLayer);
	m_paneStack->setCurrentLayer(pane, newLayer);

	return;
    }

    TransformId transform = i->second;
    TransformFactory *factory = TransformFactory::getInstance();

    QString configurationXml;

    int channel = -1;
    // pick up the default channel from any existing layers on the same pane
    for (int j = 0; j < pane->getLayerCount(); ++j) {
	int c = LayerFactory::getInstance()->getChannel(pane->getLayer(j));
	if (c != -1) {
	    channel = c;
	    break;
	}
    }

    // We always ask for configuration, even if the plugin isn't
    // supposed to be configurable, because we need to let the user
    // change the execution context (block size etc).

    PluginTransform::ExecutionContext context(channel);

    std::vector<Model *> candidateInputModels =
        m_document->getTransformInputModels();

    Model *inputModel = factory->getConfigurationForTransform(transform,
                                                              candidateInputModels,
                                                              context,
                                                              configurationXml,
                                                              m_playSource);
    if (!inputModel) return;

//    std::cerr << "MainWindow::addLayer: Input model is " << inputModel << " \"" << inputModel->objectName().toStdString() << "\"" << std::endl;

    Layer *newLayer = m_document->createDerivedLayer(transform,
                                                     inputModel,
                                                     context,
                                                     configurationXml);

    if (newLayer) {
        m_document->addLayerToView(pane, newLayer);
        m_document->setChannel(newLayer, context.channel);
        m_recentTransforms.add(transform);
        m_paneStack->setCurrentLayer(pane, newLayer);
    }

    updateMenuStates();
}

void
MainWindow::deleteCurrentPane()
{
    CommandHistory::getInstance()->startCompoundOperation
	(tr("Delete Pane"), true);

    Pane *pane = m_paneStack->getCurrentPane();
    if (pane) {
	while (pane->getLayerCount() > 0) {
	    Layer *layer = pane->getLayer(0);
	    if (layer) {
		m_document->removeLayerFromView(pane, layer);
	    } else {
		break;
	    }
	}

	RemovePaneCommand *command = new RemovePaneCommand(this, pane);
	CommandHistory::getInstance()->addCommand(command);
    }

    CommandHistory::getInstance()->endCompoundOperation();

    updateMenuStates();
}

void
MainWindow::deleteCurrentLayer()
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (pane) {
	Layer *layer = pane->getSelectedLayer();
	if (layer) {
	    m_document->removeLayerFromView(pane, layer);
	}
    }
    updateMenuStates();
}

void
MainWindow::renameCurrentLayer()
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (pane) {
	Layer *layer = pane->getSelectedLayer();
	if (layer) {
	    bool ok = false;
	    QString newName = QInputDialog::getText
		(this, tr("Rename Layer"),
		 tr("New name for this layer:"),
		 QLineEdit::Normal, layer->objectName(), &ok);
	    if (ok) {
		layer->setObjectName(newName);
		setupExistingLayersMenus();
	    }
	}
    }
}

void
MainWindow::playSpeedChanged(int position)
{
    PlaySpeedRangeMapper mapper(0, 200);

    float percent = m_playSpeed->mappedValue();

    float factor = mapper.getFactorForValue(percent);

//    float factor = mapper.getFactorForPosition(position);
//    float percent = mapper.getValueForPosition(position);

    std::cerr << "speed = " << position << " percent = " << percent << " factor = " << factor << std::endl;

//!!!    bool slow = (position < 100);
    bool something = (position != 100);
/*!!!
    int pc = lrintf(percent);

    m_playSpeed->setToolTip(tr("Playback speed: %1%2%")
                            .arg(!slow ? "+" : "")
			    .arg(pc));
*/
    m_playSharpen->setEnabled(something);
    m_playMono->setEnabled(something);
    bool sharpen = (something && m_playSharpen->isChecked());
    bool mono = (something && m_playMono->isChecked());
    m_playSource->setTimeStretch(factor, sharpen, mono);
}

void
MainWindow::playSharpenToggled()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("playsharpen", m_playSharpen->isChecked());
    settings.endGroup();

    playSpeedChanged(m_playSpeed->value());
}

void
MainWindow::playMonoToggled()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("playmono", m_playMono->isChecked());
    settings.endGroup();

    playSpeedChanged(m_playSpeed->value());
}    

void
MainWindow::playbackFrameChanged(unsigned long frame)
{
    if (!(m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;

    RealTime now = RealTime::frame2RealTime
        (frame, getMainModel()->getSampleRate());

    if (now.sec == m_lastPlayStatusSec) return;

    RealTime then = RealTime::frame2RealTime
        (m_playSource->getPlayEndFrame(), getMainModel()->getSampleRate());

    QString nowStr;
    QString thenStr;
    QString remainingStr;

    if (then.sec > 10) {
        nowStr = now.toSecText().c_str();
        thenStr = then.toSecText().c_str();
        remainingStr = (then - now).toSecText().c_str();
        m_lastPlayStatusSec = now.sec;
    } else {
        nowStr = now.toText(true).c_str();
        thenStr = then.toText(true).c_str();
        remainingStr = (then - now).toText(true).c_str();
    }        

    m_myStatusMessage = tr("Playing: %1 of %2 (%3 remaining)")
        .arg(nowStr).arg(thenStr).arg(remainingStr);

    statusBar()->showMessage(m_myStatusMessage);
}

void
MainWindow::globalCentreFrameChanged(unsigned long )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (!p->getFollowGlobalPan()) return;
    updateVisibleRangeDisplay(p);
}

void
MainWindow::viewCentreFrameChanged(View *v, unsigned long )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (v == p) updateVisibleRangeDisplay(p);
}

void
MainWindow::viewZoomLevelChanged(View *v, unsigned long , bool )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (v == p) updateVisibleRangeDisplay(p);
}

void
MainWindow::updateVisibleRangeDisplay(Pane *p) const
{
    if (!getMainModel() || !p) {
        return;
    }

    bool haveSelection = false;
    size_t startFrame = 0, endFrame = 0;

    if (m_viewManager && m_viewManager->haveInProgressSelection()) {

        bool exclusive = false;
        Selection s = m_viewManager->getInProgressSelection(exclusive);

        if (!s.isEmpty()) {
            haveSelection = true;
            startFrame = s.getStartFrame();
            endFrame = s.getEndFrame();
        }
    }

    if (!haveSelection) {
        startFrame = p->getFirstVisibleFrame();
        endFrame = p->getLastVisibleFrame();
    }

    RealTime start = RealTime::frame2RealTime
        (startFrame, getMainModel()->getSampleRate());

    RealTime end = RealTime::frame2RealTime
        (endFrame, getMainModel()->getSampleRate());

    RealTime duration = end - start;

    QString startStr, endStr, durationStr;
    startStr = start.toText(true).c_str();
    endStr = end.toText(true).c_str();
    durationStr = duration.toText(true).c_str();

    if (haveSelection) {
        m_myStatusMessage = tr("Selection: %1 to %2 (duration %3)")
            .arg(startStr).arg(endStr).arg(durationStr);
    } else {
        m_myStatusMessage = tr("Visible: %1 to %2 (duration %3)")
            .arg(startStr).arg(endStr).arg(durationStr);
    }

    statusBar()->showMessage(m_myStatusMessage);
}

void
MainWindow::outputLevelsChanged(float left, float right)
{
    m_fader->setPeakLeft(left);
    m_fader->setPeakRight(right);
}

void
MainWindow::sampleRateMismatch(size_t requested, size_t actual,
                               bool willResample)
{
    if (!willResample) {
        //!!! more helpful message needed
        QMessageBox::information
            (this, tr("Sample rate mismatch"),
             tr("The sample rate of this audio file (%1 Hz) does not match\nthe current playback rate (%2 Hz).\n\nThe file will play at the wrong speed and pitch.")
             .arg(requested).arg(actual));
    }        

    updateDescriptionLabel();
}

void
MainWindow::audioOverloadPluginDisabled()
{
    QMessageBox::information
        (this, tr("Audio processing overload"),
         tr("Audio effects plugin auditioning has been disabled\ndue to a processing overload."));
}

void
MainWindow::layerAdded(Layer *)
{
//    std::cerr << "MainWindow::layerAdded(" << layer << ")" << std::endl;
//    setupExistingLayersMenu();
    updateMenuStates();
}

void
MainWindow::layerRemoved(Layer *)
{
//    std::cerr << "MainWindow::layerRemoved(" << layer << ")" << std::endl;
    setupExistingLayersMenus();
    updateMenuStates();
}

void
MainWindow::layerAboutToBeDeleted(Layer *layer)
{
//    std::cerr << "MainWindow::layerAboutToBeDeleted(" << layer << ")" << std::endl;
    if (layer == m_timeRulerLayer) {
//	std::cerr << "(this is the time ruler layer)" << std::endl;
	m_timeRulerLayer = 0;
    }
}

void
MainWindow::layerInAView(Layer *layer, bool inAView)
{
//    std::cerr << "MainWindow::layerInAView(" << layer << "," << inAView << ")" << std::endl;

    // Check whether we need to add or remove model from play source
    Model *model = layer->getModel();
    if (model) {
        if (inAView) {
            m_playSource->addModel(model);
        } else {
            bool found = false;
            for (int i = 0; i < m_paneStack->getPaneCount(); ++i) {
                Pane *pane = m_paneStack->getPane(i);
                if (!pane) continue;
                for (int j = 0; j < pane->getLayerCount(); ++j) {
                    Layer *pl = pane->getLayer(j);
                    if (pl && pl->getModel() == model) {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) m_playSource->removeModel(model);
        }
    }

    setupExistingLayersMenus();
    updateMenuStates();
}

void
MainWindow::modelAdded(Model *model)
{
//    std::cerr << "MainWindow::modelAdded(" << model << ")" << std::endl;
    m_playSource->addModel(model);
    if (dynamic_cast<DenseTimeValueModel *>(model)) {
        setupPaneAndLayerMenus();
    }
}

void
MainWindow::mainModelChanged(WaveFileModel *model)
{
//    std::cerr << "MainWindow::mainModelChanged(" << model << ")" << std::endl;
    updateDescriptionLabel();
    m_panLayer->setModel(model);
    if (model) m_viewManager->setMainModelSampleRate(model->getSampleRate());
    if (model && !m_playTarget && m_audioOutput) createPlayTarget();
}

void
MainWindow::modelAboutToBeDeleted(Model *model)
{
//    std::cerr << "MainWindow::modelAboutToBeDeleted(" << model << ")" << std::endl;
    m_playSource->removeModel(model);
    FFTDataServer::modelAboutToBeDeleted(model);
}

void
MainWindow::modelGenerationFailed(QString transformName)
{
    QMessageBox::warning
        (this,
         tr("Failed to generate layer"),
         tr("Failed to generate a derived layer.\n\nThe layer transform \"%1\" failed.\n\nThis probably means that a plugin failed to initialise, perhaps because it\nrejected the processing block size that was requested.")
         .arg(transformName),
         QMessageBox::Ok, 0);
}

void
MainWindow::modelRegenerationFailed(QString layerName, QString transformName)
{
    QMessageBox::warning
        (this,
         tr("Failed to regenerate layer"),
         tr("Failed to regenerate derived layer \"%1\".\n\nThe layer transform \"%2\" failed to run.\n\nThis probably means the layer used a plugin that is not currently available.")
         .arg(layerName).arg(transformName),
         QMessageBox::Ok, 0);
}

void
MainWindow::rightButtonMenuRequested(Pane *pane, QPoint position)
{
//    std::cerr << "MainWindow::rightButtonMenuRequested(" << pane << ", " << position.x() << ", " << position.y() << ")" << std::endl;
    m_paneStack->setCurrentPane(pane);
    m_rightButtonMenu->popup(position);
}

void
MainWindow::propertyStacksResized()
{
/*
    std::cerr << "MainWindow::propertyStacksResized" << std::endl;
    Pane *pane = m_paneStack->getCurrentPane();
    if (pane && m_overview) {
        std::cerr << "Fixed width: "  << pane->width() << std::endl;
        m_overview->setFixedWidth(pane->width());
    }
*/
}

void
MainWindow::showLayerTree()
{
    QTreeView *view = new QTreeView();
    LayerTreeModel *tree = new LayerTreeModel(m_paneStack);
    view->expand(tree->index(0, 0, QModelIndex()));
    view->setModel(tree);
    view->show();
}

void
MainWindow::pollOSC()
{
    if (!m_oscQueue || m_oscQueue->isEmpty()) return;
    std::cerr << "MainWindow::pollOSC: have " << m_oscQueue->getMessagesAvailable() << " messages" << std::endl;

    if (m_openingAudioFile) return;

    OSCMessage message = m_oscQueue->readMessage();

    if (message.getTarget() != 0) {
        return; //!!! for now -- this class is target 0, others not handled yet
    }

    handleOSCMessage(message);
}

void
MainWindow::handleOSCMessage(const OSCMessage &message)
{
    std::cerr << "MainWindow::handleOSCMessage: thread id = " 
              << QThread::currentThreadId() << std::endl;

    // This large function should really be abstracted out.

    if (message.getMethod() == "open") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            QString path = message.getArg(0).toString();
            if (openSomeFile(path, ReplaceMainModel) != FileOpenSucceeded) {
                std::cerr << "MainWindow::handleOSCMessage: File open failed for path \""
                          << path.toStdString() << "\"" << std::endl;
            }
            //!!! we really need to spin here and not return until the
            // file has been completely decoded...
        }

    } else if (message.getMethod() == "openadditional") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            QString path = message.getArg(0).toString();
            if (openSomeFile(path, CreateAdditionalModel) != FileOpenSucceeded) {
                std::cerr << "MainWindow::handleOSCMessage: File open failed for path \""
                          << path.toStdString() << "\"" << std::endl;
            }
        }

    } else if (message.getMethod() == "recent" ||
               message.getMethod() == "last") {

        int n = 0;
        if (message.getMethod() == "recent" &&
            message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::Int)) {
            n = message.getArg(0).toInt() - 1;
        }
        std::vector<QString> recent = m_recentFiles.getRecent();
        if (n >= 0 && n < int(recent.size())) {
            if (openSomeFile(recent[n], ReplaceMainModel) != FileOpenSucceeded) {
                std::cerr << "MainWindow::handleOSCMessage: File open failed for path \""
                          << recent[n].toStdString() << "\"" << std::endl;
            }
        }

    } else if (message.getMethod() == "save") {

        QString path;
        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            path = message.getArg(0).toString();
            if (QFileInfo(path).exists()) {
                std::cerr << "MainWindow::handleOSCMessage: Refusing to overwrite existing file in save" << std::endl;
            } else {
                saveSessionFile(path);
            }
        }

    } else if (message.getMethod() == "export") {

        QString path;
        if (getMainModel()) {
            if (message.getArgCount() == 1 &&
                message.getArg(0).canConvert(QVariant::String)) {
                path = message.getArg(0).toString();
                if (QFileInfo(path).exists()) {
                    std::cerr << "MainWindow::handleOSCMessage: Refusing to overwrite existing file in export" << std::endl;
                } else {
                    WavFileWriter writer(path,
                                         getMainModel()->getSampleRate(),
                                         getMainModel()->getChannelCount());
                    MultiSelection ms = m_viewManager->getSelection();
                    if (!ms.getSelections().empty()) {
                        writer.writeModel(getMainModel(), &ms);
                    } else {
                        writer.writeModel(getMainModel());
                    }
                }
            }
        }

    } else if (message.getMethod() == "jump" ||
               message.getMethod() == "play") {

        if (getMainModel()) {

            unsigned long frame = m_viewManager->getPlaybackFrame();
            bool selection = false;
            bool play = (message.getMethod() == "play");

            if (message.getArgCount() == 1) {

                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "selection") {

                    selection = true;

                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "end") {

                    frame = getMainModel()->getEndFrame();

                } else if (message.getArg(0).canConvert(QVariant::Double)) {

                    double time = message.getArg(0).toDouble();
                    if (time < 0.0) time = 0.0;

                    frame = lrint(time * getMainModel()->getSampleRate());
                }
            }

            if (frame > getMainModel()->getEndFrame()) {
                frame = getMainModel()->getEndFrame();
            }

            if (play) {
                m_viewManager->setPlaySelectionMode(selection);
            } 

            if (selection) {
                MultiSelection::SelectionList sl = m_viewManager->getSelections();
                if (!sl.empty()) {
                    frame = sl.begin()->getStartFrame();
                }
            }

            m_viewManager->setPlaybackFrame(frame);

            if (play && !m_playSource->isPlaying()) {
                m_playSource->play(frame);
            }
        }

    } else if (message.getMethod() == "stop") {
            
        if (m_playSource->isPlaying()) m_playSource->stop();

    } else if (message.getMethod() == "loop") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                m_viewManager->setPlayLoopMode(true);
            } else if (str == "off") {
                m_viewManager->setPlayLoopMode(false);
            }
        }

    } else if (message.getMethod() == "select" ||
               message.getMethod() == "addselect") {

        if (getMainModel()) {

            int f0 = getMainModel()->getStartFrame();
            int f1 = getMainModel()->getEndFrame();

            bool done = false;

            if (message.getArgCount() == 2 &&
                message.getArg(0).canConvert(QVariant::Double) &&
                message.getArg(1).canConvert(QVariant::Double)) {
                
                double t0 = message.getArg(0).toDouble();
                double t1 = message.getArg(1).toDouble();
                if (t1 < t0) { double temp = t0; t0 = t1; t1 = temp; }
                if (t0 < 0.0) t0 = 0.0;
                if (t1 < 0.0) t1 = 0.0;

                f0 = lrint(t0 * getMainModel()->getSampleRate());
                f1 = lrint(t1 * getMainModel()->getSampleRate());
                
                Pane *pane = m_paneStack->getCurrentPane();
                Layer *layer = 0;
                if (pane) layer = pane->getSelectedLayer();
                if (layer) {
                    size_t resolution;
                    layer->snapToFeatureFrame(pane, f0, resolution,
                                              Layer::SnapLeft);
                    layer->snapToFeatureFrame(pane, f1, resolution,
                                              Layer::SnapRight);
                }

            } else if (message.getArgCount() == 1 &&
                       message.getArg(0).canConvert(QVariant::String)) {

                QString str = message.getArg(0).toString();
                if (str == "none") {
                    m_viewManager->clearSelections();
                    done = true;
                }
            }

            if (!done) {
                if (message.getMethod() == "select") {
                    m_viewManager->setSelection(Selection(f0, f1));
                } else {
                    m_viewManager->addSelection(Selection(f0, f1));
                }
            }
        }

    } else if (message.getMethod() == "add") {

        if (getMainModel()) {

            if (message.getArgCount() >= 1 &&
                message.getArg(0).canConvert(QVariant::String)) {

                int channel = -1;
                if (message.getArgCount() == 2 &&
                    message.getArg(0).canConvert(QVariant::Int)) {
                    channel = message.getArg(0).toInt();
                    if (channel < -1 ||
                        channel > int(getMainModel()->getChannelCount())) {
                        std::cerr << "WARNING: MainWindow::handleOSCMessage: channel "
                                  << channel << " out of range" << std::endl;
                        channel = -1;
                    }
                }

                QString str = message.getArg(0).toString();
                
                LayerFactory::LayerType type =
                    LayerFactory::getInstance()->getLayerTypeForName(str);

                if (type == LayerFactory::UnknownLayer) {
                    std::cerr << "WARNING: MainWindow::handleOSCMessage: unknown layer "
                              << "type " << str.toStdString() << std::endl;
                } else {

                    PaneConfiguration configuration(type,
                                                    getMainModel(),
                                                    channel);
                    
                    addPane(configuration,
                            tr("Add %1 Pane")
                            .arg(LayerFactory::getInstance()->
                                 getLayerPresentationName(type)));
                }
            }
        }

    } else if (message.getMethod() == "undo") {

        CommandHistory::getInstance()->undo();

    } else if (message.getMethod() == "redo") {

        CommandHistory::getInstance()->redo();

    } else if (message.getMethod() == "set") {

        if (message.getArgCount() == 2 &&
            message.getArg(0).canConvert(QVariant::String) &&
            message.getArg(1).canConvert(QVariant::Double)) {

            QString property = message.getArg(0).toString();
            float value = (float)message.getArg(1).toDouble();

            if (property == "gain") {
                if (value < 0.0) value = 0.0;
                m_fader->setValue(value);
                if (m_playTarget) m_playTarget->setOutputGain(value);
            } else if (property == "speedup") {
                m_playSpeed->setMappedValue(value);
            } else if (property == "overlays") {
                if (value < 0.5) {
                    m_viewManager->setOverlayMode(ViewManager::NoOverlays);
                } else if (value < 1.5) {
                    m_viewManager->setOverlayMode(ViewManager::MinimalOverlays);
                } else if (value < 2.5) {
                    m_viewManager->setOverlayMode(ViewManager::StandardOverlays);
                } else {
                    m_viewManager->setOverlayMode(ViewManager::AllOverlays);
                }                    
            } else if (property == "zoomwheels") {
                m_viewManager->setZoomWheelsEnabled(value > 0.5);
            } else if (property == "propertyboxes") {
                bool toggle = ((value < 0.5) !=
                               (m_paneStack->getLayoutStyle() == PaneStack::NoPropertyStacks));
                if (toggle) togglePropertyBoxes();
            }
                
        } else {
            PropertyContainer *container = 0;
            Pane *pane = m_paneStack->getCurrentPane();
            if (pane &&
                message.getArgCount() == 3 &&
                message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(1).canConvert(QVariant::String) &&
                message.getArg(2).canConvert(QVariant::String)) {
                if (message.getArg(0).toString() == "pane") {
                    container = pane->getPropertyContainer(0);
                } else if (message.getArg(0).toString() == "layer") {
                    container = pane->getSelectedLayer();
                }
            }
            if (container) {
                QString nameString = message.getArg(1).toString();
                QString valueString = message.getArg(2).toString();
                container->setPropertyWithCommand(nameString, valueString);
            }
        }

    } else if (message.getMethod() == "setcurrent") {

        int paneIndex = -1, layerIndex = -1;
        bool wantLayer = false;

        if (message.getArgCount() >= 1 &&
            message.getArg(0).canConvert(QVariant::Int)) {

            paneIndex = message.getArg(0).toInt() - 1;

            if (message.getArgCount() >= 2 &&
                message.getArg(1).canConvert(QVariant::Int)) {
                wantLayer = true;
                layerIndex = message.getArg(1).toInt() - 1;
            }
        }

        if (paneIndex >= 0 && paneIndex < m_paneStack->getPaneCount()) {
            Pane *pane = m_paneStack->getPane(paneIndex);
            m_paneStack->setCurrentPane(pane);
            if (layerIndex >= 0 && layerIndex < pane->getLayerCount()) {
                Layer *layer = pane->getLayer(layerIndex);
                m_paneStack->setCurrentLayer(pane, layer);
            } else if (wantLayer && layerIndex == -1) {
                m_paneStack->setCurrentLayer(pane, 0);
            }
        }

    } else if (message.getMethod() == "delete") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {
            
            QString target = message.getArg(0).toString();

            if (target == "pane") {

                deleteCurrentPane();

            } else if (target == "layer") {

                deleteCurrentLayer();

            } else {
                
                std::cerr << "WARNING: MainWindow::handleOSCMessage: Unknown delete target " << target.toStdString() << std::endl;
            }
        }

    } else if (message.getMethod() == "zoom") {

        if (message.getArgCount() == 1) {
            if (message.getArg(0).canConvert(QVariant::String) &&
                message.getArg(0).toString() == "in") {
                zoomIn();
            } else if (message.getArg(0).canConvert(QVariant::String) &&
                       message.getArg(0).toString() == "out") {
                zoomOut();
            } else if (message.getArg(0).canConvert(QVariant::String) &&
                       message.getArg(0).toString() == "default") {
                zoomDefault();
            } else if (message.getArg(0).canConvert(QVariant::Double)) {
                double level = message.getArg(0).toDouble();
                Pane *currentPane = m_paneStack->getCurrentPane();
                if (level < 1.0) level = 1.0;
                if (currentPane) currentPane->setZoomLevel(lrint(level));
            }
        }

    } else if (message.getMethod() == "zoomvertical") {

        Pane *pane = m_paneStack->getCurrentPane();
        Layer *layer = 0;
        if (pane && pane->getLayerCount() > 0) {
            layer = pane->getLayer(pane->getLayerCount() - 1);
        }
        int defaultStep = 0;
        int steps = 0;
        if (layer) {
            steps = layer->getVerticalZoomSteps(defaultStep);
            if (message.getArgCount() == 1 && steps > 0) {
                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "in") {
                    int step = layer->getCurrentVerticalZoomStep() + 1;
                    if (step < steps) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "out") {
                    int step = layer->getCurrentVerticalZoomStep() - 1;
                    if (step >= 0) layer->setVerticalZoomStep(step);
                } else if (message.getArg(0).canConvert(QVariant::String) &&
                           message.getArg(0).toString() == "default") {
                    layer->setVerticalZoomStep(defaultStep);
                }
            } else if (message.getArgCount() == 2) {
                if (message.getArg(0).canConvert(QVariant::Double) &&
                    message.getArg(1).canConvert(QVariant::Double)) {
                    double min = message.getArg(0).toDouble();
                    double max = message.getArg(1).toDouble();
                    layer->setDisplayExtents(min, max);
                }
            }
        }

    } else if (message.getMethod() == "quit") {
        
        m_abandoning = true;
        close();

    } else if (message.getMethod() == "resize") {
        
        if (message.getArgCount() == 2) {

            int width = 0, height = 0;

            if (message.getArg(1).canConvert(QVariant::Int)) {

                height = message.getArg(1).toInt();

                if (message.getArg(0).canConvert(QVariant::String) &&
                    message.getArg(0).toString() == "pane") {

                    Pane *pane = m_paneStack->getCurrentPane();
                    if (pane) pane->resize(pane->width(), height);

                } else if (message.getArg(0).canConvert(QVariant::Int)) {

                    width = message.getArg(0).toInt();
                    resize(width, height);
                }
            }
        }

    } else if (message.getMethod() == "transform") {

        Pane *pane = m_paneStack->getCurrentPane();

        if (getMainModel() &&
            pane &&
            message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            TransformId transform = message.getArg(0).toString();

            Layer *newLayer = m_document->createDerivedLayer
                (transform,
                 getMainModel(),
                 TransformFactory::getInstance()->getDefaultContextForTransform
                 (transform, getMainModel()),
                 "");

            if (newLayer) {
                m_document->addLayerToView(pane, newLayer);
                m_recentTransforms.add(transform);
                m_paneStack->setCurrentLayer(pane, newLayer);
            }
        }

    } else {
        std::cerr << "WARNING: MainWindow::handleOSCMessage: Unknown or unsupported "
                  << "method \"" << message.getMethod().toStdString()
                  << "\"" << std::endl;
    }
            
}

void
MainWindow::preferences()
{
    if (!m_preferencesDialog.isNull()) {
        m_preferencesDialog->show();
        m_preferencesDialog->raise();
        return;
    }

    m_preferencesDialog = new PreferencesDialog(this);

    // DeleteOnClose is safe here, because m_preferencesDialog is a
    // QPointer that will be zeroed when the dialog is deleted.  We
    // use it in preference to leaving the dialog lying around because
    // if you Cancel the dialog, it resets the preferences state
    // without resetting its own widgets, so its state will be
    // incorrect when next shown unless we construct it afresh
    m_preferencesDialog->setAttribute(Qt::WA_DeleteOnClose);

    m_preferencesDialog->show();
}

void
MainWindow::mouseEnteredWidget()
{
    QWidget *w = dynamic_cast<QWidget *>(sender());
    if (!w) return;

    if (w == m_fader) {
        contextHelpChanged(tr("Adjust the master playback level"));
    } else if (w == m_playSpeed) {
        contextHelpChanged(tr("Adjust the master playback speed"));
    } else if (w == m_playSharpen && w->isEnabled()) {
        contextHelpChanged(tr("Toggle transient sharpening for playback time scaling"));
    } else if (w == m_playMono && w->isEnabled()) {
        contextHelpChanged(tr("Toggle mono mode for playback time scaling"));
    }
}

void
MainWindow::mouseLeftWidget()
{
    contextHelpChanged("");
}

void
MainWindow::inProgressSelectionChanged()
{
    Pane *currentPane = 0;
    if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
    if (currentPane) updateVisibleRangeDisplay(currentPane);
}

void
MainWindow::contextHelpChanged(const QString &s)
{
    if (s == "" && m_myStatusMessage != "") {
        statusBar()->showMessage(m_myStatusMessage);
        return;
    }
    statusBar()->showMessage(s);
}

void
MainWindow::website()
{
    openHelpUrl(tr("http://www.sonicvisualiser.org/"));
}

void
MainWindow::help()
{
    openHelpUrl(tr("http://www.sonicvisualiser.org/doc/reference/1.0/en/"));
}

void
MainWindow::openHelpUrl(QString url)
{
    // This method mostly lifted from Qt Assistant source code

    QProcess *process = new QProcess(this);
    connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    QStringList args;

#ifdef Q_OS_MAC
    args.append(url);
    process->start("open", args);
#else
#ifdef Q_OS_WIN32

	QString pf(getenv("ProgramFiles"));
	QString command = pf + QString("\\Internet Explorer\\IEXPLORE.EXE");

	args.append(url);
	process->start(command, args);

#else
#ifdef Q_WS_X11
    if (!qgetenv("KDE_FULL_SESSION").isEmpty()) {
        args.append("exec");
        args.append(url);
        process->start("kfmclient", args);
    } else if (!qgetenv("BROWSER").isEmpty()) {
        args.append(url);
        process->start(qgetenv("BROWSER"), args);
    } else {
        args.append(url);
        process->start("firefox", args);
    }
#endif
#endif
#endif
}

void
MainWindow::about()
{
    bool debug = false;
    QString version = "(unknown version)";

#ifdef BUILD_DEBUG
    debug = true;
#endif
#ifdef SV_VERSION
#ifdef SVNREV
    version = tr("Release %1 : Revision %2").arg(SV_VERSION).arg(SVNREV);
#else
    version = tr("Release %1").arg(SV_VERSION);
#endif
#else
#ifdef SVNREV
    version = tr("Unreleased : Revision %1").arg(SVNREV);
#endif
#endif

    QString aboutText;

    aboutText += tr("<h3>About Sonic Visualiser</h3>");
    aboutText += tr("<p>Sonic Visualiser is a program for viewing and exploring audio data for<br>semantic music analysis and annotation.</p>");
    aboutText += tr("<p>%1 : %2 configuration</p>")
        .arg(version)
        .arg(debug ? tr("Debug") : tr("Release"));

#ifndef BUILD_STATIC
    aboutText += tr("<br>Using Qt v%1 &copy; Trolltech AS").arg(QT_VERSION_STR);
#else
#ifdef QT_SHARED
    aboutText += tr("<br>Using Qt v%1 &copy; Trolltech AS").arg(QT_VERSION_STR);
#endif
#endif

#ifdef BUILD_STATIC
    aboutText += tr("<p>Statically linked");
#ifndef QT_SHARED
    aboutText += tr("<br>With Qt (v%1) &copy; Trolltech AS").arg(QT_VERSION_STR);
#endif
#ifdef HAVE_JACK
#ifdef JACK_VERSION
    aboutText += tr("<br>With JACK audio output (v%1) &copy; Paul Davis and Jack O'Quin").arg(JACK_VERSION);
#else
    aboutText += tr("<br>With JACK audio output &copy; Paul Davis and Jack O'Quin");
#endif
#endif
#ifdef HAVE_PORTAUDIO
    aboutText += tr("<br>With PortAudio audio output &copy; Ross Bencina and Phil Burk");
#endif
#ifdef HAVE_OGGZ
#ifdef OGGZ_VERSION
    aboutText += tr("<br>With Ogg file decoder (oggz v%1, fishsound v%2) &copy; CSIRO Australia").arg(OGGZ_VERSION).arg(FISHSOUND_VERSION);
#else
    aboutText += tr("<br>With Ogg file decoder &copy; CSIRO Australia");
#endif
#endif
#ifdef HAVE_MAD
#ifdef MAD_VERSION
    aboutText += tr("<br>With MAD mp3 decoder (v%1) &copy; Underbit Technologies Inc").arg(MAD_VERSION);
#else
    aboutText += tr("<br>With MAD mp3 decoder &copy; Underbit Technologies Inc");
#endif
#endif
#ifdef HAVE_SAMPLERATE
#ifdef SAMPLERATE_VERSION
    aboutText += tr("<br>With libsamplerate (v%1) &copy; Erik de Castro Lopo").arg(SAMPLERATE_VERSION);
#else
    aboutText += tr("<br>With libsamplerate &copy; Erik de Castro Lopo");
#endif
#endif
#ifdef HAVE_SNDFILE
#ifdef SNDFILE_VERSION
    aboutText += tr("<br>With libsndfile (v%1) &copy; Erik de Castro Lopo").arg(SNDFILE_VERSION);
#else
    aboutText += tr("<br>With libsndfile &copy; Erik de Castro Lopo");
#endif
#endif
#ifdef HAVE_FFTW3F
#ifdef FFTW3_VERSION
    aboutText += tr("<br>With FFTW3 (v%1) &copy; Matteo Frigo and MIT").arg(FFTW3_VERSION);
#else
    aboutText += tr("<br>With FFTW3 &copy; Matteo Frigo and MIT");
#endif
#endif
#ifdef HAVE_VAMP
    aboutText += tr("<br>With Vamp plugin support (API v%1, host SDK v%2) &copy; Chris Cannam").arg(VAMP_API_VERSION).arg(VAMP_SDK_VERSION);
#endif
    aboutText += tr("<br>With LADSPA plugin support (API v%1) &copy; Richard Furse, Paul Davis, Stefan Westerfeld").arg(LADSPA_VERSION);
    aboutText += tr("<br>With DSSI plugin support (API v%1) &copy; Chris Cannam, Steve Harris, Sean Bolton").arg(DSSI_VERSION);
#ifdef HAVE_LIBLO
#ifdef LIBLO_VERSION
    aboutText += tr("<br>With liblo Lite OSC library (v%1) &copy; Steve Harris").arg(LIBLO_VERSION);
#else
    aboutText += tr("<br>With liblo Lite OSC library &copy; Steve Harris").arg(LIBLO_VERSION);
#endif
    if (m_oscQueue && m_oscQueue->isOK()) {
        aboutText += tr("<p>The OSC URL for this instance is: \"%1\"").arg(m_oscQueue->getOSCURL());
    }
#endif
    aboutText += "</p>";
#endif

    aboutText += 
        "<p>Sonic Visualiser Copyright &copy; 2005 - 2007 Chris Cannam and<br>"
        "Queen Mary, University of London.</p>"
        "<p>This program is free software; you can redistribute it and/or<br>"
        "modify it under the terms of the GNU General Public License as<br>"
        "published by the Free Software Foundation; either version 2 of the<br>"
        "License, or (at your option) any later version.<br>See the file "
        "COPYING included with this distribution for more information.</p>";
    
    QMessageBox::about(this, tr("About Sonic Visualiser"), aboutText);
}

