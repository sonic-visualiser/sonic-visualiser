/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
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
#include "widgets/Fader.h"
#include "view/Panner.h"
#include "widgets/PropertyBox.h"
#include "widgets/PropertyStack.h"
#include "widgets/AudioDial.h"
#include "widgets/LayerTree.h"
#include "widgets/ListInputDialog.h"
#include "audioio/AudioCallbackPlaySource.h"
#include "audioio/AudioCallbackPlayTarget.h"
#include "audioio/AudioTargetFactory.h"
#include "data/fileio/AudioFileReaderFactory.h"
#include "data/fileio/DataFileReaderFactory.h"
#include "data/fileio/WavFileWriter.h"
#include "data/fileio/CSVFileWriter.h"
#include "data/fileio/BZipFileDevice.h"
#include "base/RecentFiles.h"
#include "transform/TransformFactory.h"
#include "base/PlayParameterRepository.h"
#include "base/XmlExportable.h"
#include "base/CommandHistory.h"
#include "base/Profiler.h"
#include "base/Clipboard.h"

// For version information
#include "vamp/vamp.h"
#include "vamp-sdk/PluginBase.h"
#include "plugin/api/ladspa.h"
#include "plugin/api/dssi.h"

#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
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
#include <QTextStream>
#include <QProcess>
#include <QShortcut>
#include <QSettings>
#include <QDateTime>
#include <QProcess>
#include <QCheckBox>

#include <iostream>
#include <cstdio>
#include <errno.h>

using std::cerr;
using std::endl;

using std::vector;
using std::map;
using std::set;


MainWindow::MainWindow() :
    m_document(0),
    m_paneStack(0),
    m_viewManager(0),
    m_panner(0),
    m_timeRulerLayer(0),
    m_playSource(0),
    m_playTarget(0),
    m_recentFiles("RecentFiles"),
    m_recentTransforms("RecentTransforms"),
    m_mainMenusCreated(false),
    m_paneMenu(0),
    m_layerMenu(0),
    m_transformsMenu(0),
    m_existingLayersMenu(0),
    m_recentFilesMenu(0),
    m_recentTransformsMenu(0),
    m_rightButtonMenu(0),
    m_rightButtonLayerMenu(0),
    m_rightButtonTransformsMenu(0),
    m_documentModified(false),
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

    m_descriptionLabel = new QLabel;

    m_paneStack = new PaneStack(frame, m_viewManager);
    connect(m_paneStack, SIGNAL(currentPaneChanged(Pane *)),
	    this, SLOT(currentPaneChanged(Pane *)));
    connect(m_paneStack, SIGNAL(currentLayerChanged(Pane *, Layer *)),
	    this, SLOT(currentLayerChanged(Pane *, Layer *)));
    connect(m_paneStack, SIGNAL(rightButtonMenuRequested(Pane *, QPoint)),
            this, SLOT(rightButtonMenuRequested(Pane *, QPoint)));

    m_panner = new Panner(frame);
    m_panner->setViewManager(m_viewManager);
    m_panner->setFixedHeight(40);

    m_panLayer = new WaveformLayer;
    m_panLayer->setChannelMode(WaveformLayer::MergeChannels);
//    m_panLayer->setScale(WaveformLayer::MeterScale);
    m_panLayer->setAutoNormalize(true);
    m_panLayer->setBaseColour(Qt::darkGreen);
    m_panLayer->setAggressiveCacheing(true);
    m_panner->addLayer(m_panLayer);

    m_playSource = new AudioCallbackPlaySource(m_viewManager);

    connect(m_playSource, SIGNAL(sampleRateMismatch(size_t, size_t, bool)),
	    this,           SLOT(sampleRateMismatch(size_t, size_t, bool)));

    m_fader = new Fader(frame, false);

    m_playSpeed = new AudioDial(frame);
    m_playSpeed->setMinimum(0);
    m_playSpeed->setMaximum(199);
    m_playSpeed->setValue(100);
    m_playSpeed->setFixedWidth(24);
    m_playSpeed->setFixedHeight(24);
    m_playSpeed->setNotchesVisible(true);
    m_playSpeed->setPageStep(10);
    m_playSpeed->setToolTip(tr("Playback speed: +0%"));
    m_playSpeed->setDefaultValue(100);
    connect(m_playSpeed, SIGNAL(valueChanged(int)),
	    this, SLOT(playSpeedChanged(int)));

    m_playSharpen = new QPushButton(frame);
    m_playSharpen->setToolTip(tr("Sharpen percussive transients"));
    m_playSharpen->setFixedSize(20, 20);
//    m_playSharpen->setFlat(true);
    m_playSharpen->setEnabled(false);
    m_playSharpen->setCheckable(true);
    m_playSharpen->setChecked(false);
    m_playSharpen->setIcon(QIcon(":icons/sharpen.png"));
    connect(m_playSharpen, SIGNAL(clicked()), this, SLOT(playSharpenToggled()));

    m_playMono = new QPushButton(frame);
    m_playMono->setToolTip(tr("Run time stretcher in mono only"));
    m_playMono->setFixedSize(20, 20);
//    m_playMono->setFlat(true);
    m_playMono->setEnabled(false);
    m_playMono->setCheckable(true);
    m_playMono->setChecked(false);
    m_playMono->setIcon(QIcon(":icons/mono.png"));
    connect(m_playMono, SIGNAL(clicked()), this, SLOT(playMonoToggled()));

    QSettings settings;
    settings.beginGroup("MainWindow");
    m_playSharpen->setChecked(settings.value("playsharpen", true).toBool());
    m_playMono->setChecked(settings.value("playmono", false).toBool());
    settings.endGroup();

    layout->addWidget(m_paneStack, 0, 0, 1, 5);
    layout->addWidget(m_panner, 1, 0);
    layout->addWidget(m_fader, 1, 1);
    layout->addWidget(m_playSpeed, 1, 2);
    layout->addWidget(m_playSharpen, 1, 3);
    layout->addWidget(m_playMono, 1, 4);

    layout->setColumnStretch(0, 10);

    frame->setLayout(layout);

    connect(m_viewManager, SIGNAL(outputLevelsChanged(float, float)),
	    this, SLOT(outputLevelsChanged(float, float)));

    connect(Preferences::getInstance(),
            SIGNAL(propertyChanged(PropertyContainer::PropertyName)),
            this,
            SLOT(preferenceChanged(PropertyContainer::PropertyName)));

    setupMenus();
    setupToolbars();

    statusBar()->addWidget(m_descriptionLabel);

    newSession();
}

MainWindow::~MainWindow()
{
    closeSession();
    delete m_playTarget;
    delete m_playSource;
    delete m_viewManager;
    Profiles::getInstance()->dump();
}

