/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006-2007 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "../version.h"

#include "MainWindow.h"
#include "framework/Document.h"
#include "PreferencesDialog.h"

#include "view/Pane.h"
#include "view/PaneStack.h"
#include "data/model/WaveFileModel.h"
#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/NoteModel.h"
#include "data/model/Labeller.h"
#include "view/ViewManager.h"
#include "base/Preferences.h"
#include "layer/WaveformLayer.h"
#include "layer/TimeRulerLayer.h"
#include "layer/TimeInstantLayer.h"
#include "layer/TimeValueLayer.h"
#include "layer/Colour3DPlotLayer.h"
#include "layer/SliceLayer.h"
#include "layer/SliceableLayer.h"
#include "layer/ImageLayer.h"
#include "widgets/Fader.h"
#include "view/Overview.h"
#include "widgets/PropertyBox.h"
#include "widgets/PropertyStack.h"
#include "widgets/AudioDial.h"
#include "widgets/IconLoader.h"
#include "widgets/LayerTree.h"
#include "widgets/ListInputDialog.h"
#include "widgets/SubdividingMenu.h"
#include "widgets/NotifyingPushButton.h"
#include "widgets/KeyReference.h"
#include "widgets/LabelCounterInputDialog.h"
#include "audioio/AudioCallbackPlaySource.h"
#include "audioio/AudioCallbackPlayTarget.h"
#include "audioio/AudioTargetFactory.h"
#include "audioio/PlaySpeedRangeMapper.h"
#include "data/fileio/DataFileReaderFactory.h"
#include "data/fileio/PlaylistFileReader.h"
#include "data/fileio/WavFileWriter.h"
#include "data/fileio/CSVFileWriter.h"
#include "data/fileio/MIDIFileWriter.h"
#include "data/fileio/BZipFileDevice.h"
#include "data/fileio/FileSource.h"
#include "data/fft/FFTDataServer.h"
#include "base/RecentFiles.h"
#include "plugin/transform/TransformerFactory.h"
#include "base/PlayParameterRepository.h"
#include "base/XmlExportable.h"
#include "base/CommandHistory.h"
#include "base/Profiler.h"
#include "base/Clipboard.h"
#include "base/UnitDatabase.h"
#include "base/ColourDatabase.h"
#include "data/osc/OSCQueue.h"

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
#include <QScrollArea>

#include <iostream>
#include <cstdio>
#include <errno.h>

using std::cerr;
using std::endl;

using std::vector;
using std::map;
using std::set;