void
MainWindow::setupMenus()
{
    QAction *action = 0;
    QMenu *menu = 0;
    QToolBar *toolbar = 0;

    if (!m_mainMenusCreated) {
        m_rightButtonMenu = new QMenu();
    }

    if (m_rightButtonLayerMenu) {
        m_rightButtonLayerMenu->clear();
    } else {
        m_rightButtonLayerMenu = m_rightButtonMenu->addMenu(tr("&Layer"));
        m_rightButtonMenu->addSeparator();
    }

    if (m_rightButtonTransformsMenu) {
        m_rightButtonTransformsMenu->clear();
    } else {
        m_rightButtonTransformsMenu = m_rightButtonMenu->addMenu(tr("&Transform"));
        m_rightButtonMenu->addSeparator();
    }

    if (!m_mainMenusCreated) {

        CommandHistory::getInstance()->registerMenu(m_rightButtonMenu);
        m_rightButtonMenu->addSeparator();

	menu = menuBar()->addMenu(tr("&File"));
        toolbar = addToolBar(tr("File Toolbar"));

        QIcon icon(":icons/filenew.png");
        icon.addFile(":icons/filenew-22.png");
	action = new QAction(icon, tr("&New Session"), this);
	action->setShortcut(tr("Ctrl+N"));
	action->setStatusTip(tr("Clear the current Sonic Visualiser session and start a new one"));
	connect(action, SIGNAL(triggered()), this, SLOT(newSession()));
	menu->addAction(action);
        toolbar->addAction(action);
	
        icon = QIcon(":icons/fileopen.png");
        icon.addFile(":icons/fileopen-22.png");

	action = new QAction(icon, tr("&Open Session..."), this);
	action->setShortcut(tr("Ctrl+O"));
	action->setStatusTip(tr("Open a previously saved Sonic Visualiser session file"));
	connect(action, SIGNAL(triggered()), this, SLOT(openSession()));
	menu->addAction(action);

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

	action = new QAction(tr("&Import Audio File..."), this);
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
        m_recentFilesMenu = menu->addMenu(tr("&Recent Files"));
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
	connect(action, SIGNAL(triggered()), this, SLOT(close()));
	menu->addAction(action);

	menu = menuBar()->addMenu(tr("&Edit"));
	CommandHistory::getInstance()->registerMenu(menu);

	menu->addSeparator();

	action = new QAction(QIcon(":/icons/editcut.png"),
			     tr("Cu&t"), this);
	action->setShortcut(tr("Ctrl+X"));
	connect(action, SIGNAL(triggered()), this, SLOT(cut()));
	connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);

	action = new QAction(QIcon(":/icons/editcopy.png"),
			     tr("&Copy"), this);
	action->setShortcut(tr("Ctrl+C"));
	connect(action, SIGNAL(triggered()), this, SLOT(copy()));
	connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);

	action = new QAction(QIcon(":/icons/editpaste.png"),
			     tr("&Paste"), this);
	action->setShortcut(tr("Ctrl+V"));
	connect(action, SIGNAL(triggered()), this, SLOT(paste()));
	connect(this, SIGNAL(canPaste(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);

	action = new QAction(tr("&Delete Selected Items"), this);
	action->setShortcut(tr("Del"));
	connect(action, SIGNAL(triggered()), this, SLOT(deleteSelected()));
	connect(this, SIGNAL(canEditSelection(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);

	menu->addSeparator();
        m_rightButtonMenu->addSeparator();
	
	action = new QAction(tr("Select &All"), this);
	action->setShortcut(tr("Ctrl+A"));
	connect(action, SIGNAL(triggered()), this, SLOT(selectAll()));
	connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);
	
	action = new QAction(tr("Select &Visible Range"), this);
	action->setShortcut(tr("Ctrl+Shift+A"));
	connect(action, SIGNAL(triggered()), this, SLOT(selectVisible()));
	connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
	
	action = new QAction(tr("Select to &Start"), this);
	action->setShortcut(tr("Shift+Left"));
	connect(action, SIGNAL(triggered()), this, SLOT(selectToStart()));
	connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
	
	action = new QAction(tr("Select to &End"), this);
	action->setShortcut(tr("Shift+Right"));
	connect(action, SIGNAL(triggered()), this, SLOT(selectToEnd()));
	connect(this, SIGNAL(canSelect(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);

	action = new QAction(tr("C&lear Selection"), this);
	action->setShortcut(tr("Esc"));
	connect(action, SIGNAL(triggered()), this, SLOT(clearSelection()));
	connect(this, SIGNAL(canClearSelection(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        m_rightButtonMenu->addAction(action);

	menu->addSeparator();

	action = new QAction(tr("&Insert Instant at Playback Position"), this);
	action->setShortcut(tr("Enter"));
	connect(action, SIGNAL(triggered()), this, SLOT(insertInstant()));
	connect(this, SIGNAL(canInsertInstant(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);

        // Laptop shortcut (no keypad Enter key)
        connect(new QShortcut(tr(";"), this), SIGNAL(activated()),
                this, SLOT(insertInstant()));

	menu = menuBar()->addMenu(tr("&View"));

        QActionGroup *overlayGroup = new QActionGroup(this);
        
        action = new QAction(tr("&No Text Overlays"), this);
	action->setShortcut(tr("0"));
	action->setStatusTip(tr("Show no texts for frame times, layer names etc"));
	connect(action, SIGNAL(triggered()), this, SLOT(showNoOverlays()));
        action->setCheckable(true);
        action->setChecked(false);
        overlayGroup->addAction(action);
	menu->addAction(action);
        
        action = new QAction(tr("Basic &Text Overlays"), this);
	action->setShortcut(tr("9"));
	action->setStatusTip(tr("Show texts for frame times etc, but not layer names etc"));
	connect(action, SIGNAL(triggered()), this, SLOT(showBasicOverlays()));
        action->setCheckable(true);
        action->setChecked(true);
        overlayGroup->addAction(action);
	menu->addAction(action);
        
        action = new QAction(tr("&All Text Overlays"), this);
	action->setShortcut(tr("8"));
	action->setStatusTip(tr("Show texts for frame times, layer names etc"));
	connect(action, SIGNAL(triggered()), this, SLOT(showAllTextOverlays()));
        action->setCheckable(true);
        action->setChecked(false);
        overlayGroup->addAction(action);
	menu->addAction(action);

	menu->addSeparator();
	
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
	
	action = new QAction(tr("Jump Left"), this);
	action->setShortcut(tr("Ctrl+Left"));
	action->setStatusTip(tr("Scroll the current pane a big step to the left"));
	connect(action, SIGNAL(triggered()), this, SLOT(jumpLeft()));
	connect(this, SIGNAL(canScroll(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
	
	action = new QAction(tr("Jump Right"), this);
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
	connect(action, SIGNAL(triggered()), this, SLOT(zoomDefault()));
	connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);

        action = new QAction(tr("Zoom to &Fit"), this);
	action->setStatusTip(tr("Zoom to show the whole file"));
	connect(action, SIGNAL(triggered()), this, SLOT(zoomToFit()));
	connect(this, SIGNAL(canZoom(bool)), action, SLOT(setEnabled(bool)));
	menu->addAction(action);
        
        action = new QAction(tr("Show &Zoom Wheels"), this);
	action->setShortcut(tr("Z"));
	action->setStatusTip(tr("Show thumbwheels for zooming horizontally and vertically"));
	connect(action, SIGNAL(triggered()), this, SLOT(toggleZoomWheels()));
        action->setCheckable(true);
        action->setChecked(m_viewManager->getZoomWheelsEnabled());
	menu->addAction(action);

/*!!! This one doesn't work properly yet

	menu->addSeparator();

	action = new QAction(tr("Show &Layer Hierarchy"), this);
	action->setShortcut(tr("Alt+L"));
	connect(action, SIGNAL(triggered()), this, SLOT(showLayerTree()));
	menu->addAction(action);
*/
    }

    if (m_paneMenu) {
	m_paneActions.clear();
	m_paneMenu->clear();
    } else {
	m_paneMenu = menuBar()->addMenu(tr("&Pane"));
    }

    if (m_layerMenu) {
	m_layerActions.clear();
	m_layerMenu->clear();
    } else {
	m_layerMenu = menuBar()->addMenu(tr("&Layer"));
    }

    if (m_transformsMenu) {
        m_transformActions.clear();
        m_transformActionsReverse.clear();
        m_transformsMenu->clear();
    } else {
	m_transformsMenu = menuBar()->addMenu(tr("&Transform"));
    }

    TransformFactory::TransformList transforms =
	TransformFactory::getInstance()->getAllTransforms();

    vector<QString> types =
        TransformFactory::getInstance()->getAllTransformTypes();

    map<QString, map<QString, QMenu *> > categoryMenus;
    map<QString, map<QString, QMenu *> > makerMenus;

    map<QString, QMenu *> byPluginNameMenus;
    map<QString, map<QString, QMenu *> > pluginNameMenus;

    m_recentTransformsMenu = m_transformsMenu->addMenu(tr("&Recent Transforms"));
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
        QMenu *byCategoryMenu = m_transformsMenu->addMenu(byCategoryLabel);
        m_rightButtonTransformsMenu->addMenu(byCategoryMenu);

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
                    if (parentKey == "") {
                        categoryMenus[*i][key] = byCategoryMenu->addMenu(*k);
                    } else {
                        categoryMenus[*i][key] =
                            categoryMenus[*i][parentKey]->addMenu(*k);
                    }
                }
            }
        }

        QString byPluginNameLabel = tr("%1 by Plugin Name").arg(*i);
        byPluginNameMenus[*i] = m_transformsMenu->addMenu(byPluginNameLabel);
        m_rightButtonTransformsMenu->addMenu(byPluginNameMenus[*i]);

        QString byMakerLabel = tr("%1 by Maker").arg(*i);
        QMenu *byMakerMenu = m_transformsMenu->addMenu(byMakerLabel);
        m_rightButtonTransformsMenu->addMenu(byMakerMenu);

        vector<QString> makers = 
            TransformFactory::getInstance()->getTransformMakers(*i);
        
        for (vector<QString>::iterator j = makers.begin();
             j != makers.end(); ++j) {

            QString maker = *j;
            if (maker == "") maker = tr("Unknown");
            
            makerMenus[*i][maker] = byMakerMenu->addMenu(maker);
        }
    }

    map<QString, set<QString> > pluginNameLists;
    map<QString, map<QString, QMenu *> > pluginNameToChunkMenuMap;

    for (unsigned int i = 0; i < transforms.size(); ++i) {
	QString description = transforms[i].description;
	if (description == "") description = transforms[i].name;
        QString type = transforms[i].type;
        QString pluginName = description.section(": ", 0, 0);
        pluginNameLists[type].insert(pluginName);
    }

    for (vector<QString>::iterator i = types.begin(); i != types.end(); ++i) {

        size_t total = pluginNameLists[*i].size();
        size_t chunk = 14;
        
        if (total < (chunk * 3) / 2) continue;

        size_t count = 0;
        QMenu *chunkMenu = new QMenu();

        QString firstNameInChunk;
        QChar firstInitialInChunk;
        bool discriminateStartInitial = false;

        for (set<QString>::iterator j = pluginNameLists[*i].begin();
             j != pluginNameLists[*i].end();
             ++j) {

            pluginNameToChunkMenuMap[*i][*j] = chunkMenu;

            set<QString>::iterator k = j;
            ++k;

            QChar initial = (*j)[0];

            if (count == 0) {
                firstNameInChunk = *j;
                firstInitialInChunk = initial;
            }

            bool lastInChunk = (k == pluginNameLists[*i].end() ||
                                (count >= chunk-1 &&
                                 (count == (5*chunk) / 2 ||
                                  (*k)[0] != initial)));

            ++count;

            if (lastInChunk) {

                bool discriminateEndInitial = (k != pluginNameLists[*i].end() &&
                                               (*k)[0] == initial);

                bool initialsEqual = (firstInitialInChunk == initial);

                QString from = QString("%1").arg(firstInitialInChunk);
                if (discriminateStartInitial ||
                    (discriminateEndInitial && initialsEqual)) {
                    from = firstNameInChunk.left(3);
                }

                QString to = QString("%1").arg(initial);
                if (discriminateEndInitial ||
                    (discriminateStartInitial && initialsEqual)) {
                    to = j->left(3);
                }

                QString menuText;

                if (from == to) menuText = from;
                else menuText = tr("%1 - %2").arg(from).arg(to);

                discriminateStartInitial = discriminateEndInitial;

                chunkMenu->setTitle(menuText);
                
                byPluginNameMenus[*i]->addMenu(chunkMenu);

                chunkMenu = new QMenu();

                count = 0;
            }
        }

        if (count == 0) delete chunkMenu;
    }
    
    for (unsigned int i = 0; i < transforms.size(); ++i) {
	
	QString description = transforms[i].description;
	if (description == "") description = transforms[i].name;

        QString type = transforms[i].type;

        QString category = transforms[i].category;
        if (category == "") category = tr("Unclassified");

        QString maker = transforms[i].maker;
        if (maker == "") maker = tr("Unknown");

        QString pluginName = description.section(": ", 0, 0);
        QString output = description.section(": ", 1);

	action = new QAction(tr("%1...").arg(description), this);
	connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
	m_transformActions[action] = transforms[i].name;
        m_transformActionsReverse[transforms[i].name] = action;
	connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));

        if (categoryMenus[type].find(category) == categoryMenus[type].end()) {
            std::cerr << "WARNING: MainWindow::setupMenus: Internal error: "
                      << "No category menu for transform \""
                      << description.toStdString() << "\" (category = \""
                      << category.toStdString() << "\")" << std::endl;
        } else {
            categoryMenus[type][category]->addAction(action);
        }

        if (makerMenus[type].find(maker) == makerMenus[type].end()) {
            std::cerr << "WARNING: MainWindow::setupMenus: Internal error: "
                      << "No maker menu for transform \""
                      << description.toStdString() << "\" (maker = \""
                      << maker.toStdString() << "\")" << std::endl;
        } else {
            makerMenus[type][maker]->addAction(action);
        }

        action = new QAction(tr("%1...").arg(output == "" ? pluginName : output), this);
        connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
        m_transformActions[action] = transforms[i].name;
        connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));

//        cerr << "Transform: \"" << name.toStdString() << "\": plugin name \"" << pluginName.toStdString() << "\"" << endl;

        if (pluginNameMenus[type].find(pluginName) ==
            pluginNameMenus[type].end()) {

            QMenu *parentMenu = pluginNameToChunkMenuMap[type][pluginName];
            if (!parentMenu) parentMenu = byPluginNameMenus[type];

            if (output == "") {
                parentMenu->addAction(action);
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

    setupRecentTransformsMenu();

    menu = m_paneMenu;

    action = new QAction(QIcon(":/icons/pane.png"), tr("Add &New Pane"), this);
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

    int channels = 1;
    if (getMainModel()) channels = getMainModel()->getChannelCount();

    if (channels < 1) channels = 1;

    LayerFactory::LayerType backgroundTypes[] = {
	LayerFactory::Waveform,
	LayerFactory::Spectrogram,
	LayerFactory::MelodicRangeSpectrogram,
	LayerFactory::PeakFrequencySpectrogram,
        LayerFactory::Spectrum
    };

    for (unsigned int i = 0;
	 i < sizeof(backgroundTypes)/sizeof(backgroundTypes[0]); ++i) {

	for (int menuType = 0; menuType <= 1; ++menuType) { // pane, layer

	    if (menuType == 0) menu = m_paneMenu;
	    else menu = m_layerMenu;

	    QMenu *submenu = 0;

	    for (int c = 0; c <= channels; ++c) {

		if (c == 1 && channels == 1) continue;
		bool isDefault = (c == 0);
		bool isOnly = (isDefault && (channels == 1));

		if (menuType == 1) {
		    if (isDefault) isOnly = true;
		    else continue;
		}

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
		    mainText = tr("Add &Spectrogram");
		    if (menuType == 0) {
			shortcutText = tr("Alt+S");
			tipText = tr("Add a new pane showing a dB spectrogram");
		    } else {
			tipText = tr("Add a new layer showing a dB spectrogram");
		    }
		    break;
		
		case LayerFactory::MelodicRangeSpectrogram:
		    mainText = tr("Add &Melodic Range Spectrogram");
		    if (menuType == 0) {
			shortcutText = tr("Alt+M");
			tipText = tr("Add a new pane showing a spectrogram set up for a pitch overview");
		    } else {
			tipText = tr("Add a new layer showing a spectrogram set up for a pitch overview");
		    }
		    break;
		
		case LayerFactory::PeakFrequencySpectrogram:
		    mainText = tr("Add &Peak Frequency Spectrogram");
		    if (menuType == 0) {
			shortcutText = tr("Alt+P");
			tipText = tr("Add a new pane showing a spectrogram set up for tracking frequencies");
		    } else {
			tipText = tr("Add a new layer showing a spectrogram set up for tracking frequencies");
		    }
		    break;

                case LayerFactory::Spectrum:
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

		if (isOnly) {

		    action = new QAction(icon, mainText, this);
		    action->setShortcut(shortcutText);
		    action->setStatusTip(tipText);
		    if (menuType == 0) {
			connect(action, SIGNAL(triggered()), this, SLOT(addPane()));
			connect(this, SIGNAL(canAddPane(bool)), action, SLOT(setEnabled(bool)));
			m_paneActions[action] = PaneConfiguration(type);
		    } else {
			connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
			connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
			m_layerActions[action] = type;
		    }
		    menu->addAction(action);

		} else {

		    QString actionText;
		    if (c == 0) 
			if (mono) actionText = tr("&All Channels Mixed");
			else actionText = tr("&All Channels");
		    else actionText = tr("Channel &%1").arg(c);

		    if (!submenu) {
			submenu = menu->addMenu(mainText);
		    }
		 
		    action = new QAction(icon, actionText, this);
		    if (isDefault) action->setShortcut(shortcutText);
		    action->setStatusTip(tipText);
		    if (menuType == 0) {
			connect(action, SIGNAL(triggered()), this, SLOT(addPane()));
			connect(this, SIGNAL(canAddPane(bool)), action, SLOT(setEnabled(bool)));
			m_paneActions[action] = PaneConfiguration(type, c - 1);
		    } else {
			connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
			connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
			m_layerActions[action] = type;
		    }
		    submenu->addAction(action);
		}
	    }
	}
    }

    menu = m_paneMenu;

    menu->addSeparator();

    action = new QAction(QIcon(":/icons/editdelete.png"), tr("&Delete Pane"), this);
    action->setShortcut(tr("Alt+D"));
    action->setStatusTip(tr("Delete the currently selected pane"));
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
    m_rightButtonLayerMenu->addMenu(m_existingLayersMenu);
    setupExistingLayersMenu();

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

    if (!m_mainMenusCreated) {
	
	menu = menuBar()->addMenu(tr("&Help"));

	action = new QAction(tr("&Help Reference"), this); 
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
/*
	action = new QAction(tr("About &Qt"), this);
	action->setStatusTip(tr("Show information about Qt"));
	connect(action, SIGNAL(triggered()),
		QApplication::getInstance(), SLOT(aboutQt()));
	menu->addAction(action);
*/
    }

    m_mainMenusCreated = true;
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
MainWindow::setupExistingLayersMenu()
{
    if (!m_existingLayersMenu) return; // should have been created by setupMenus

//    std::cerr << "MainWindow::setupExistingLayersMenu" << std::endl;

    m_existingLayersMenu->clear();
    m_existingLayerActions.clear();

    vector<Layer *> orderedLayers;
    set<Layer *> observedLayers;

    for (int i = 0; i < m_paneStack->getPaneCount(); ++i) {

	Pane *pane = m_paneStack->getPane(i);
	if (!pane) continue;

	for (int j = 0; j < pane->getLayerCount(); ++j) {

	    Layer *layer = pane->getLayer(j);
	    if (!layer) continue;
	    if (observedLayers.find(layer) != observedLayers.end()) {
		std::cerr << "found duplicate layer " << layer << std::endl;
		continue;
	    }

//	    std::cerr << "found new layer " << layer << " (name = " 
//		      << layer->getLayerPresentationName().toStdString() << ")" << std::endl;

	    orderedLayers.push_back(layer);
	    observedLayers.insert(layer);
	}
    }

    map<QString, int> observedNames;

    for (int i = 0; i < orderedLayers.size(); ++i) {
	
	QString name = orderedLayers[i]->getLayerPresentationName();
	int n = ++observedNames[name];
	if (n > 1) name = QString("%1 <%2>").arg(name).arg(n);

	QAction *action = new QAction(name, this);
	connect(action, SIGNAL(triggered()), this, SLOT(addLayer()));
	connect(this, SIGNAL(canAddLayer(bool)), action, SLOT(setEnabled(bool)));
	m_existingLayerActions[action] = orderedLayers[i];

	m_existingLayersMenu->addAction(action);
    }
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
    connect(action, SIGNAL(triggered()), this, SLOT(playSelectionToggled()));
    connect(this, SIGNAL(canPlaySelection(bool)), action, SLOT(setEnabled(bool)));

    action = toolbar->addAction(QIcon(":/icons/playloop.png"),
				tr("Loop Playback"));
    action->setCheckable(true);
    action->setChecked(m_viewManager->getPlayLoopMode());
    action->setShortcut(tr("l"));
    action->setStatusTip(tr("Loop playback"));
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
    connect(action, SIGNAL(triggered()), this, SLOT(toolNavigateSelected()));
    group->addAction(action);
    m_toolActions[ViewManager::NavigateMode] = action;

    action = toolbar->addAction(QIcon(":/icons/select.png"),
				tr("Select"));
    action->setCheckable(true);
    action->setShortcut(tr("2"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolSelectSelected()));
    group->addAction(action);
    m_toolActions[ViewManager::SelectMode] = action;

    action = toolbar->addAction(QIcon(":/icons/move.png"),
				tr("Edit"));
    action->setCheckable(true);
    action->setShortcut(tr("3"));
    connect(action, SIGNAL(triggered()), this, SLOT(toolEditSelected()));
    connect(this, SIGNAL(canEditLayer(bool)), action, SLOT(setEnabled(bool)));
    group->addAction(action);
    m_toolActions[ViewManager::EditMode] = action;

    action = toolbar->addAction(QIcon(":/icons/draw.png"),
				tr("Draw"));
    action->setCheckable(true);
    action->setShortcut(tr("4"));
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
    bool haveCurrentPane =
	(m_paneStack &&
	 (m_paneStack->getCurrentPane() != 0));
    bool haveCurrentLayer =
	(haveCurrentPane &&
	 (m_paneStack->getCurrentPane()->getSelectedLayer()));
    bool haveMainModel =
	(getMainModel() != 0);
    bool havePlayTarget =
	(m_playTarget != 0);
    bool haveSelection = 
	(m_viewManager &&
	 !m_viewManager->getSelections().empty());
    bool haveCurrentEditableLayer =
	(haveCurrentLayer &&
	 m_paneStack->getCurrentPane()->getSelectedLayer()->
	 isLayerEditable());
    bool haveCurrentTimeInstantsLayer = 
	(haveCurrentLayer &&
	 dynamic_cast<TimeInstantLayer *>
	 (m_paneStack->getCurrentPane()->getSelectedLayer()));
    bool haveCurrentTimeValueLayer = 
	(haveCurrentLayer &&
	 dynamic_cast<TimeValueLayer *>
	 (m_paneStack->getCurrentPane()->getSelectedLayer()));
    bool haveCurrentColour3DPlot =
        (haveCurrentLayer &&
         dynamic_cast<Colour3DPlotLayer *>
         (m_paneStack->getCurrentPane()->getSelectedLayer()));
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
    emit canDeleteCurrentLayer(haveCurrentLayer);
    emit canRenameLayer(haveCurrentLayer);
    emit canEditLayer(haveCurrentEditableLayer);
    emit canSelect(haveMainModel && haveCurrentPane);
    emit canPlay(/*!!! haveMainModel && */ havePlayTarget);
    emit canFfwd(haveCurrentTimeInstantsLayer || haveCurrentTimeValueLayer);
    emit canRewind(haveCurrentTimeInstantsLayer || haveCurrentTimeValueLayer);
    emit canPaste(haveCurrentEditableLayer && haveClipboardContents);
    emit canInsertInstant(haveCurrentPane);
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
MainWindow::currentPaneChanged(Pane *)
{
    updateMenuStates();
}

void
MainWindow::currentLayerChanged(Pane *, Layer *)
{
    updateMenuStates();
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
    QString orig = m_audioFile;

//    std::cerr << "orig = " << orig.toStdString() << std::endl;
    
    if (orig == "") orig = ".";

    QString path = QFileDialog::getOpenFileName
	(this, tr("Select an audio file"), orig,
	 tr("Audio files (%1)\nAll files (*.*)")
	 .arg(AudioFileReaderFactory::getKnownExtensions()));

    if (path != "") {
	if (!openAudioFile(path, ReplaceMainModel)) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("Audio file \"%1\" could not be opened").arg(path));
	}
    }
}

void
MainWindow::importMoreAudio()
{
    QString orig = m_audioFile;

//    std::cerr << "orig = " << orig.toStdString() << std::endl;
    
    if (orig == "") orig = ".";

    QString path = QFileDialog::getOpenFileName
	(this, tr("Select an audio file"), orig,
	 tr("Audio files (%1)\nAll files (*.*)")
	 .arg(AudioFileReaderFactory::getKnownExtensions()));

    if (path != "") {
	if (!openAudioFile(path, CreateAdditionalModel)) {
	    QMessageBox::critical(this, tr("Failed to open file"),
				  tr("Audio file \"%1\" could not be opened").arg(path));
	}
    }
}

void
MainWindow::exportAudio()
{
    if (!getMainModel()) return;

    QString path = QFileDialog::getSaveFileName
	(this, tr("Select a file to export to"), ".",
	 tr("WAV audio files (*.wav)\nAll files (*.*)"));
    
    if (path == "") return;

    if (!path.endsWith(".wav")) path = path + ".wav";

    bool ok = false;
    QString error;

    WavFileWriter *writer = 0;
    MultiSelection ms = m_viewManager->getSelection();
    MultiSelection::SelectionList selections = m_viewManager->getSelections();

    bool multiple = false;

    if (selections.empty()) {

	writer = new WavFileWriter(path, getMainModel()->getSampleRate(),
				   getMainModel(), 0);
    
    } else if (selections.size() == 1) {

	QStringList items;
	items << tr("Export the selected region only")
	      << tr("Export the whole audio file");
	
	bool ok = false;
	QString item = ListInputDialog::getItem
	    (this, tr("Select region to export"),
	     tr("Which region from the original audio file do you want to export?"),
	     items, 0, &ok);
	
	if (!ok || item.isEmpty()) return;
	
	if (item == items[0]) {

	    writer = new WavFileWriter(path, getMainModel()->getSampleRate(),
				       getMainModel(), &ms);

	} else {

	    writer = new WavFileWriter(path, getMainModel()->getSampleRate(),
				       getMainModel(), 0);
	}
    } else {

	QStringList items;
	items << tr("Export the selected regions into a single audio file")
	      << tr("Export the selected regions into separate files")
	      << tr("Export the whole audio file");

	bool ok = false;
	QString item = ListInputDialog::getItem
	    (this, tr("Select region to export"),
	     tr("Multiple regions of the original audio file are selected.\nWhat do you want to export?"),
	     items, 0, &ok);
	    
	if (!ok || item.isEmpty()) return;

	if (item == items[0]) {

	    writer = new WavFileWriter(path, getMainModel()->getSampleRate(),
				       getMainModel(), &ms);

	} else if (item == items[2]) {

	    writer = new WavFileWriter(path, getMainModel()->getSampleRate(),
				       getMainModel(), 0);

	} else {

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

		WavFileWriter subwriter(subpath, getMainModel()->getSampleRate(),
					getMainModel(), &subms);
		subwriter.write();
		ok = subwriter.isOK();

		if (!ok) {
		    error = subwriter.getError();
		    break;
		}
	    }
	}
    }

    if (writer) {
	writer->write();
	ok = writer->isOK();
	error = writer->getError();
	delete writer;
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

    QString path = QFileDialog::getOpenFileName
	(this, tr("Select file"), ".",
	 tr("All supported files (%1)\nSonic Visualiser Layer XML files (*.svl)\nComma-separated data files (*.csv)\nSpace-separated .lab files (*.lab)\nMIDI files (*.mid)\nText files (*.txt)\nAll files (*.*)").arg(DataFileReaderFactory::getKnownExtensions()));

    if (path != "") {

        if (!openLayerFile(path)) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("File %1 could not be opened.").arg(path));
            return;
        }
    }
}

bool
MainWindow::openLayerFile(QString path)
{
    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::openLayerFile: no current pane" << std::endl;
	return false;
    }

    if (!getMainModel()) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindow::openLayerFile: No main model -- hence no default sample rate available" << std::endl;
	return false;
    }

    if (path.endsWith(".svl") || path.endsWith(".xml")) {

        PaneCallback callback(this);
        QFile file(path);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << "ERROR: MainWindow::openLayerFile("
                      << path.toStdString()
                      << "): Failed to open file for reading" << std::endl;
            return false;
        }
        
        SVFileReader reader(m_document, callback);
        reader.setCurrentPane(pane);
        
        QXmlInputSource inputSource(&file);
        reader.parse(inputSource);
        
        if (!reader.isOK()) {
            std::cerr << "ERROR: MainWindow::openLayerFile("
                      << path.toStdString()
                      << "): Failed to read XML file: "
                      << reader.getErrorString().toStdString() << std::endl;
            return false;
        }

        m_recentFiles.addFile(path);
        return true;
        
    } else {
        
        Model *model = DataFileReaderFactory::load(path, getMainModel()->getSampleRate());
        
        if (model) {
            Layer *newLayer = m_document->createImportedLayer(model);
            if (newLayer) {
                m_document->addLayerToView(pane, newLayer);
                m_recentFiles.addFile(path);
                return true;
            }
        }
    }

    return false;
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

    QString path = QFileDialog::getSaveFileName
	(this, tr("Select a file to export to"), ".",
	 tr("Sonic Visualiser Layer XML files (*.svl)\nComma-separated data files (*.csv)\nText files (*.txt)\nAll files (*.*)"));

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

bool
MainWindow::openAudioFile(QString path, AudioFileOpenMode mode)
{
    if (!(QFileInfo(path).exists() &&
	  QFileInfo(path).isFile() &&
	  QFileInfo(path).isReadable())) {
	return false;
    }

    WaveFileModel *newModel = new WaveFileModel(path);

    if (!newModel->isOK()) {
	delete newModel;
	return false;
    }

    bool setAsMain = true;
    static bool prevSetAsMain = true;

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
                return false;
            }
            
            setAsMain = (item == items[0]);
            prevSetAsMain = setAsMain;
        }
    }

    if (setAsMain) {

        Model *prevMain = getMainModel();
        if (prevMain) m_playSource->removeModel(prevMain);

	PlayParameterRepository::getInstance()->clear();

	// The clear() call will have removed the parameters for the
	// main model.  Re-add them with the new one.
	PlayParameterRepository::getInstance()->addModel(newModel);

	m_document->setMainModel(newModel);
	setupMenus();

	if (m_sessionFile == "") {
	    setWindowTitle(tr("Sonic Visualiser: %1")
			   .arg(QFileInfo(path).fileName()));
	    CommandHistory::getInstance()->clear();
	    CommandHistory::getInstance()->documentSaved();
	    m_documentModified = false;
	} else {
	    setWindowTitle(tr("Sonic Visualiser: %1 [%2]")
			   .arg(QFileInfo(m_sessionFile).fileName())
			   .arg(QFileInfo(path).fileName()));
	    if (m_documentModified) {
		m_documentModified = false;
		documentModified(); // so as to restore "(modified)" window title
	    }
	}

	m_audioFile = path;

    } else { // !setAsMain

	CommandHistory::getInstance()->startCompoundOperation
	    (tr("Import \"%1\"").arg(QFileInfo(path).fileName()), true);

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
    m_recentFiles.addFile(path);

    return true;
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

void
MainWindow::newSession()
{
    if (!checkSaveModified()) return;

    closeSession();
    createDocument();

    Pane *pane = m_paneStack->addPane();

    if (!m_timeRulerLayer) {
	m_timeRulerLayer = m_document->createMainModelLayer
	    (LayerFactory::TimeRuler);
    }

    m_document->addLayerToView(pane, m_timeRulerLayer);

    Layer *waveform = m_document->createMainModelLayer(LayerFactory::Waveform);
    m_document->addLayerToView(pane, waveform);

    m_panner->registerView(pane);

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
    connect(m_document, SIGNAL(modelRegenerationFailed(QString)),
            this, SLOT(modelRegenerationFailed(QString)));
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

	m_panner->unregisterView(pane);
	m_paneStack->deletePane(pane);
    }

    while (m_paneStack->getHiddenPaneCount() > 0) {

	Pane *pane = m_paneStack->getHiddenPane
	    (m_paneStack->getHiddenPaneCount() - 1);

	while (pane->getLayerCount() > 0) {
	    m_document->removeLayerFromView
		(pane, pane->getLayer(pane->getLayerCount() - 1));
	}

	m_panner->unregisterView(pane);
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

    QString path = QFileDialog::getOpenFileName
	(this, tr("Select a session file"), orig,
	 tr("Sonic Visualiser session files (*.sv)\nAll files (*.*)"));

    if (path.isEmpty()) return;

    if (!(QFileInfo(path).exists() &&
	  QFileInfo(path).isFile() &&
	  QFileInfo(path).isReadable())) {
	QMessageBox::critical(this, tr("Failed to open file"),
			      tr("File \"%1\" does not exist or is not a readable file").arg(path));
	return;
    }

    if (!openSessionFile(path)) {
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

    QString importSpec;

    if (canImportLayer) {
        importSpec = tr("All supported files (*.sv %1 %2)\nSonic Visualiser session files (*.sv)\nAudio files (%1)\nLayer files (%2)\nAll files (*.*)")
            .arg(AudioFileReaderFactory::getKnownExtensions())
            .arg(DataFileReaderFactory::getKnownExtensions());
    } else {
        importSpec = tr("All supported files (*.sv %1)\nSonic Visualiser session files (*.sv)\nAudio files (%1)\nAll files (*.*)")
            .arg(AudioFileReaderFactory::getKnownExtensions());
    }

    QString path = QFileDialog::getOpenFileName
	(this, tr("Select a file to open"), orig, importSpec);

    if (path.isEmpty()) return;

    if (!(QFileInfo(path).exists() &&
	  QFileInfo(path).isFile() &&
	  QFileInfo(path).isReadable())) {
	QMessageBox::critical(this, tr("Failed to open file"),
			      tr("File \"%1\" does not exist or is not a readable file").arg(path));
	return;
    }

    if (path.endsWith(".sv")) {

        if (!checkSaveModified()) return;

        if (!openSessionFile(path)) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("Session file \"%1\" could not be opened").arg(path));
        }

    } else {

        if (!openAudioFile(path, AskUser)) {

            if (!canImportLayer || !openLayerFile(path)) {

                QMessageBox::critical(this, tr("Failed to open file"),
                                      tr("File \"%1\" could not be opened").arg(path));
            }
        }
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

    if (path.endsWith("sv")) {

        if (!checkSaveModified()) return ;

        if (!openSessionFile(path)) {
            QMessageBox::critical(this, tr("Failed to open file"),
                                  tr("Session file \"%1\" could not be opened").arg(path));
        }

    } else {

        if (!openAudioFile(path, AskUser)) {

            bool canImportLayer = (getMainModel() != 0 &&
                                   m_paneStack != 0 &&
                                   m_paneStack->getCurrentPane() != 0);

            if (!canImportLayer || !openLayerFile(path)) {

                QMessageBox::critical(this, tr("Failed to open file"),
                                      tr("File \"%1\" could not be opened").arg(path));
            }
        }
    }
}

bool
MainWindow::openSomeFile(QString path)
{
    if (openAudioFile(path)) {
	return true;
    } else if (openSessionFile(path)) {
	return true;
    } else {
	return false;
    }
}

bool
MainWindow::openSessionFile(QString path)
{
    BZipFileDevice bzFile(path);
    if (!bzFile.open(QIODevice::ReadOnly)) {
        std::cerr << "Failed to open session file \"" << path.toStdString()
                  << "\": " << bzFile.errorString().toStdString() << std::endl;
        return false;
    }

    QString error;
    closeSession();
    createDocument();

    PaneCallback callback(this);
    m_viewManager->clearSelections();

    SVFileReader reader(m_document, callback);
    QXmlInputSource inputSource(&bzFile);
    reader.parse(inputSource);
    
    if (!reader.isOK()) {
        error = tr("SV XML file read error:\n%1").arg(reader.getErrorString());
    }
    
    bzFile.close();

    bool ok = (error == "");
    
    if (ok) {
	setWindowTitle(tr("Sonic Visualiser: %1")
		       .arg(QFileInfo(path).fileName()));
	m_sessionFile = path;
	setupMenus();
	CommandHistory::getInstance()->clear();
	CommandHistory::getInstance()->documentSaved();
	m_documentModified = false;
	updateMenuStates();
        m_recentFiles.addFile(path);
    } else {
	setWindowTitle(tr("Sonic Visualiser"));
    }

    return ok;
}

void
MainWindow::closeEvent(QCloseEvent *e)
{
    if (!checkSaveModified()) {
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

        QString fname = QString("tmp-%1-%2.sv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
            .arg(QProcess().pid());
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

    QString path;
    bool good = false;

    while (!good) {

	path = QFileDialog::getSaveFileName
	    (this, tr("Select a file to save to"), orig,
	     tr("Sonic Visualiser session files (*.sv)\nAll files (*.*)"), 0,
	     QFileDialog::DontConfirmOverwrite); // we'll do that

	if (path.isEmpty()) return;

	if (!path.endsWith(".sv")) path = path + ".sv";

	QFileInfo fi(path);

	if (fi.isDir()) {
	    QMessageBox::critical(this, tr("Directory selected"),
				  tr("File \"%1\" is a directory").arg(path));
	    continue;
	}

	if (fi.exists()) {
	    if (QMessageBox::question(this, tr("File exists"),
				      tr("The file \"%1\" already exists.\nDo you want to overwrite it?").arg(path),
				      QMessageBox::Ok,
				      QMessageBox::Cancel) == QMessageBox::Ok) {
		good = true;
	    } else {
		continue;
	    }
	}

	good = true;
    }

    if (!saveSessionFile(path)) {
	QMessageBox::critical(this, tr("Failed to save file"),
			      tr("Session file \"%1\" could not be saved.").arg(m_sessionFile));
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

    if (bzFile.errorString() != "") {
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
MainWindow::showBasicOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::BasicOverlays);
}

void
MainWindow::showAllTextOverlays()
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
MainWindow::play()
{
    if (m_playSource->isPlaying()) {
	m_playSource->stop();
    } else {
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

    CommandHistory::getInstance()->startCompoundOperation
	(action->text(), true);

    AddPaneCommand *command = new AddPaneCommand(this);
    CommandHistory::getInstance()->addCommand(command);

    Pane *pane = command->getPane();

    if (i->second.layer == LayerFactory::Spectrum) {
        pane->setPlaybackFollow(View::PlaybackScrollContinuous);
    }

    if (i->second.layer != LayerFactory::TimeRuler &&
        i->second.layer != LayerFactory::Spectrum) {

	if (!m_timeRulerLayer) {
//	    std::cerr << "no time ruler layer, creating one" << std::endl;
	    m_timeRulerLayer = m_document->createMainModelLayer
		(LayerFactory::TimeRuler);
	}

//	std::cerr << "adding time ruler layer " << m_timeRulerLayer << std::endl;

	m_document->addLayerToView(pane, m_timeRulerLayer);
    }

    Layer *newLayer = m_document->createLayer(i->second.layer);
    m_document->setModel(newLayer, m_document->getMainModel());
    m_document->setChannel(newLayer, i->second.channel);
    m_document->addLayerToView(pane, newLayer);

    m_paneStack->setCurrentPane(pane);

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
    } else {
	m_mw->m_paneStack->showPane(m_pane);
    }

    m_mw->m_paneStack->setCurrentPane(m_pane);
    m_mw->m_panner->registerView(m_pane);
    m_added = true;
}

void
MainWindow::AddPaneCommand::unexecute()
{
    m_mw->m_paneStack->hidePane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_mw->m_panner->unregisterView(m_pane); 
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
    m_mw->m_panner->unregisterView(m_pane);
    m_added = false;
}

void
MainWindow::RemovePaneCommand::unexecute()
{
    m_mw->m_paneStack->showPane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_mw->m_panner->registerView(m_pane);
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

    TransformName transform = i->second;
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

    bool ok =
        factory->getConfigurationForTransform
        (transform, m_document->getMainModel(), context, configurationXml);
    if (!ok) return;

    Layer *newLayer = m_document->createDerivedLayer(transform,
                                                     m_document->getMainModel(),
                                                     context,
                                                     configurationXml);

    if (newLayer) {
        m_document->addLayerToView(pane, newLayer);
        m_document->setChannel(newLayer, context.channel);
        m_recentTransforms.add(transform);
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
		setupExistingLayersMenu();
	    }
	}
    }
}

void
MainWindow::playSpeedChanged(int speed)
{
    bool slow = false;
    bool something = false;
    float factor;

    if (speed < 100) {
        slow = true;
        speed = 100 - speed;
    } else {
        speed = speed - 100;
    }

    // speed is 0 -> 100

    if (speed == 0) {
        factor = 1.0;
    } else {
        factor = speed;
        factor = 1.0 + (factor * factor) / 1000.f;
        something = true;
    }

    int pc = lrintf((factor - 1.0) * 100);

    if (!slow) factor = 1.0 / factor;

    std::cerr << "speed = " << speed << " factor = " << factor << std::endl;

    m_playSpeed->setToolTip(tr("Playback speed: %1%2%")
                            .arg(!slow ? "+" : "-")
			    .arg(pc));

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
             tr("The sample rate of this audio file (%1 Hz) does not match\nthe current playback rate (%2 Hz).\n\nThe file will play at the wrong speed.")
             .arg(requested).arg(actual));
    }        

/*!!! Let's not do this for now, and see how we go -- now that we're putting
      sample rate information in the status bar

    QMessageBox::information
	(this, tr("Sample rate mismatch"),
	 tr("The sample rate of this audio file (%1 Hz) does not match\nthat of the output audio device (%2 Hz).\n\nThe file will be resampled automatically during playback.")
	 .arg(requested).arg(actual));
*/

    updateDescriptionLabel();
}