MainWindow::MainWindow(bool withAudioOutput, bool withOSCSupport) :
    MainWindowBase(withAudioOutput, withOSCSupport),
    m_overview(0),
    m_mainMenusCreated(false),
    m_paneMenu(0),
    m_layerMenu(0),
    m_transformsMenu(0),
    m_playbackMenu(0),
    m_existingLayersMenu(0),
    m_sliceMenu(0),
    m_recentFilesMenu(0),
    m_recentTransformersMenu(0),
    m_rightButtonMenu(0),
    m_rightButtonLayerMenu(0),
    m_rightButtonTransformersMenu(0),
    m_rightButtonPlaybackMenu(0),
    m_soloAction(0),
    m_soloModified(false),
    m_prevSolo(false),
    m_ffwdAction(0),
    m_rwdAction(0),
    m_preferencesDialog(0),
    m_layerTreeView(0),
    m_keyReference(new KeyReference())
{
    setWindowTitle(tr("Sonic Visualiser"));

    UnitDatabase *udb = UnitDatabase::getInstance();
    udb->registerUnit("Hz");
    udb->registerUnit("dB");
    udb->registerUnit("s");

    ColourDatabase *cdb = ColourDatabase::getInstance();
    cdb->addColour(Qt::black, tr("Black"));
    cdb->addColour(Qt::darkRed, tr("Red"));
    cdb->addColour(Qt::darkBlue, tr("Blue"));
    cdb->addColour(Qt::darkGreen, tr("Green"));
    cdb->addColour(QColor(200, 50, 255), tr("Purple"));
    cdb->addColour(QColor(255, 150, 50), tr("Orange"));
    cdb->setUseDarkBackground(cdb->addColour(Qt::white, tr("White")), true);
    cdb->setUseDarkBackground(cdb->addColour(Qt::red, tr("Bright Red")), true);
    cdb->setUseDarkBackground(cdb->addColour(QColor(30, 150, 255), tr("Bright Blue")), true);
    cdb->setUseDarkBackground(cdb->addColour(Qt::green, tr("Bright Green")), true);
    cdb->setUseDarkBackground(cdb->addColour(QColor(225, 74, 255), tr("Bright Purple")), true);
    cdb->setUseDarkBackground(cdb->addColour(QColor(255, 188, 80), tr("Bright Orange")), true);

    QFrame *frame = new QFrame;
    setCentralWidget(frame);

    QGridLayout *layout = new QGridLayout;

    m_descriptionLabel = new QLabel; //!!! hang on, this is declared in base class -- should be declared and initialised by same class

    QScrollArea *scroll = new QScrollArea(frame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    scroll->setWidget(m_paneStack);

    m_overview = new Overview(frame);
    m_overview->setViewManager(m_viewManager);
    m_overview->setFixedHeight(40);
#ifndef _WIN32
    // For some reason, the contents of the overview never appear if we
    // make this setting on Windows.  I have no inclination at the moment
    // to track down the reason why.
    m_overview->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
#endif
    connect(m_overview, SIGNAL(contextHelpChanged(const QString &)),
            this, SLOT(contextHelpChanged(const QString &)));

    m_panLayer = new WaveformLayer;
    m_panLayer->setChannelMode(WaveformLayer::MergeChannels);
    m_panLayer->setAggressiveCacheing(true);
    m_overview->addLayer(m_panLayer);

    if (m_viewManager->getGlobalDarkBackground()) {
        m_panLayer->setBaseColour
            (ColourDatabase::getInstance()->getColourIndex(tr("Bright Green")));
    } else {
        m_panLayer->setBaseColour
            (ColourDatabase::getInstance()->getColourIndex(tr("Green")));
    }

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

    IconLoader il;

    m_playSharpen = new NotifyingPushButton(frame);
    m_playSharpen->setToolTip(tr("Sharpen percussive transients"));
    m_playSharpen->setFixedSize(20, 20);
    m_playSharpen->setEnabled(false);
    m_playSharpen->setCheckable(true);
    m_playSharpen->setChecked(false);
    m_playSharpen->setIcon(il.load("sharpen"));
    connect(m_playSharpen, SIGNAL(clicked()), this, SLOT(playSharpenToggled()));
    connect(m_playSharpen, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_playSharpen, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    m_playMono = new NotifyingPushButton(frame);
    m_playMono->setToolTip(tr("Run time stretcher in mono only"));
    m_playMono->setFixedSize(20, 20);
    m_playMono->setEnabled(false);
    m_playMono->setCheckable(true);
    m_playMono->setChecked(false);
    m_playMono->setIcon(il.load("mono"));
    connect(m_playMono, SIGNAL(clicked()), this, SLOT(playMonoToggled()));
    connect(m_playMono, SIGNAL(mouseEntered()), this, SLOT(mouseEnteredWidget()));
    connect(m_playMono, SIGNAL(mouseLeft()), this, SLOT(mouseLeftWidget()));

    QSettings settings;
    settings.beginGroup("MainWindow");
    m_playSharpen->setChecked(settings.value("playsharpen", true).toBool());
    m_playMono->setChecked(settings.value("playmono", false).toBool());
    settings.endGroup();

    layout->setSpacing(4);
    layout->addWidget(scroll, 0, 0, 1, 5);
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

    setupMenus();
    setupToolbars();
    setupHelpMenu();

    statusBar();

    newSession();
}

MainWindow::~MainWindow()
{
    delete m_keyReference;
    delete m_preferencesDialog;
    delete m_layerTreeView;
    Profiles::getInstance()->dump();
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

    if (m_rightButtonTransformersMenu) {
        m_rightButtonTransformersMenu->clear();
    } else {
        m_rightButtonTransformersMenu = m_rightButtonMenu->addMenu(tr("&Transformer"));
        m_rightButtonTransformersMenu->setTearOffEnabled(true);
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
    setupTransformersMenu();

    m_mainMenusCreated = true;
}

void
MainWindow::setupFileMenu()
{
    if (m_mainMenusCreated) return;

    QMenu *menu = menuBar()->addMenu(tr("&File"));
    menu->setTearOffEnabled(true);
    QToolBar *toolbar = addToolBar(tr("File Toolbar"));

    m_keyReference->setCategory(tr("File and Session Management"));

    IconLoader il;

    QIcon icon = il.load("filenew");
    icon.addPixmap(il.loadPixmap("filenew-22"));
    QAction *action = new QAction(icon, tr("&New Session"), this);
    action->setShortcut(tr("Ctrl+N"));
    action->setStatusTip(tr("Abandon the current Sonic Visualiser session and start a new one"));
    connect(action, SIGNAL(triggered()), this, SLOT(newSession()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    toolbar->addAction(action);

    icon = il.load("fileopensession");
    action = new QAction(icon, tr("&Open Session..."), this);
    action->setShortcut(tr("Ctrl+O"));
    action->setStatusTip(tr("Open a previously saved Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(openSession()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    icon = il.load("fileopen");
    icon.addPixmap(il.loadPixmap("fileopen-22"));

    action = new QAction(icon, tr("&Open..."), this);
    action->setStatusTip(tr("Open a session file, audio file, or layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(openSomething()));
    toolbar->addAction(action);

    icon = il.load("filesave");
    icon.addPixmap(il.loadPixmap("filesave-22"));
    action = new QAction(icon, tr("&Save Session"), this);
    action->setShortcut(tr("Ctrl+S"));
    action->setStatusTip(tr("Save the current session into a Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(saveSession()));
    connect(this, SIGNAL(canSave(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    toolbar->addAction(action);
	
    icon = il.load("filesaveas");
    icon.addPixmap(il.loadPixmap("filesaveas-22"));
    action = new QAction(icon, tr("Save Session &As..."), this);
    action->setStatusTip(tr("Save the current session into a new Sonic Visualiser session file"));
    connect(action, SIGNAL(triggered()), this, SLOT(saveSessionAs()));
    menu->addAction(action);
    toolbar->addAction(action);

    menu->addSeparator();

    icon = il.load("fileopenaudio");
    action = new QAction(icon, tr("&Import Audio File..."), this);
    action->setShortcut(tr("Ctrl+I"));
    action->setStatusTip(tr("Import an existing audio file"));
    connect(action, SIGNAL(triggered()), this, SLOT(importAudio()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    action = new QAction(tr("Import Secondary Audio File..."), this);
    action->setShortcut(tr("Ctrl+Shift+I"));
    action->setStatusTip(tr("Import an extra audio file as a separate layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(importMoreAudio()));
    connect(this, SIGNAL(canImportMoreAudio(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
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
    m_keyReference->registerShortcut(action);
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
    m_keyReference->registerShortcut(action);
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
	
    menu->addSeparator();
    action = new QAction(il.load("exit"),
                         tr("&Quit"), this);
    action->setShortcut(tr("Ctrl+Q"));
    action->setStatusTip(tr("Exit Sonic Visualiser"));
    connect(action, SIGNAL(triggered()), this, SLOT(close()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
}

void
MainWindow::setupEditMenu()
{
    if (m_mainMenusCreated) return;

    QMenu *menu = menuBar()->addMenu(tr("&Edit"));
    menu->setTearOffEnabled(true);
    CommandHistory::getInstance()->registerMenu(menu);

    m_keyReference->setCategory(tr("Editing"));

    menu->addSeparator();

    IconLoader il;

    QAction *action = new QAction(il.load("editcut"),
                                  tr("Cu&t"), this);
    action->setShortcut(tr("Ctrl+X"));
    action->setStatusTip(tr("Cut the selection from the current layer to the clipboard"));
    connect(action, SIGNAL(triggered()), this, SLOT(cut()));
    connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    action = new QAction(il.load("editcopy"),
                         tr("&Copy"), this);
    action->setShortcut(tr("Ctrl+C"));
    action->setStatusTip(tr("Copy the selection from the current layer to the clipboard"));
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));
    connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    action = new QAction(il.load("editpaste"),
                         tr("&Paste"), this);
    action->setShortcut(tr("Ctrl+V"));
    action->setStatusTip(tr("Paste from the clipboard to the current layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(paste()));
    connect(this, SIGNAL(canPaste(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    m_deleteSelectedAction = new QAction(tr("&Delete Selected Items"), this);
    m_deleteSelectedAction->setShortcut(tr("Del"));
    m_deleteSelectedAction->setStatusTip(tr("Delete items in current selection from the current layer"));
    connect(m_deleteSelectedAction, SIGNAL(triggered()), this, SLOT(deleteSelected()));
    connect(this, SIGNAL(canDeleteSelection(bool)), m_deleteSelectedAction, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(m_deleteSelectedAction);
    menu->addAction(m_deleteSelectedAction);
    m_rightButtonMenu->addAction(m_deleteSelectedAction);

    menu->addSeparator();
    m_rightButtonMenu->addSeparator();

    m_keyReference->setCategory(tr("Selection"));

    action = new QAction(tr("Select &All"), this);
    action->setShortcut(tr("Ctrl+A"));
    action->setStatusTip(tr("Select the whole duration of the current session"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);
	
    action = new QAction(tr("Select &Visible Range"), this);
    action->setShortcut(tr("Ctrl+Shift+A"));
    action->setStatusTip(tr("Select the time range corresponding to the current window width"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectVisible()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("Select to &Start"), this);
    action->setShortcut(tr("Shift+Left"));
    action->setStatusTip(tr("Select from the start of the session to the current playback position"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectToStart()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("Select to &End"), this);
    action->setShortcut(tr("Shift+Right"));
    action->setStatusTip(tr("Select from the current playback position to the end of the session"));
    connect(action, SIGNAL(triggered()), this, SLOT(selectToEnd()));
    connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    action = new QAction(tr("C&lear Selection"), this);
    action->setShortcut(tr("Esc"));
    action->setStatusTip(tr("Clear the selection"));
    connect(action, SIGNAL(triggered()), this, SLOT(clearSelection()));
    connect(this, SIGNAL(canClearSelection(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonMenu->addAction(action);

    menu->addSeparator();

    m_keyReference->setCategory(tr("Tapping Time Instants"));

    action = new QAction(tr("&Insert Instant at Playback Position"), this);
    action->setShortcut(tr("Enter"));
    action->setStatusTip(tr("Insert a new time instant at the current playback position, in a new layer if necessary"));
    connect(action, SIGNAL(triggered()), this, SLOT(insertInstant()));
    connect(this, SIGNAL(canInsertInstant(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    // Laptop shortcut (no keypad Enter key)
    QString shortcut(tr(";"));
    connect(new QShortcut(shortcut, this), SIGNAL(activated()),
            this, SLOT(insertInstant()));
    m_keyReference->registerAlternativeShortcut(action, shortcut);

    action = new QAction(tr("Insert Instants at Selection &Boundaries"), this);
    action->setShortcut(tr("Shift+Enter"));
    action->setStatusTip(tr("Insert new time instants at the start and end of the current selected regions, in a new layer if necessary"));
    connect(action, SIGNAL(triggered()), this, SLOT(insertInstantsAtBoundaries()));
    connect(this, SIGNAL(canInsertInstantsAtBoundaries(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    QMenu *numberingMenu = menu->addMenu(tr("Number New Instants with"));
    QActionGroup *numberingGroup = new QActionGroup(this);

    Labeller::TypeNameMap types = m_labeller->getTypeNames();
    for (Labeller::TypeNameMap::iterator i = types.begin(); i != types.end(); ++i) {

        if (i->first == Labeller::ValueFromLabel ||
            i->first == Labeller::ValueFromExistingNeighbour) continue;

        action = new QAction(i->second, this);
        connect(action, SIGNAL(triggered()), this, SLOT(setInstantsNumbering()));
        action->setCheckable(true);
        action->setChecked(m_labeller->getType() == i->first);
        numberingGroup->addAction(action);
        numberingMenu->addAction(action);
        m_numberingActions[action] = (int)i->first;

        if (i->first == Labeller::ValueFromTwoLevelCounter) {

            QMenu *cycleMenu = numberingMenu->addMenu(tr("Cycle size"));
            QActionGroup *cycleGroup = new QActionGroup(this);

            int cycles[] = { 2, 3, 4, 5, 6, 7, 8, 10, 12, 16 };
            for (int i = 0; i < int(sizeof(cycles)/sizeof(cycles[0])); ++i) {
                action = new QAction(QString("%1").arg(cycles[i]), this);
                connect(action, SIGNAL(triggered()), this, SLOT(setInstantsCounterCycle()));
                action->setCheckable(true);
                action->setChecked(cycles[i] == m_labeller->getCounterCycleSize());
                cycleGroup->addAction(action);
                cycleMenu->addAction(action);
            }
            
            action = new QAction(tr("Reset Counters..."), this);
            connect(action, SIGNAL(triggered()), this, SLOT(resetInstantsCounters()));
            numberingMenu->addAction(action);
        }

        if (i->first == Labeller::ValueNone ||
            i->first == Labeller::ValueFromTwoLevelCounter ||
            i->first == Labeller::ValueFromRealTime) {
            numberingMenu->addSeparator();
        }
    }

    action = new QAction(tr("Re-Number Selected Instants"), this);
    action->setStatusTip(tr("Re-number the selected instants using the current labelling scheme"));
    connect(action, SIGNAL(triggered()), this, SLOT(renumberInstants()));
    connect(this, SIGNAL(canRenumberInstants(bool)), action, SLOT(setEnabled(bool)));
//    m_keyReference->registerShortcut(action);
    menu->addAction(action);
}

void
MainWindow::setupViewMenu()
{
    if (m_mainMenusCreated) return;

    IconLoader il;

    QAction *action = 0;

    m_keyReference->setCategory(tr("Panning and Navigation"));

    QMenu *menu = menuBar()->addMenu(tr("&View"));
    menu->setTearOffEnabled(true);
    action = new QAction(tr("Scroll &Left"), this);
    action->setShortcut(tr("Left"));
    action->setStatusTip(tr("Scroll the current pane to the left"));
    connect(action, SIGNAL(triggered()), this, SLOT(scrollLeft()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("Scroll &Right"), this);
    action->setShortcut(tr("Right"));
    action->setStatusTip(tr("Scroll the current pane to the right"));
    connect(action, SIGNAL(triggered()), this, SLOT(scrollRight()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("&Jump Left"), this);
    action->setShortcut(tr("Ctrl+Left"));
    action->setStatusTip(tr("Scroll the current pane a big step to the left"));
    connect(action, SIGNAL(triggered()), this, SLOT(jumpLeft()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("J&ump Right"), this);
    action->setShortcut(tr("Ctrl+Right"));
    action->setStatusTip(tr("Scroll the current pane a big step to the right"));
    connect(action, SIGNAL(triggered()), this, SLOT(jumpRight()));
    connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    menu->addSeparator();

    m_keyReference->setCategory(tr("Zoom"));

    action = new QAction(il.load("zoom-in"),
                         tr("Zoom &In"), this);
    action->setShortcut(tr("Up"));
    action->setStatusTip(tr("Increase the zoom level"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomIn()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(il.load("zoom-out"),
                         tr("Zoom &Out"), this);
    action->setShortcut(tr("Down"));
    action->setStatusTip(tr("Decrease the zoom level"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomOut()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
	
    action = new QAction(tr("Restore &Default Zoom"), this);
    action->setStatusTip(tr("Restore the zoom level to the default"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomDefault()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    menu->addAction(action);

    action = new QAction(il.load("zoom-fit"),
                         tr("Zoom to &Fit"), this);
    action->setShortcut(tr("F"));
    action->setStatusTip(tr("Zoom to show the whole file"));
    connect(action, SIGNAL(triggered()), this, SLOT(zoomToFit()));
    connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    menu->addSeparator();

    m_keyReference->setCategory(tr("Display Features"));

    QActionGroup *overlayGroup = new QActionGroup(this);
        
    action = new QAction(tr("Show &No Overlays"), this);
    action->setShortcut(tr("0"));
    action->setStatusTip(tr("Hide centre indicator, frame times, layer names and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showNoOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &Minimal Overlays"), this);
    action->setShortcut(tr("9"));
    action->setStatusTip(tr("Show centre indicator only"));
    connect(action, SIGNAL(triggered()), this, SLOT(showMinimalOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &Standard Overlays"), this);
    action->setShortcut(tr("8"));
    action->setStatusTip(tr("Show centre indicator, frame times and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showStandardOverlays()));
    action->setCheckable(true);
    action->setChecked(true);
    overlayGroup->addAction(action);
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show &All Overlays"), this);
    action->setShortcut(tr("7"));
    action->setStatusTip(tr("Show all texts and scale"));
    connect(action, SIGNAL(triggered()), this, SLOT(showAllOverlays()));
    action->setCheckable(true);
    action->setChecked(false);
    overlayGroup->addAction(action);
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
        
    menu->addSeparator();

    action = new QAction(tr("Show &Zoom Wheels"), this);
    action->setShortcut(tr("Z"));
    action->setStatusTip(tr("Show thumbwheels for zooming horizontally and vertically"));
    connect(action, SIGNAL(triggered()), this, SLOT(toggleZoomWheels()));
    action->setCheckable(true);
    action->setChecked(m_viewManager->getZoomWheelsEnabled());
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
        
    action = new QAction(tr("Show Property Bo&xes"), this);
    action->setShortcut(tr("X"));
    action->setStatusTip(tr("Show the layer property boxes at the side of the main window"));
    connect(action, SIGNAL(triggered()), this, SLOT(togglePropertyBoxes()));
    action->setCheckable(true);
    action->setChecked(true);
    m_keyReference->registerShortcut(action);
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

    menu->addSeparator();

    action = new QAction(tr("Show La&yer Hierarchy"), this);
    action->setShortcut(tr("H"));
    action->setStatusTip(tr("Open a window displaying the hierarchy of panes and layers in this session"));
    connect(action, SIGNAL(triggered()), this, SLOT(showLayerTree()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
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

    IconLoader il;

    m_keyReference->setCategory(tr("Managing Panes and Layers"));

    QAction *action = new QAction(il.load("pane"), tr("Add &New Pane"), this);
    action->setShortcut(tr("N"));
    action->setStatusTip(tr("Add a new pane containing only a time ruler"));
    connect(action, SIGNAL(triggered()), this, SLOT(addPane()));
    connect(this, SIGNAL(canAddPane(bool)), action, SLOT(setEnabled(bool)));
    m_paneActions[action] = PaneConfiguration(LayerFactory::TimeRuler);
    m_keyReference->registerShortcut(action);
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
	
	icon = il.load(LayerFactory::getInstance()->getLayerIconName(type));

	mainText = tr("Add New %1 Layer").arg(name);
	tipText = tr("Add a new empty layer of type %1").arg(name);

	action = new QAction(icon, mainText, this);
	action->setStatusTip(tipText);

	if (type == LayerFactory::Text) {
	    action->setShortcut(tr("T"));
            m_keyReference->registerShortcut(action);
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
    if (m_document) models = m_document->getTransformerInputModels(); //!!! not well named for this!
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
                icon = il.load("waveform");
                mainText = tr("Add &Waveform");
                if (menuType == 0) {
                    shortcutText = tr("W");
                    tipText = tr("Add a new pane showing a waveform view");
                } else {
                    tipText = tr("Add a new layer showing a waveform view");
                }
                mono = false;
                break;
		
            case LayerFactory::Spectrogram:
                icon = il.load("spectrogram");
                mainText = tr("Add Spectro&gram");
                if (menuType == 0) {
                    shortcutText = tr("G");
                    tipText = tr("Add a new pane showing a spectrogram");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram");
                }
                break;
		
            case LayerFactory::MelodicRangeSpectrogram:
                icon = il.load("spectrogram");
                mainText = tr("Add &Melodic Range Spectrogram");
                if (menuType == 0) {
                    shortcutText = tr("M");
                    tipText = tr("Add a new pane showing a spectrogram set up for an overview of note pitches");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram set up for an overview of note pitches");
                }
                break;
		
            case LayerFactory::PeakFrequencySpectrogram:
                icon = il.load("spectrogram");
                mainText = tr("Add Pea&k Frequency Spectrogram");
                if (menuType == 0) {
                    shortcutText = tr("K");
                    tipText = tr("Add a new pane showing a spectrogram set up for tracking frequencies");
                } else {
                    tipText = tr("Add a new layer showing a spectrogram set up for tracking frequencies");
                }
                break;
                
            case LayerFactory::Spectrum:
                icon = il.load("spectrum");
                mainText = tr("Add Spectr&um");
                if (menuType == 0) {
                    shortcutText = tr("U");
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
                        if (shortcutText != "") {
                            m_keyReference->registerShortcut(action);
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

    action = new QAction(il.load("editdelete"), tr("&Delete Pane"), this);
    action->setShortcut(tr("Ctrl+Shift+D"));
    action->setStatusTip(tr("Delete the currently active pane"));
    connect(action, SIGNAL(triggered()), this, SLOT(deleteCurrentPane()));
    connect(this, SIGNAL(canDeleteCurrentPane(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    menu = m_layerMenu;

    action = new QAction(il.load("timeruler"), tr("Add &Time Ruler"), this);
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

    QAction *raction = new QAction(tr("&Rename Layer..."), this);
    raction->setShortcut(tr("R"));
    raction->setStatusTip(tr("Rename the currently active layer"));
    connect(raction, SIGNAL(triggered()), this, SLOT(renameCurrentLayer()));
    connect(this, SIGNAL(canRenameLayer(bool)), raction, SLOT(setEnabled(bool)));
    menu->addAction(raction);
    m_rightButtonLayerMenu->addAction(raction);

    action = new QAction(il.load("editdelete"), tr("&Delete Layer"), this);
    action->setShortcut(tr("Ctrl+D"));
    action->setStatusTip(tr("Delete the currently active layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(deleteCurrentLayer()));
    connect(this, SIGNAL(canDeleteCurrentLayer(bool)), action, SLOT(setEnabled(bool)));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);
    m_rightButtonLayerMenu->addAction(action);

    m_keyReference->registerShortcut(raction); // rename after delete, so delete layer goes next to delete pane
}

void
MainWindow::setupTransformersMenu()
{
    if (m_transformsMenu) {
        m_transformActions.clear();
        m_transformActionsReverse.clear();
        m_transformsMenu->clear();
    } else {
	m_transformsMenu = menuBar()->addMenu(tr("&Transform")); 
        m_transformsMenu->setTearOffEnabled(true);
   }

    TransformerFactory::TransformerList transforms =
	TransformerFactory::getInstance()->getAllTransformers();

    vector<QString> types =
        TransformerFactory::getInstance()->getAllTransformerTypes();

    map<QString, map<QString, SubdividingMenu *> > categoryMenus;
    map<QString, map<QString, SubdividingMenu *> > makerMenus;

    map<QString, SubdividingMenu *> byPluginNameMenus;
    map<QString, map<QString, QMenu *> > pluginNameMenus;

    set<SubdividingMenu *> pendingMenus;

    m_recentTransformersMenu = m_transformsMenu->addMenu(tr("&Recent Transforms"));
    m_recentTransformersMenu->setTearOffEnabled(true);
    m_rightButtonTransformersMenu->addMenu(m_recentTransformersMenu);
    connect(&m_recentTransformers, SIGNAL(recentChanged()),
            this, SLOT(setupRecentTransformersMenu()));

    m_transformsMenu->addSeparator();
    m_rightButtonTransformersMenu->addSeparator();
    
    for (vector<QString>::iterator i = types.begin(); i != types.end(); ++i) {

        if (i != types.begin()) {
            m_transformsMenu->addSeparator();
            m_rightButtonTransformersMenu->addSeparator();
        }

        QString byCategoryLabel = tr("%1 by Category").arg(*i);
        SubdividingMenu *byCategoryMenu = new SubdividingMenu(byCategoryLabel,
                                                              20, 40);
        byCategoryMenu->setTearOffEnabled(true);
        m_transformsMenu->addMenu(byCategoryMenu);
        m_rightButtonTransformersMenu->addMenu(byCategoryMenu);
        pendingMenus.insert(byCategoryMenu);

        vector<QString> categories = 
            TransformerFactory::getInstance()->getTransformerCategories(*i);

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
        m_rightButtonTransformersMenu->addMenu(byPluginNameMenus[*i]);
        pendingMenus.insert(byPluginNameMenus[*i]);

        QString byMakerLabel = tr("%1 by Maker").arg(*i);
        SubdividingMenu *byMakerMenu = new SubdividingMenu(byMakerLabel, 20, 40);
        byMakerMenu->setTearOffEnabled(true);
        m_transformsMenu->addMenu(byMakerMenu);
        m_rightButtonTransformersMenu->addMenu(byMakerMenu);
        pendingMenus.insert(byMakerMenu);

        vector<QString> makers = 
            TransformerFactory::getInstance()->getTransformerMakers(*i);

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

//        cerr << "Transformer: \"" << name.toStdString() << "\": plugin name \"" << pluginName.toStdString() << "\"" << endl;

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

    setupRecentTransformersMenu();
}

void
MainWindow::setupHelpMenu()
{
    QMenu *menu = menuBar()->addMenu(tr("&Help"));
    menu->setTearOffEnabled(true);
    
    m_keyReference->setCategory(tr("Help"));

    IconLoader il;

    QAction *action = new QAction(il.load("help"),
                                  tr("&Help Reference"), this); 
    action->setShortcut(tr("F1"));
    action->setStatusTip(tr("Open the Sonic Visualiser reference manual")); 
    connect(action, SIGNAL(triggered()), this, SLOT(help()));
    m_keyReference->registerShortcut(action);
    menu->addAction(action);

    action = new QAction(tr("&Key and Mouse Reference"), this);
    action->setShortcut(tr("F2"));
    action->setStatusTip(tr("Open a window showing the keystrokes you can use in Sonic Visualiser"));
    connect(action, SIGNAL(triggered()), this, SLOT(keyReference()));
    m_keyReference->registerShortcut(action);
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
        if (i == 0) {
            action->setShortcut(tr("Ctrl+R"));
            m_keyReference->registerShortcut
                (tr("Re-open"),
                 action->shortcut(),
                 tr("Re-open the current or most recently opened file"));
        }
	m_recentFilesMenu->addAction(action);
    }
}

void
MainWindow::setupRecentTransformersMenu()
{
    m_recentTransformersMenu->clear();
    vector<QString> transforms = m_recentTransformers.getRecent();
    for (size_t i = 0; i < transforms.size(); ++i) {
        TransformerActionReverseMap::iterator ti =
            m_transformActionsReverse.find(transforms[i]);
        if (ti == m_transformActionsReverse.end()) {
            std::cerr << "WARNING: MainWindow::setupRecentTransformersMenu: "
                      << "Unknown transform \"" << transforms[i].toStdString()
                      << "\" in recent transforms list" << std::endl;
            continue;
        }
        if (i == 0) {
            ti->second->setShortcut(tr("Ctrl+T"));
            m_keyReference->registerShortcut
                (tr("Repeat Transformer"),
                 ti->second->shortcut(),
                 tr("Re-select the most recently run transform"));
        }
	m_recentTransformersMenu->addAction(ti->second);
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

    IconLoader il;

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

	QIcon icon = il.load(factory->getLayerIconName
                             (factory->getLayerType(layer)));

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
    m_keyReference->setCategory(tr("Playback and Transport Controls"));

    IconLoader il;

    QMenu *menu = m_playbackMenu = menuBar()->addMenu(tr("Play&back"));
    menu->setTearOffEnabled(true);
    m_rightButtonMenu->addSeparator();
    m_rightButtonPlaybackMenu = m_rightButtonMenu->addMenu(tr("Playback"));

    QToolBar *toolbar = addToolBar(tr("Playback Toolbar"));

    QAction *rwdStartAction = toolbar->addAction(il.load("rewind-start"),
                                                 tr("Rewind to Start"));
    rwdStartAction->setShortcut(tr("Home"));
    rwdStartAction->setStatusTip(tr("Rewind to the start"));
    connect(rwdStartAction, SIGNAL(triggered()), this, SLOT(rewindStart()));
    connect(this, SIGNAL(canPlay(bool)), rwdStartAction, SLOT(setEnabled(bool)));

    QAction *m_rwdAction = toolbar->addAction(il.load("rewind"),
                                              tr("Rewind"));
    m_rwdAction->setShortcut(tr("PgUp"));
    m_rwdAction->setStatusTip(tr("Rewind to the previous time instant or time ruler notch"));
    connect(m_rwdAction, SIGNAL(triggered()), this, SLOT(rewind()));
    connect(this, SIGNAL(canRewind(bool)), m_rwdAction, SLOT(setEnabled(bool)));

    QAction *playAction = toolbar->addAction(il.load("playpause"),
                                             tr("Play / Pause"));
    playAction->setCheckable(true);
    playAction->setShortcut(tr("Space"));
    playAction->setStatusTip(tr("Start or stop playback from the current position"));
    connect(playAction, SIGNAL(triggered()), this, SLOT(play()));
    connect(m_playSource, SIGNAL(playStatusChanged(bool)),
	    playAction, SLOT(setChecked(bool)));
    connect(this, SIGNAL(canPlay(bool)), playAction, SLOT(setEnabled(bool)));

    m_ffwdAction = toolbar->addAction(il.load("ffwd"),
                                              tr("Fast Forward"));
    m_ffwdAction->setShortcut(tr("PgDown"));
    m_ffwdAction->setStatusTip(tr("Fast-forward to the next time instant or time ruler notch"));
    connect(m_ffwdAction, SIGNAL(triggered()), this, SLOT(ffwd()));
    connect(this, SIGNAL(canFfwd(bool)), m_ffwdAction, SLOT(setEnabled(bool)));

    QAction *ffwdEndAction = toolbar->addAction(il.load("ffwd-end"),
                                                tr("Fast Forward to End"));
    ffwdEndAction->setShortcut(tr("End"));
    ffwdEndAction->setStatusTip(tr("Fast-forward to the end"));
    connect(ffwdEndAction, SIGNAL(triggered()), this, SLOT(ffwdEnd()));
    connect(this, SIGNAL(canPlay(bool)), ffwdEndAction, SLOT(setEnabled(bool)));

    toolbar = addToolBar(tr("Play Mode Toolbar"));

    QAction *psAction = toolbar->addAction(il.load("playselection"),
                                           tr("Constrain Playback to Selection"));
    psAction->setCheckable(true);
    psAction->setChecked(m_viewManager->getPlaySelectionMode());
    psAction->setShortcut(tr("s"));
    psAction->setStatusTip(tr("Constrain playback to the selected regions"));
    connect(m_viewManager, SIGNAL(playSelectionModeChanged(bool)),
            psAction, SLOT(setChecked(bool)));
    connect(psAction, SIGNAL(triggered()), this, SLOT(playSelectionToggled()));
    connect(this, SIGNAL(canPlaySelection(bool)), psAction, SLOT(setEnabled(bool)));

    QAction *plAction = toolbar->addAction(il.load("playloop"),
                                           tr("Loop Playback"));
    plAction->setCheckable(true);
    plAction->setChecked(m_viewManager->getPlayLoopMode());
    plAction->setShortcut(tr("l"));
    plAction->setStatusTip(tr("Loop playback"));
    connect(m_viewManager, SIGNAL(playLoopModeChanged(bool)),
            plAction, SLOT(setChecked(bool)));
    connect(plAction, SIGNAL(triggered()), this, SLOT(playLoopToggled()));
    connect(this, SIGNAL(canPlay(bool)), plAction, SLOT(setEnabled(bool)));

    m_soloAction = toolbar->addAction(il.load("solo"),
                                           tr("Solo Current Pane"));
    m_soloAction->setCheckable(true);
    m_soloAction->setChecked(m_viewManager->getPlaySoloMode());
    m_prevSolo = m_viewManager->getPlaySoloMode();
    m_soloAction->setShortcut(tr("o"));
    m_soloAction->setStatusTip(tr("Solo the current pane during playback"));
    connect(m_viewManager, SIGNAL(playSoloModeChanged(bool)),
            m_soloAction, SLOT(setChecked(bool)));
    connect(m_soloAction, SIGNAL(triggered()), this, SLOT(playSoloToggled()));
    connect(this, SIGNAL(canChangeSolo(bool)), m_soloAction, SLOT(setEnabled(bool)));

    QAction *alAction = 0;
    if (Document::canAlign()) {
        alAction = toolbar->addAction(il.load("align"),
                                      tr("Align File Timelines"));
        alAction->setCheckable(true);
        alAction->setChecked(m_viewManager->getAlignMode());
        alAction->setStatusTip(tr("Treat multiple audio files as versions of the same work, and align their timelines"));
        connect(m_viewManager, SIGNAL(alignModeChanged(bool)),
                alAction, SLOT(setChecked(bool)));
        connect(alAction, SIGNAL(triggered()), this, SLOT(alignToggled()));
        connect(this, SIGNAL(canAlign(bool)), alAction, SLOT(setEnabled(bool)));
    }

    m_keyReference->registerShortcut(playAction);
    m_keyReference->registerShortcut(psAction);
    m_keyReference->registerShortcut(plAction);
    m_keyReference->registerShortcut(m_soloAction);
    if (alAction) m_keyReference->registerShortcut(alAction);
    m_keyReference->registerShortcut(m_rwdAction);
    m_keyReference->registerShortcut(m_ffwdAction);
    m_keyReference->registerShortcut(rwdStartAction);
    m_keyReference->registerShortcut(ffwdEndAction);

    menu->addAction(playAction);
    menu->addAction(psAction);
    menu->addAction(plAction);
    menu->addAction(m_soloAction);
    if (alAction) menu->addAction(alAction);
    menu->addSeparator();
    menu->addAction(m_rwdAction);
    menu->addAction(m_ffwdAction);
    menu->addSeparator();
    menu->addAction(rwdStartAction);
    menu->addAction(ffwdEndAction);
    menu->addSeparator();

    m_rightButtonPlaybackMenu->addAction(playAction);
    m_rightButtonPlaybackMenu->addAction(psAction);
    m_rightButtonPlaybackMenu->addAction(plAction);
    m_rightButtonPlaybackMenu->addAction(m_soloAction);
    if (alAction) m_rightButtonPlaybackMenu->addAction(alAction);
    m_rightButtonPlaybackMenu->addSeparator();
    m_rightButtonPlaybackMenu->addAction(m_rwdAction);
    m_rightButtonPlaybackMenu->addAction(m_ffwdAction);
    m_rightButtonPlaybackMenu->addSeparator();
    m_rightButtonPlaybackMenu->addAction(rwdStartAction);
    m_rightButtonPlaybackMenu->addAction(ffwdEndAction);
    m_rightButtonPlaybackMenu->addSeparator();

    QAction *fastAction = menu->addAction(tr("Speed Up"));
    fastAction->setShortcut(tr("Ctrl+PgUp"));
    fastAction->setStatusTip(tr("Time-stretch playback to speed it up without changing pitch"));
    connect(fastAction, SIGNAL(triggered()), this, SLOT(speedUpPlayback()));
    connect(this, SIGNAL(canSpeedUpPlayback(bool)), fastAction, SLOT(setEnabled(bool)));
    
    QAction *slowAction = menu->addAction(tr("Slow Down"));
    slowAction->setShortcut(tr("Ctrl+PgDown"));
    slowAction->setStatusTip(tr("Time-stretch playback to slow it down without changing pitch"));
    connect(slowAction, SIGNAL(triggered()), this, SLOT(slowDownPlayback()));
    connect(this, SIGNAL(canSlowDownPlayback(bool)), slowAction, SLOT(setEnabled(bool)));

    QAction *normalAction = menu->addAction(tr("Restore Normal Speed"));
    normalAction->setShortcut(tr("Ctrl+Home"));
    normalAction->setStatusTip(tr("Restore non-time-stretched playback"));
    connect(normalAction, SIGNAL(triggered()), this, SLOT(restoreNormalPlayback()));
    connect(this, SIGNAL(canChangePlaybackSpeed(bool)), normalAction, SLOT(setEnabled(bool)));

    m_keyReference->registerShortcut(fastAction);
    m_keyReference->registerShortcut(slowAction);
    m_keyReference->registerShortcut(normalAction);

    m_rightButtonPlaybackMenu->addAction(fastAction);
    m_rightButtonPlaybackMenu->addAction(slowAction);
    m_rightButtonPlaybackMenu->addAction(normalAction);

    toolbar = addToolBar(tr("Edit Toolbar"));
    CommandHistory::getInstance()->registerToolbar(toolbar);

    m_keyReference->setCategory(tr("Tool Selection"));

    toolbar = addToolBar(tr("Tools Toolbar"));
    QActionGroup *group = new QActionGroup(this);

    QAction *action = toolbar->addAction(il.load("navigate"),
                                         tr("Navigate"));
    action->setCheckable(true);
    action->setChecked(true);
    action->setShortcut(tr("1"));
    action->setStatusTip(tr("Navigate"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolNavigateSelected()));
    group->addAction(action);
    m_keyReference->registerShortcut(action);
    m_toolActions[ViewManager::NavigateMode] = action;

    action = toolbar->addAction(il.load("select"),
				tr("Select"));
    action->setCheckable(true);
    action->setShortcut(tr("2"));
    action->setStatusTip(tr("Select ranges"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolSelectSelected()));
    group->addAction(action);
    m_keyReference->registerShortcut(action);
    m_toolActions[ViewManager::SelectMode] = action;

    action = toolbar->addAction(il.load("move"),
				tr("Edit"));
    action->setCheckable(true);
    action->setShortcut(tr("3"));
    action->setStatusTip(tr("Edit items in layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolEditSelected()));
    connect(this, SIGNAL(canEditLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_keyReference->registerShortcut(action);
    m_toolActions[ViewManager::EditMode] = action;

    action = toolbar->addAction(il.load("draw"),
				tr("Draw"));
    action->setCheckable(true);
    action->setShortcut(tr("4"));
    action->setStatusTip(tr("Draw new items in layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolDrawSelected()));
    connect(this, SIGNAL(canEditLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_keyReference->registerShortcut(action);
    m_toolActions[ViewManager::DrawMode] = action;

    action = toolbar->addAction(il.load("measure"),
				tr("Measure"));
    action->setCheckable(true);
    action->setShortcut(tr("5"));
    action->setStatusTip(tr("Make measurements in layer"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolMeasureSelected()));
    connect(this, SIGNAL(canMeasureLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_keyReference->registerShortcut(action);
    m_toolActions[ViewManager::MeasureMode] = action;

//    action = toolbar->addAction(il.load("text"),
//				tr("Text"));
//    action->setCheckable(true);
//    action->setShortcut(tr("5"));
//    connect(action, SIGNAL(triggered()), this, SLOT(toolTextSelected()));
//    group->addAction(action);
//    m_toolActions[ViewManager::TextMode] = action;

    toolNavigateSelected();

    Pane::registerShortcuts(*m_keyReference);
}

void
MainWindow::updateMenuStates()
{
    MainWindowBase::updateMenuStates();

    Pane *currentPane = 0;
    Layer *currentLayer = 0;

    if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentLayer = currentPane->getSelectedLayer();

    bool haveCurrentPane =
        (currentPane != 0);
    bool haveCurrentLayer =
        (haveCurrentPane &&
         (currentLayer != 0));
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
    
    emit canChangeSolo(havePlayTarget);
    emit canAlign(havePlayTarget && m_document && m_document->canAlign());

    emit canChangePlaybackSpeed(true);
    int v = m_playSpeed->value();
    emit canSpeedUpPlayback(v < m_playSpeed->maximum());
    emit canSlowDownPlayback(v > m_playSpeed->minimum());

    if (m_viewManager && 
        (m_viewManager->getToolMode() == ViewManager::MeasureMode)) {
        emit canDeleteSelection(haveCurrentLayer);
        m_deleteSelectedAction->setText(tr("&Delete Current Measurement"));
        m_deleteSelectedAction->setStatusTip(tr("Delete the measurement currently under the mouse pointer"));
    } else {
        emit canDeleteSelection(haveSelection && haveCurrentEditableLayer);
        m_deleteSelectedAction->setText(tr("&Delete Selected Items"));
        m_deleteSelectedAction->setStatusTip(tr("Delete items in current selection from the current layer"));
    }

    if (m_ffwdAction && m_rwdAction) {
        if (haveCurrentTimeInstantsLayer) {
            m_ffwdAction->setText(tr("Fast Forward to Next Instant"));
            m_ffwdAction->setStatusTip(tr("Fast forward to the next time instant in the current layer"));
            m_rwdAction->setText(tr("Rewind to Previous Instant"));
            m_rwdAction->setStatusTip(tr("Rewind to the previous time instant in the current layer"));
        } else if (haveCurrentTimeValueLayer) {
            m_ffwdAction->setText(tr("Fast Forward to Next Point"));
            m_ffwdAction->setStatusTip(tr("Fast forward to the next point in the current layer"));
            m_rwdAction->setText(tr("Rewind to Previous Point"));
            m_rwdAction->setStatusTip(tr("Rewind to the previous point in the current layer"));
        } else {
            m_ffwdAction->setText(tr("Fast Forward"));
            m_ffwdAction->setStatusTip(tr("Fast forward"));
            m_rwdAction->setText(tr("Rewind"));
            m_rwdAction->setStatusTip(tr("Rewind"));
        }
    }
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
    //!!!
    MainWindowBase::documentModified();
}

void
MainWindow::documentRestored()
{
    //!!!
    MainWindowBase::documentRestored();
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

void
MainWindow::toolMeasureSelected()
{
    m_viewManager->setToolMode(ViewManager::MeasureMode);
}

//void
//MainWindow::toolTextSelected()
//{
//    m_viewManager->setToolMode(ViewManager::TextMode);
//}

void
MainWindow::importAudio()
{
    QString path = getOpenFileName(FileFinder::AudioFile);

    if (path != "") {
	if (openAudio(path, ReplaceMainModel) == FileOpenFailed) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("<b>File open failed</b><p>Audio file \"%1\" could not be opened").arg(path));
	}
    }
}

void
MainWindow::importMoreAudio()
{
    QString path = getOpenFileName(FileFinder::AudioFile);

    if (path != "") {
	if (openAudio(path, CreateAdditionalModel) == FileOpenFailed) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("<b>File open failed</b><p>Audio file \"%1\" could not be opened").arg(path));
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

        FileOpenStatus status = openLayer(path);
        
        if (status == FileOpenFailed) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("<b>File open failed</b><p>Layer file %1 could not be opened.").arg(path));
            return;
        } else if (status == FileOpenWrongMode) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
        }
    }
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

    FileFinder::FileType type = FileFinder::LayerFileNoMidi;

    if (dynamic_cast<NoteModel *>(model)) type = FileFinder::LayerFile;

    QString path = getSaveFileName(type);

    if (path == "") return;

    if (QFileInfo(path).suffix() == "") path += ".svl";

    QString suffix = QFileInfo(path).suffix().toLower();

    QString error;

    if (suffix == "xml" || suffix == "svl") {

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

    } else if (suffix == "mid" || suffix == "midi") {

        NoteModel *nm = dynamic_cast<NoteModel *>(model);

        if (!nm) {
            error = tr("Can't export non-note layers to MIDI");
        } else {
            MIDIFileWriter writer(path, nm);
            writer.write();
            if (!writer.isOK()) {
                error = writer.getError();
            }
        }

    } else {

        CSVFileWriter writer(path, model,
                             ((suffix == "csv") ? "," : "\t"));
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
			      tr("<b>File open failed</b><p>Session file \"%1\" could not be opened").arg(path));
    }
}

void
MainWindow::openSomething()
{
    QString orig = m_audioFile;
    if (orig == "") orig = ".";
    else orig = QFileInfo(orig).absoluteDir().canonicalPath();

    QString path = getOpenFileName(FileFinder::AnyFile);

    if (path.isEmpty()) return;

    FileOpenStatus status = open(path, AskUser);

    if (status == FileOpenFailed) {
        QMessageBox::critical(this, tr("Failed to open file"),
                              tr("<b>File open failed</b><p>File \"%1\" could not be opened").arg(path));
    } else if (status == FileOpenWrongMode) {
        QMessageBox::critical(this, tr("Failed to open file"),
                              tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
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

    FileOpenStatus status = open(text);

    if (status == FileOpenFailed) {
        QMessageBox::critical(this, tr("Failed to open location"),
                              tr("<b>Open failed</b><p>URL \"%1\" could not be opened").arg(text));
    } else if (status == FileOpenWrongMode) {
        QMessageBox::critical(this, tr("Failed to open location"),
                              tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
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

    FileOpenStatus status = open(path);

    if (status == FileOpenFailed) {
        QMessageBox::critical(this, tr("Failed to open location"),
                              tr("<b>Open failed</b><p>File or URL \"%1\" could not be opened").arg(path));
    } else if (status == FileOpenWrongMode) {
        QMessageBox::critical(this, tr("Failed to open location"),
                              tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
    }
}

void
MainWindow::paneAdded(Pane *pane)
{
    if (m_overview) m_overview->registerView(pane);
}    

void
MainWindow::paneHidden(Pane *pane)
{
    if (m_overview) m_overview->unregisterView(pane); 
}    

void
MainWindow::paneAboutToBeDeleted(Pane *pane)
{
    if (m_overview) m_overview->unregisterView(pane); 
}    

void
MainWindow::paneDropAccepted(Pane *pane, QStringList uriList)
{
    if (pane) m_paneStack->setCurrentPane(pane);

    for (QStringList::iterator i = uriList.begin(); i != uriList.end(); ++i) {

        FileOpenStatus status = open(*i, ReplaceCurrentPane);

        if (status == FileOpenFailed) {
            QMessageBox::critical(this, tr("Failed to open dropped URL"),
                                  tr("<b>Open failed</b><p>Dropped URL \"%1\" could not be opened").arg(*i));
        } else if (status == FileOpenWrongMode) {
            QMessageBox::critical(this, tr("Failed to open dropped URL"),
                                  tr("<b>Audio required</b><p>Please load at least one audio file before importing annotation data"));
        }
    }
}

void
MainWindow::paneDropAccepted(Pane *pane, QString text)
{
    if (pane) m_paneStack->setCurrentPane(pane);

    QUrl testUrl(text);
    if (testUrl.scheme() == "file" || 
        testUrl.scheme() == "http" || 
        testUrl.scheme() == "ftp") {
        QStringList list;
        list.push_back(text);
        paneDropAccepted(pane, list);
        return;
    }

    //!!! open as text -- but by importing as if a CSV, or just adding
    //to a text layer?
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

    delete m_keyReference;
    m_keyReference = 0;

    if (m_preferencesDialog &&
        m_preferencesDialog->isVisible()) {
        closeSession(); // otherwise we'll have to wait for prefs changes
        m_preferencesDialog->applicationClosing(false);
    }

    if (m_layerTreeView &&
        m_layerTreeView->isVisible()) {
        delete m_layerTreeView;
    }

    closeSession();

    e->accept();
    return;
}

bool
MainWindow::commitData(bool mayAskUser)
{
    if (mayAskUser) {
        bool rv = checkSaveModified();
        if (rv) {
            if (m_preferencesDialog &&
                m_preferencesDialog->isVisible()) {
                m_preferencesDialog->applicationClosing(false);
            }
        }
        return rv;
    } else {
        if (m_preferencesDialog &&
            m_preferencesDialog->isVisible()) {
            m_preferencesDialog->applicationClosing(true);
        }
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
			     tr("<b>Session modified</b><p>The current session has been modified.<br>Do you want to save it?"),
			     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                             QMessageBox::Yes);

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
				  tr("<b>Save failed</b><p>Session file \"%1\" could not be saved.").arg(m_sessionFile));
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
			      tr("<b>Save failed</b><p>Session file \"%1\" could not be saved.").arg(path));
    } else {
	setWindowTitle(tr("Sonic Visualiser: %1")
		       .arg(QFileInfo(path).fileName()));
	m_sessionFile = path;
	CommandHistory::getInstance()->documentSaved();
	documentRestored();
        m_recentFiles.addFile(path);
    }
}

void
MainWindow::preferenceChanged(PropertyContainer::PropertyName name)
{
    MainWindowBase::preferenceChanged(name);

    if (name == "Background Mode" && m_viewManager) {
        if (m_viewManager->getGlobalDarkBackground()) {
            m_panLayer->setBaseColour
                (ColourDatabase::getInstance()->getColourIndex(tr("Bright Green")));
        } else {
            m_panLayer->setBaseColour
                (ColourDatabase::getInstance()->getColourIndex(tr("Green")));
        }      
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
        std::vector<Model *> inputModels = m_document->getTransformerInputModels();
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
            //!!!???
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

    TransformerActionMap::iterator i = m_transformActions.find(action);

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

    TransformerId transform = i->second;
    TransformerFactory *factory = TransformerFactory::getInstance();

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

    PluginTransformer::ExecutionContext context(channel);

    std::vector<Model *> candidateInputModels =
        m_document->getTransformerInputModels();

    size_t startFrame = 0, duration = 0;
    size_t endFrame = 0;
    m_viewManager->getSelection().getExtents(startFrame, endFrame);
    if (endFrame > startFrame) duration = endFrame - startFrame;
    else startFrame = 0;

    Model *inputModel = factory->getConfigurationForTransformer(transform,
                                                              candidateInputModels,
                                                              context,
                                                              configurationXml,
                                                              m_playSource,
                                                              startFrame,
                                                              duration);
    if (!inputModel) return;

//    std::cerr << "MainWindow::addLayer: Input model is " << inputModel << " \"" << inputModel->objectName().toStdString() << "\"" << std::endl;

    Layer *newLayer = m_document->createDerivedLayer(transform,
                                                     inputModel,
                                                     context,
                                                     configurationXml);

    if (newLayer) {
        m_document->addLayerToView(pane, newLayer);
        m_document->setChannel(newLayer, context.channel);
        m_recentTransformers.add(transform);
        m_paneStack->setCurrentLayer(pane, newLayer);
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
MainWindow::playSoloToggled()
{
    MainWindowBase::playSoloToggled();
    m_soloModified = true;
}

void
MainWindow::alignToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (!m_viewManager) return;

    if (action) {
	m_viewManager->setAlignMode(action->isChecked());
    } else {
	m_viewManager->setAlignMode(!m_viewManager->getAlignMode());
    }

    if (m_viewManager->getAlignMode()) {
        m_prevSolo = m_soloAction->isChecked();
        if (!m_soloAction->isChecked()) {
            m_soloAction->setChecked(true);
            MainWindowBase::playSoloToggled();
        }
        m_soloModified = false;
        emit canChangeSolo(false);
        m_document->alignModels();
        m_document->setAutoAlignment(true);
    } else {
        if (!m_soloModified) {
            if (m_soloAction->isChecked() != m_prevSolo) {
                m_soloAction->setChecked(m_prevSolo);
                MainWindowBase::playSoloToggled();
            }
        }
        emit canChangeSolo(true);
        m_document->setAutoAlignment(false);
    }

    for (int i = 0; i < m_paneStack->getPaneCount(); ++i) {

	Pane *pane = m_paneStack->getPane(i);
	if (!pane) continue;

        pane->update();
    }
}

void
MainWindow::playSpeedChanged(int position)
{
    PlaySpeedRangeMapper mapper(0, 200);

    float percent = m_playSpeed->mappedValue();
    float factor = mapper.getFactorForValue(percent);

    std::cerr << "speed = " << position << " percent = " << percent << " factor = " << factor << std::endl;

    bool something = (position != 100);

    int pc = lrintf(percent);

    if (!something) {
        contextHelpChanged(tr("Playback speed: Normal"));
    } else {
        contextHelpChanged(tr("Playback speed: %1%2%")
                           .arg(position > 100 ? "+" : "")
                           .arg(pc));
    }

    m_playSharpen->setEnabled(something);
    m_playMono->setEnabled(something);
    bool sharpen = (something && m_playSharpen->isChecked());
    bool mono = (something && m_playMono->isChecked());
    m_playSource->setTimeStretch(factor, sharpen, mono);

    updateMenuStates();
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
MainWindow::speedUpPlayback()
{
    int value = m_playSpeed->value();
    value = value + m_playSpeed->pageStep();
    if (value > m_playSpeed->maximum()) value = m_playSpeed->maximum();
    m_playSpeed->setValue(value);
}

void
MainWindow::slowDownPlayback()
{
    int value = m_playSpeed->value();
    value = value - m_playSpeed->pageStep();
    if (value < m_playSpeed->minimum()) value = m_playSpeed->minimum();
    m_playSpeed->setValue(value);
}

void
MainWindow::restoreNormalPlayback()
{
    m_playSpeed->setValue(m_playSpeed->defaultValue());
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
        QMessageBox::information
            (this, tr("Sample rate mismatch"),
             tr("<b>Wrong sample rate</b><p>The sample rate of this audio file (%1 Hz) does not match\nthe current playback rate (%2 Hz).<p>The file will play at the wrong speed and pitch.<p>Change the <i>Resample mismatching files on import</i> option under <i>File</i> -> <i>Preferences</i> if you want to alter this behaviour.")
             .arg(requested).arg(actual));
    }        

    updateDescriptionLabel();
}

void
MainWindow::audioOverloadPluginDisabled()
{
    QMessageBox::information
        (this, tr("Audio processing overload"),
         tr("<b>Overloaded</b><p>Audio effects plugin auditioning has been disabled due to a processing overload."));
}

void
MainWindow::layerRemoved(Layer *layer)
{
    setupExistingLayersMenus();
    MainWindowBase::layerRemoved(layer);
}

void
MainWindow::layerInAView(Layer *layer, bool inAView)
{
    setupExistingLayersMenus();
    MainWindowBase::layerInAView(layer, inAView);
}

void
MainWindow::modelAdded(Model *model)
{
    MainWindowBase::modelAdded(model);
    if (dynamic_cast<DenseTimeValueModel *>(model)) {
        setupPaneAndLayerMenus();
    }
}

void
MainWindow::mainModelChanged(WaveFileModel *model)
{
    m_panLayer->setModel(model);

    MainWindowBase::mainModelChanged(model);

    if (m_playTarget) {
        connect(m_fader, SIGNAL(valueChanged(float)),
                m_playTarget, SLOT(setOutputGain(float)));
    }
}

void
MainWindow::setInstantsNumbering()
{
    QAction *a = dynamic_cast<QAction *>(sender());
    if (!a) return;

    int type = m_numberingActions[a];
    
    if (m_labeller) m_labeller->setType(Labeller::ValueType(type));

    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("labellertype", type);
    settings.endGroup();
}

void
MainWindow::setInstantsCounterCycle()
{
    QAction *a = dynamic_cast<QAction *>(sender());
    if (!a) return;
    
    int cycle = a->text().toInt();
    if (cycle == 0) return;

    if (m_labeller) m_labeller->setCounterCycleSize(cycle);
    
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("labellercycle", cycle);
    settings.endGroup();
}

void
MainWindow::resetInstantsCounters()
{
    LabelCounterInputDialog dialog(m_labeller, this);
    dialog.exec();
}

void
MainWindow::modelGenerationFailed(QString transformName)
{
    QMessageBox::warning
        (this,
         tr("Failed to generate layer"),
         tr("<b>Layer generation failed</b><p>Failed to generate a derived layer.<p>The layer transform \"%1\" failed.<p>This may mean that a plugin failed to initialise, perhaps because it rejected the processing block size that was requested.")
         .arg(transformName),
         QMessageBox::Ok);
}

void
MainWindow::modelRegenerationFailed(QString layerName, QString transformName)
{
    QMessageBox::warning
        (this,
         tr("Failed to regenerate layer"),
         tr("<b>Layer generation failed</b><p>Failed to regenerate derived layer \"%1\".<p>The layer transform \"%2\" failed to run.<p>This may mean that the layer used a plugin that is not currently available.")
         .arg(layerName).arg(transformName),
         QMessageBox::Ok);
}

void
MainWindow::rightButtonMenuRequested(Pane *pane, QPoint position)
{
//    std::cerr << "MainWindow::rightButtonMenuRequested(" << pane << ", " << position.x() << ", " << position.y() << ")" << std::endl;
    m_paneStack->setCurrentPane(pane);
    m_rightButtonMenu->popup(position);
}

void
MainWindow::showLayerTree()
{
    if (!m_layerTreeView.isNull()) {
        m_layerTreeView->show();
        m_layerTreeView->raise();
        return;
    }

    //!!! should use an actual dialog class
        
    m_layerTreeView = new QTreeView();
    LayerTreeModel *tree = new LayerTreeModel(m_paneStack);
    m_layerTreeView->resize(500, 300); //!!!
    m_layerTreeView->setModel(tree);
    m_layerTreeView->expandAll();
    m_layerTreeView->show();
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
            if (open(path, ReplaceMainModel) != FileOpenSucceeded) {
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
            if (open(path, CreateAdditionalModel) != FileOpenSucceeded) {
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
            if (open(recent[n], ReplaceMainModel) != FileOpenSucceeded) {
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

    } else if (message.getMethod() == "solo") {

        if (message.getArgCount() == 1 &&
            message.getArg(0).canConvert(QVariant::String)) {

            QString str = message.getArg(0).toString();
            if (str == "on") {
                m_viewManager->setPlaySoloMode(true);
            } else if (str == "off") {
                m_viewManager->setPlaySoloMode(false);
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

            TransformerId transform = message.getArg(0).toString();

            Layer *newLayer = m_document->createDerivedLayer
                (transform,
                 getMainModel(),
                 TransformerFactory::getInstance()->getDefaultContextForTransformer
                 (transform, getMainModel()),
                 "");

            if (newLayer) {
                m_document->addLayerToView(pane, newLayer);
                m_recentTransformers.add(transform);
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

void
MainWindow::keyReference()
{
    m_keyReference->show();
}