void
MainWindow::layerAdded(Layer *layer)
{
//    std::cerr << "MainWindow::layerAdded(" << layer << ")" << std::endl;
//    setupExistingLayersMenu();
    updateMenuStates();
}

void
MainWindow::layerRemoved(Layer *layer)
{
//    std::cerr << "MainWindow::layerRemoved(" << layer << ")" << std::endl;
    setupExistingLayersMenu();
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

    setupExistingLayersMenu();
    updateMenuStates();
}

void
MainWindow::modelAdded(Model *model)
{
//    std::cerr << "MainWindow::modelAdded(" << model << ")" << std::endl;
    m_playSource->addModel(model);
}

void
MainWindow::mainModelChanged(WaveFileModel *model)
{
//    std::cerr << "MainWindow::mainModelChanged(" << model << ")" << std::endl;
    updateDescriptionLabel();
    m_panLayer->setModel(model);
    if (model) m_viewManager->setMainModelSampleRate(model->getSampleRate());
    if (model && !m_playTarget) createPlayTarget();
}

void
MainWindow::modelAboutToBeDeleted(Model *model)
{
//    std::cerr << "MainWindow::modelAboutToBeDeleted(" << model << ")" << std::endl;
    m_playSource->removeModel(model);
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
MainWindow::showLayerTree()
{
    QTreeView *view = new QTreeView();
    LayerTreeModel *tree = new LayerTreeModel(m_paneStack);
    view->expand(tree->index(0, 0, QModelIndex()));
    view->setModel(tree);
    view->show();
}

void
MainWindow::preferenceChanged(PropertyContainer::PropertyName name)
{
    if (name == "Property Box Layout") {
        if (Preferences::getInstance()->getPropertyBoxLayout() ==
            Preferences::VerticallyStacked) {
            m_paneStack->setLayoutStyle(PaneStack::PropertyStackPerPaneLayout);
        } else {
            m_paneStack->setLayoutStyle(PaneStack::SinglePropertyStackLayout);
        }
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
MainWindow::website()
{
    openHelpUrl(tr("http://www.sonicvisualiser.org/"));
}

void
MainWindow::help()
{
    openHelpUrl(tr("http://www.sonicvisualiser.org/doc/reference/en/"));
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
    aboutText += tr("<p>Sonic Visualiser is a program for viewing and exploring audio data for semantic music analysis and annotation.</p>");
    aboutText += tr("<p>%1 : %2 build</p>")
        .arg(version)
        .arg(debug ? tr("Debug") : tr("Release"));

#ifdef BUILD_STATIC
    aboutText += tr("<p>Statically linked");
#ifndef QT_SHARED
    aboutText += tr("<br>With Qt (v%1) &copy; Trolltech AS").arg(QT_VERSION_STR);
#endif
#ifdef HAVE_JACK
    aboutText += tr("<br>With JACK audio output (v%1) &copy; Paul Davis and Jack O'Quin").arg(JACK_VERSION);
#endif
#ifdef HAVE_PORTAUDIO
    aboutText += tr("<br>With PortAudio audio output &copy; Ross Bencina and Phil Burk");
#endif
#ifdef HAVE_OGGZ
    aboutText += tr("<br>With Ogg file decoder (oggz v%1, fishsound v%2) &copy; CSIRO Australia").arg(OGGZ_VERSION).arg(FISHSOUND_VERSION);
#endif
#ifdef HAVE_MAD
    aboutText += tr("<br>With MAD mp3 decoder (v%1) &copy; Underbit Technologies Inc").arg(MAD_VERSION);
#endif
#ifdef HAVE_SAMPLERATE
    aboutText += tr("<br>With libsamplerate (v%1) &copy; Erik de Castro Lopo").arg(SAMPLERATE_VERSION);
#endif
#ifdef HAVE_SNDFILE
    aboutText += tr("<br>With libsndfile (v%1) &copy; Erik de Castro Lopo").arg(SNDFILE_VERSION);
#endif
#ifdef HAVE_FFTW3
    aboutText += tr("<br>With FFTW3 (v%1) &copy; Matteo Frigo and MIT").arg(FFTW3_VERSION);
#endif
#ifdef HAVE_VAMP
    aboutText += tr("<br>With Vamp plugin support (API v%1, SDK v%2) &copy; Chris Cannam").arg(VAMP_API_VERSION).arg(VAMP_SDK_VERSION);
#endif
    aboutText += tr("<br>With LADSPA plugin support (API v%1) &copy; Richard Furse, Paul Davis, Stefan Westerfeld").arg(LADSPA_VERSION);
    aboutText += tr("<br>With DSSI plugin support (API v%1) &copy; Chris Cannam, Steve Harris, Sean Bolton").arg(DSSI_VERSION);
    aboutText += "</p>";
#endif

    aboutText += 
        "<p>Sonic Visualiser Copyright &copy; 2005 - 2006 Chris Cannam<br>"
        "Centre for Digital Music, Queen Mary, University of London.</p>"
        "<p>This program is free software; you can redistribute it and/or<br>"
        "modify it under the terms of the GNU General Public License as<br>"
        "published by the Free Software Foundation; either version 2 of the<br>"
        "License, or (at your option) any later version.<br>See the file "
        "COPYING included with this distribution for more information.</p>";
    
    QMessageBox::about(this, tr("About Sonic Visualiser"), aboutText);
}


#ifdef INCLUDE_MOCFILES
#include "MainWindow.moc.cpp"
#endif
