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

#include "MainWindowBase.h"
#include "document/Document.h"


#include "view/Pane.h"
#include "view/PaneStack.h"
#include "data/model/WaveFileModel.h"
#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/NoteModel.h"
#include "data/model/Labeller.h"
#include "view/ViewManager.h"

#include "layer/WaveformLayer.h"
#include "layer/TimeRulerLayer.h"
#include "layer/TimeInstantLayer.h"
#include "layer/TimeValueLayer.h"
#include "layer/Colour3DPlotLayer.h"
#include "layer/SliceLayer.h"
#include "layer/SliceableLayer.h"
#include "layer/ImageLayer.h"

#include "widgets/ListInputDialog.h"

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

#include "base/PlayParameterRepository.h"
#include "base/XmlExportable.h"
#include "base/CommandHistory.h"
#include "base/Profiler.h"
#include "base/Preferences.h"

#include "osc/OSCQueue.h"

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


MainWindowBase::MainWindowBase(bool withAudioOutput, bool withOSCSupport) :
    m_document(0),
    m_paneStack(0),
    m_viewManager(0),
    m_timeRulerLayer(0),
    m_audioOutput(withAudioOutput),
    m_playSource(0),
    m_playTarget(0),
    m_oscQueue(withOSCSupport ? new OSCQueue() : 0),
    m_recentFiles("RecentFiles", 20),
    m_recentTransforms("RecentTransforms", 20),
    m_documentModified(false),
    m_openingAudioFile(false),
    m_abandoning(false),
    m_labeller(0)
{
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
	    this, SLOT(documentModified()));
    connect(CommandHistory::getInstance(), SIGNAL(documentRestored()),
	    this, SLOT(documentRestored()));
    
    m_viewManager = new ViewManager();
    connect(m_viewManager, SIGNAL(selectionChanged()),
	    this, SLOT(updateMenuStates()));
    connect(m_viewManager, SIGNAL(inProgressSelectionChanged()),
	    this, SLOT(inProgressSelectionChanged()));

    Preferences::BackgroundMode mode =
        Preferences::getInstance()->getBackgroundMode();
    m_initialDarkBackground = m_viewManager->getGlobalDarkBackground();
    if (mode != Preferences::BackgroundFromTheme) {
        m_viewManager->setGlobalDarkBackground
            (mode == Preferences::DarkBackground);
    }

    m_paneStack = new PaneStack(0, m_viewManager);
    connect(m_paneStack, SIGNAL(currentPaneChanged(Pane *)),
	    this, SLOT(currentPaneChanged(Pane *)));
    connect(m_paneStack, SIGNAL(currentLayerChanged(Pane *, Layer *)),
	    this, SLOT(currentLayerChanged(Pane *, Layer *)));
    connect(m_paneStack, SIGNAL(rightButtonMenuRequested(Pane *, QPoint)),
            this, SLOT(rightButtonMenuRequested(Pane *, QPoint)));
    connect(m_paneStack, SIGNAL(contextHelpChanged(const QString &)),
            this, SLOT(contextHelpChanged(const QString &)));
    connect(m_paneStack, SIGNAL(paneAdded(Pane *)),
            this, SLOT(paneAdded(Pane *)));
    connect(m_paneStack, SIGNAL(paneHidden(Pane *)),
            this, SLOT(paneHidden(Pane *)));
    connect(m_paneStack, SIGNAL(paneAboutToBeDeleted(Pane *)),
            this, SLOT(paneAboutToBeDeleted(Pane *)));
    connect(m_paneStack, SIGNAL(dropAccepted(Pane *, QStringList)),
            this, SLOT(paneDropAccepted(Pane *, QStringList)));
    connect(m_paneStack, SIGNAL(dropAccepted(Pane *, QString)),
            this, SLOT(paneDropAccepted(Pane *, QString)));

    m_playSource = new AudioCallbackPlaySource(m_viewManager);

    connect(m_playSource, SIGNAL(sampleRateMismatch(size_t, size_t, bool)),
	    this,           SLOT(sampleRateMismatch(size_t, size_t, bool)));
    connect(m_playSource, SIGNAL(audioOverloadPluginDisabled()),
            this,           SLOT(audioOverloadPluginDisabled()));

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

    if (m_oscQueue && m_oscQueue->isOK()) {
        connect(m_oscQueue, SIGNAL(messagesAvailable()), this, SLOT(pollOSC()));
        QTimer *oscTimer = new QTimer(this);
        connect(oscTimer, SIGNAL(timeout()), this, SLOT(pollOSC()));
        oscTimer->start(1000);
    }

    Labeller::ValueType labellerType = Labeller::ValueFromTwoLevelCounter;
    QSettings settings;
    settings.beginGroup("MainWindow");
    labellerType = (Labeller::ValueType)
        settings.value("labellertype", (int)labellerType).toInt();
    int cycle = settings.value("labellercycle", 4).toInt();
    settings.endGroup();

    m_labeller = new Labeller(labellerType);
    m_labeller->setCounterCycleSize(cycle);
}

MainWindowBase::~MainWindowBase()
{
    delete m_playTarget;
    delete m_playSource;
    delete m_viewManager;
    delete m_oscQueue;
    Profiles::getInstance()->dump();
}

QString
MainWindowBase::getOpenFileName(FileFinder::FileType type)
{
    FileFinder *ff = FileFinder::getInstance();
    switch (type) {
    case FileFinder::SessionFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::AudioFile:
        return ff->getOpenFileName(type, m_audioFile);
    case FileFinder::LayerFile:
        return ff->getOpenFileName(type, m_sessionFile);
    case FileFinder::LayerFileNoMidi:
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
MainWindowBase::getSaveFileName(FileFinder::FileType type)
{
    FileFinder *ff = FileFinder::getInstance();
    switch (type) {
    case FileFinder::SessionFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::AudioFile:
        return ff->getSaveFileName(type, m_audioFile);
    case FileFinder::LayerFile:
        return ff->getSaveFileName(type, m_sessionFile);
    case FileFinder::LayerFileNoMidi:
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
MainWindowBase::registerLastOpenedFilePath(FileFinder::FileType type, QString path)
{
    FileFinder *ff = FileFinder::getInstance();
    ff->registerLastOpenedFilePath(type, path);
}

void
MainWindowBase::updateMenuStates()
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
    emit canMeasureLayer(haveCurrentLayer);
    emit canSelect(haveMainModel && haveCurrentPane);
    emit canPlay(havePlayTarget);
    emit canFfwd(true);
    emit canRewind(true);
    emit canPaste(haveCurrentEditableLayer && haveClipboardContents);
    emit canInsertInstant(haveCurrentPane);
    emit canInsertInstantsAtBoundaries(haveCurrentPane && haveSelection);
    emit canRenumberInstants(haveCurrentTimeInstantsLayer && haveSelection);
    emit canPlaySelection(haveMainModel && havePlayTarget && haveSelection);
    emit canClearSelection(haveSelection);
    emit canEditSelection(haveSelection && haveCurrentEditableLayer);
    emit canSave(m_sessionFile != "" && m_documentModified);
}

void
MainWindowBase::documentModified()
{
//    std::cerr << "MainWindowBase::documentModified" << std::endl;

    if (!m_documentModified) {
        //!!! this in subclass implementation?
	setWindowTitle(tr("%1 (modified)").arg(windowTitle()));
    }

    m_documentModified = true;
    updateMenuStates();
}

void
MainWindowBase::documentRestored()
{
//    std::cerr << "MainWindowBase::documentRestored" << std::endl;

    if (m_documentModified) {
        //!!! this in subclass implementation?
	QString wt(windowTitle());
	wt.replace(tr(" (modified)"), "");
	setWindowTitle(wt);
    }

    m_documentModified = false;
    updateMenuStates();
}

void
MainWindowBase::playLoopToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (action) {
	m_viewManager->setPlayLoopMode(action->isChecked());
    } else {
	m_viewManager->setPlayLoopMode(!m_viewManager->getPlayLoopMode());
    }
}

void
MainWindowBase::playSelectionToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (action) {
	m_viewManager->setPlaySelectionMode(action->isChecked());
    } else {
	m_viewManager->setPlaySelectionMode(!m_viewManager->getPlaySelectionMode());
    }
}

void
MainWindowBase::playSoloToggled()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    
    if (action) {
	m_viewManager->setPlaySoloMode(action->isChecked());
    } else {
	m_viewManager->setPlaySoloMode(!m_viewManager->getPlaySoloMode());
    }

    if (m_viewManager->getPlaySoloMode()) {
        currentPaneChanged(m_paneStack->getCurrentPane());
    } else {
        m_viewManager->setPlaybackModel(0);
        if (m_playSource) {
            m_playSource->clearSoloModelSet();
        }
    }
}

void
MainWindowBase::currentPaneChanged(Pane *p)
{
    updateMenuStates();
    updateVisibleRangeDisplay(p);

    if (!p) return;

    if (!(m_viewManager &&
          m_playSource &&
          m_viewManager->getPlaySoloMode())) {
        if (m_viewManager) m_viewManager->setPlaybackModel(0);
        return;
    }

    Model *prevPlaybackModel = m_viewManager->getPlaybackModel();

    View::ModelSet soloModels = p->getModels();
    
    for (View::ModelSet::iterator mi = soloModels.begin();
         mi != soloModels.end(); ++mi) {
        if (dynamic_cast<RangeSummarisableTimeValueModel *>(*mi)) {
            m_viewManager->setPlaybackModel(*mi);
        }
    }
    
    RangeSummarisableTimeValueModel *a = 
        dynamic_cast<RangeSummarisableTimeValueModel *>(prevPlaybackModel);
    RangeSummarisableTimeValueModel *b = 
        dynamic_cast<RangeSummarisableTimeValueModel *>(m_viewManager->
                                                        getPlaybackModel());

    m_playSource->setSoloModelSet(soloModels);

    if (a && b && (a != b)) {
        int frame = m_playSource->getCurrentPlayingFrame();
        //!!! I don't really believe that these functions are the right way around
        int rframe = a->alignFromReference(frame);
        int bframe = b->alignToReference(rframe);
        if (m_playSource->isPlaying()) m_playSource->play(bframe);
    }
}

void
MainWindowBase::currentLayerChanged(Pane *p, Layer *)
{
    updateMenuStates();
    updateVisibleRangeDisplay(p);
}

void
MainWindowBase::selectAll()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(getMainModel()->getStartFrame(),
					  getMainModel()->getEndFrame()));
}

void
MainWindowBase::selectToStart()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(getMainModel()->getStartFrame(),
					  m_viewManager->getGlobalCentreFrame()));
}

void
MainWindowBase::selectToEnd()
{
    if (!getMainModel()) return;
    m_viewManager->setSelection(Selection(m_viewManager->getGlobalCentreFrame(),
					  getMainModel()->getEndFrame()));
}

void
MainWindowBase::selectVisible()
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
MainWindowBase::clearSelection()
{
    m_viewManager->clearSelections();
}

void
MainWindowBase::cut()
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
MainWindowBase::copy()
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
MainWindowBase::paste()
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
MainWindowBase::deleteSelected()
{
    if (m_paneStack->getCurrentPane() &&
	m_paneStack->getCurrentPane()->getSelectedLayer()) {
        
        Layer *layer = m_paneStack->getCurrentPane()->getSelectedLayer();

        if (m_viewManager && 
            (m_viewManager->getToolMode() == ViewManager::MeasureMode)) {

            layer->deleteCurrentMeasureRect();

        } else {

            MultiSelection::SelectionList selections =
                m_viewManager->getSelections();
            
            for (MultiSelection::SelectionList::iterator i = selections.begin();
                 i != selections.end(); ++i) {
                layer->deleteSelection(*i);
            }
	}
    }
}

void
MainWindowBase::insertInstant()
{
    int frame = m_viewManager->getPlaybackFrame();
    insertInstantAt(frame);
}

void
MainWindowBase::insertInstantsAtBoundaries()
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
MainWindowBase::insertInstantAt(size_t frame)
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
            SparseOneDimensionalModel::Point point(frame, "");

            SparseOneDimensionalModel::Point prevPoint(0);
            bool havePrevPoint = false;

            SparseOneDimensionalModel::EditCommand *command =
                new SparseOneDimensionalModel::EditCommand(sodm, tr("Add Point"));

            if (m_labeller->actingOnPrevPoint()) {

                SparseOneDimensionalModel::PointList prevPoints =
                    sodm->getPreviousPoints(frame);

                if (!prevPoints.empty()) {
                    prevPoint = *prevPoints.begin();
                    havePrevPoint = true;
                }
            }

            if (m_labeller) {

                m_labeller->setSampleRate(sodm->getSampleRate());

                if (havePrevPoint) {
                    command->deletePoint(prevPoint);
                }

                m_labeller->label<SparseOneDimensionalModel::Point>
                    (point, havePrevPoint ? &prevPoint : 0);

                if (havePrevPoint) {
                    command->addPoint(prevPoint);
                }
            }
            
            command->addPoint(point);

            command->setName(tr("Add Point at %1 s")
                             .arg(RealTime::frame2RealTime
                                  (frame,
                                   sodm->getSampleRate())
                                  .toText(false).c_str()));

            command->finish();
        }
    }
}

void
MainWindowBase::renumberInstants()
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return;

    Layer *layer = dynamic_cast<TimeInstantLayer *>(pane->getSelectedLayer());
    if (!layer) return;

    MultiSelection ms(m_viewManager->getSelection());
    
    Model *model = layer->getModel();
    SparseOneDimensionalModel *sodm = dynamic_cast<SparseOneDimensionalModel *>
        (model);
    if (!sodm) return;

    if (!m_labeller) return;

    Labeller labeller(*m_labeller);
    labeller.setSampleRate(sodm->getSampleRate());

    // This uses a command

    labeller.labelAll<SparseOneDimensionalModel::Point>(*sodm, &ms);
}

MainWindowBase::FileOpenStatus
MainWindowBase::open(QString fileOrUrl, AudioFileOpenMode mode)
{
    return open(FileSource(fileOrUrl), mode);
}

MainWindowBase::FileOpenStatus
MainWindowBase::open(FileSource source, AudioFileOpenMode mode)
{
    FileOpenStatus status;

    if (!source.isAvailable()) return FileOpenFailed;
    source.waitForData();

    bool canImportLayer = (getMainModel() != 0 &&
                           m_paneStack != 0 &&
                           m_paneStack->getCurrentPane() != 0);

    if ((status = openAudio(source, mode)) != FileOpenFailed) {
        return status;
    } else if ((status = openSession(source)) != FileOpenFailed) {
	return status;
    } else if ((status = openPlaylist(source, mode)) != FileOpenFailed) {
        return status;
    } else if (!canImportLayer) {
        return FileOpenWrongMode;
    } else if ((status = openImage(source)) != FileOpenFailed) {
        return status;
    } else if ((status = openLayer(source)) != FileOpenFailed) {
        return status;
    } else {
	return FileOpenFailed;
    }
}

MainWindowBase::FileOpenStatus
MainWindowBase::openAudio(FileSource source, AudioFileOpenMode mode)
{
    std::cerr << "MainWindowBase::openAudio(" << source.getLocation().toStdString() << ")" << std::endl;

    if (!source.isAvailable()) return FileOpenFailed;
    source.waitForData();

    m_openingAudioFile = true;

    size_t rate = 0;

    if (Preferences::getInstance()->getResampleOnLoad()) {
        rate = m_playSource->getSourceSampleRate();
    }

    WaveFileModel *newModel = new WaveFileModel(source, rate);

    if (!newModel->isOK()) {
	delete newModel;
        m_openingAudioFile = false;
	return FileOpenFailed;
    }

    std::cerr << "mode = " << mode << std::endl;

    if (mode == AskUser) {
        if (getMainModel()) {

            static bool prevSetAsMain = true;
            bool setAsMain = true;
            
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

            if (setAsMain) mode = ReplaceMainModel;
            else mode = CreateAdditionalModel;

        } else {
            mode = ReplaceMainModel;
        }
    }

    if (mode == ReplaceCurrentPane) {

        Pane *pane = m_paneStack->getCurrentPane();
        if (pane) {
            if (getMainModel()) {
                View::ModelSet models(pane->getModels());
                if (models.find(getMainModel()) != models.end()) {
                    mode = ReplaceMainModel;
                }
            } else {
                mode = ReplaceMainModel;
            }
        } else {
            mode = CreateAdditionalModel;
        }
    }

    if (mode == CreateAdditionalModel && !getMainModel()) {
        mode = ReplaceMainModel;
    }

    if (mode == ReplaceMainModel) {

        Model *prevMain = getMainModel();
        if (prevMain) {
            m_playSource->removeModel(prevMain);
            PlayParameterRepository::getInstance()->removeModel(prevMain);
        }
        PlayParameterRepository::getInstance()->addModel(newModel);

	m_document->setMainModel(newModel);

	setupMenus();

	if (m_sessionFile == "") {
            //!!! shouldn't be dealing directly with title from here -- call a method
	    setWindowTitle(tr("Sonic Visualiser: %1")
                           .arg(source.getLocation()));
	    CommandHistory::getInstance()->clear();
	    CommandHistory::getInstance()->documentSaved();
	    m_documentModified = false;
	} else {
	    setWindowTitle(tr("Sonic Visualiser: %1 [%2]")
			   .arg(QFileInfo(m_sessionFile).fileName())
			   .arg(source.getLocation()));
	    if (m_documentModified) {
		m_documentModified = false;
		documentModified(); // so as to restore "(modified)" window title
	    }
	}

        if (!source.isRemote()) m_audioFile = source.getLocalFilename();

    } else if (mode == CreateAdditionalModel) {

	CommandHistory::getInstance()->startCompoundOperation
	    (tr("Import \"%1\"").arg(source.getLocation()), true);

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

    } else if (mode == ReplaceCurrentPane) {

        // We know there is a current pane, otherwise we would have
        // reset the mode to CreateAdditionalModel above; and we know
        // the current pane does not contain the main model, otherwise
        // we would have reset it to ReplaceMainModel.  But we don't
        // know whether the pane contains a waveform model at all.
        
        Pane *pane = m_paneStack->getCurrentPane();
        Layer *replace = 0;

        for (int i = 0; i < pane->getLayerCount(); ++i) {
            Layer *layer = pane->getLayer(i);
            if (dynamic_cast<WaveformLayer *>(layer)) {
                replace = layer;
                break;
            }
        }

	CommandHistory::getInstance()->startCompoundOperation
	    (tr("Import \"%1\"").arg(source.getLocation()), true);

	m_document->addImportedModel(newModel);

        if (replace) {
            m_document->removeLayerFromView(pane, replace);
        }

	Layer *newLayer = m_document->createImportedLayer(newModel);

	if (newLayer) {
	    m_document->addLayerToView(pane, newLayer);
	}
	
	CommandHistory::getInstance()->endCompoundOperation();
    }

    updateMenuStates();
    m_recentFiles.addFile(source.getLocation());
    if (!source.isRemote()) {
        // for file dialog
        registerLastOpenedFilePath(FileFinder::AudioFile,
                                   source.getLocalFilename());
    }
    m_openingAudioFile = false;

    currentPaneChanged(m_paneStack->getCurrentPane());

    return FileOpenSucceeded;
}

MainWindowBase::FileOpenStatus
MainWindowBase::openPlaylist(FileSource source, AudioFileOpenMode mode)
{
    std::set<QString> extensions;
    PlaylistFileReader::getSupportedExtensions(extensions);
    QString extension = source.getExtension();
    if (extensions.find(extension) == extensions.end()) return FileOpenFailed;

    if (!source.isAvailable()) return FileOpenFailed;
    source.waitForData();

    PlaylistFileReader reader(source.getLocalFilename());
    if (!reader.isOK()) return FileOpenFailed;

    PlaylistFileReader::Playlist playlist = reader.load();

    bool someSuccess = false;

    for (PlaylistFileReader::Playlist::const_iterator i = playlist.begin();
         i != playlist.end(); ++i) {

        FileOpenStatus status = openAudio(*i, mode);

        if (status == FileOpenCancelled) {
            return FileOpenCancelled;
        }

        if (status == FileOpenSucceeded) {
            someSuccess = true;
            mode = CreateAdditionalModel;
        }
    }

    if (someSuccess) return FileOpenSucceeded;
    else return FileOpenFailed;
}

MainWindowBase::FileOpenStatus
MainWindowBase::openLayer(FileSource source)
{
    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindowBase::openLayer: no current pane" << std::endl;
	return FileOpenWrongMode;
    }

    if (!getMainModel()) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindowBase::openLayer: No main model -- hence no default sample rate available" << std::endl;
	return FileOpenWrongMode;
    }

    if (!source.isAvailable()) return FileOpenFailed;
    source.waitForData();

    QString path = source.getLocalFilename();

    if (source.getExtension() == "svl" || source.getExtension() == "xml") {

        PaneCallback callback(this);
        QFile file(path);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << "ERROR: MainWindowBase::openLayer("
                      << source.getLocation().toStdString()
                      << "): Failed to open file for reading" << std::endl;
            return FileOpenFailed;
        }
        
        SVFileReader reader(m_document, callback, source.getLocation());
        reader.setCurrentPane(pane);
        
        QXmlInputSource inputSource(&file);
        reader.parse(inputSource);
        
        if (!reader.isOK()) {
            std::cerr << "ERROR: MainWindowBase::openLayer("
                      << source.getLocation().toStdString()
                      << "): Failed to read XML file: "
                      << reader.getErrorString().toStdString() << std::endl;
            return FileOpenFailed;
        }

        m_recentFiles.addFile(source.getLocation());

        if (!source.isRemote()) {
            registerLastOpenedFilePath(FileFinder::LayerFile, path); // for file dialog
        }

    } else {
        
        try {

            Model *model = DataFileReaderFactory::load
                (path, getMainModel()->getSampleRate());
        
            if (model) {

                std::cerr << "MainWindowBase::openLayer: Have model" << std::endl;

                Layer *newLayer = m_document->createImportedLayer(model);

                if (newLayer) {

                    m_document->addLayerToView(pane, newLayer);
                    m_recentFiles.addFile(source.getLocation());
                    
                    if (!source.isRemote()) {
                        registerLastOpenedFilePath
                            (FileFinder::LayerFile,
                             path); // for file dialog
                    }
                    
                    return FileOpenSucceeded;
                }
            }
        } catch (DataFileReaderFactory::Exception e) {
            if (e == DataFileReaderFactory::ImportCancelled) {
                return FileOpenCancelled;
            }
        }
    }
    
    source.setLeaveLocalFile(true);
    return FileOpenFailed;
}

MainWindowBase::FileOpenStatus
MainWindowBase::openImage(FileSource source)
{
    Pane *pane = m_paneStack->getCurrentPane();
    
    if (!pane) {
	// shouldn't happen, as the menu action should have been disabled
	std::cerr << "WARNING: MainWindowBase::openImage: no current pane" << std::endl;
	return FileOpenWrongMode;
    }

    if (!m_document->getMainModel()) {
        return FileOpenWrongMode;
    }

    bool newLayer = false;

    ImageLayer *il = dynamic_cast<ImageLayer *>(pane->getSelectedLayer());
    if (!il) {
        for (int i = pane->getLayerCount()-1; i >= 0; --i) {
            il = dynamic_cast<ImageLayer *>(pane->getLayer(i));
            if (il) break;
        }
    }
    if (!il) {
        il = dynamic_cast<ImageLayer *>
            (m_document->createEmptyLayer(LayerFactory::Image));
        if (!il) return FileOpenFailed;
        newLayer = true;
    }

    // We don't put the image file in Recent Files

    std::cerr << "openImage: trying location \"" << source.getLocation().toStdString() << "\" in image layer" << std::endl;

    if (!il->addImage(m_viewManager->getGlobalCentreFrame(), source.getLocation())) {
        if (newLayer) {
            m_document->setModel(il, 0); // releasing its model
            delete il;
        }
        return FileOpenFailed;
    } else {
        if (newLayer) {
            m_document->addLayerToView(pane, il);
        }
        m_paneStack->setCurrentLayer(pane, il);
    }

    return FileOpenSucceeded;
}

MainWindowBase::FileOpenStatus
MainWindowBase::openSessionFile(QString fileOrUrl)
{
    return openSession(FileSource(fileOrUrl));
}

MainWindowBase::FileOpenStatus
MainWindowBase::openSession(FileSource source)
{
    if (!source.isAvailable()) return FileOpenFailed;
    if (source.getExtension() != "sv") return FileOpenFailed;
    source.waitForData();

    BZipFileDevice bzFile(source.getLocalFilename());
    if (!bzFile.open(QIODevice::ReadOnly)) return FileOpenFailed;

    if (!checkSaveModified()) return FileOpenCancelled;

    QString error;
    closeSession();
    createDocument();

    PaneCallback callback(this);
    m_viewManager->clearSelections();

    SVFileReader reader(m_document, callback, source.getLocation());
    QXmlInputSource inputSource(&bzFile);
    reader.parse(inputSource);
    
    if (!reader.isOK()) {
        error = tr("SV XML file read error:\n%1").arg(reader.getErrorString());
    }
    
    bzFile.close();

    bool ok = (error == "");

    if (ok) {

	setWindowTitle(tr("Sonic Visualiser: %1")
		       .arg(source.getLocation()));

	if (!source.isRemote()) m_sessionFile = source.getLocalFilename();

	setupMenus();

	CommandHistory::getInstance()->clear();
	CommandHistory::getInstance()->documentSaved();
	m_documentModified = false;
	updateMenuStates();

        m_recentFiles.addFile(source.getLocation());

        if (!source.isRemote()) {
            // for file dialog
            registerLastOpenedFilePath(FileFinder::SessionFile,
                                        source.getLocalFilename());
        }

    } else {
	setWindowTitle(tr("Sonic Visualiser"));
    }

    return ok ? FileOpenSucceeded : FileOpenFailed;
}

void
MainWindowBase::createPlayTarget()
{
    if (m_playTarget) return;

    m_playTarget = AudioTargetFactory::createCallbackTarget(m_playSource);
    if (!m_playTarget) {
	QMessageBox::warning
	    (this, tr("Couldn't open audio device"),
	     tr("<b>No audio available</b><p>Could not open an audio device for playback.<p>Audio playback will not be available during this session."),
	     QMessageBox::Ok);
    }
}

WaveFileModel *
MainWindowBase::getMainModel()
{
    if (!m_document) return 0;
    return m_document->getMainModel();
}

const WaveFileModel *
MainWindowBase::getMainModel() const
{
    if (!m_document) return 0;
    return m_document->getMainModel();
}

void
MainWindowBase::createDocument()
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

bool
MainWindowBase::saveSessionFile(QString path)
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
			      tr("<b>Save failed</b><p>Failed to write to file \"%1\": %2")
			      .arg(path).arg(bzFile.errorString()));
        bzFile.close();
	return false;
    }

    bzFile.close();
    return true;
}

void
MainWindowBase::toXml(QTextStream &out)
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
MainWindowBase::addPaneToStack()
{
    AddPaneCommand *command = new AddPaneCommand(this);
    CommandHistory::getInstance()->addCommand(command);
    return command->getPane();
}

void
MainWindowBase::zoomIn()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->zoom(true);
}

void
MainWindowBase::zoomOut()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->zoom(false);
}

void
MainWindowBase::zoomToFit()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (!currentPane) return;

    Model *model = getMainModel();
    if (!model) return;
    
    size_t start = model->getStartFrame();
    size_t end = model->getEndFrame();
    size_t pixels = currentPane->width();

    size_t sw = currentPane->getVerticalScaleWidth();
    if (pixels > sw * 2) pixels -= sw * 2;
    else pixels = 1;
    if (pixels > 4) pixels -= 4;

    size_t zoomLevel = (end - start) / pixels;

    currentPane->setZoomLevel(zoomLevel);
    currentPane->setCentreFrame((start + end) / 2);
}

void
MainWindowBase::zoomDefault()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->setZoomLevel(1024);
}

void
MainWindowBase::scrollLeft()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(false, false);
}

void
MainWindowBase::jumpLeft()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(false, true);
}

void
MainWindowBase::scrollRight()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(true, false);
}

void
MainWindowBase::jumpRight()
{
    Pane *currentPane = m_paneStack->getCurrentPane();
    if (currentPane) currentPane->scroll(true, true);
}

void
MainWindowBase::showNoOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::NoOverlays);
}

void
MainWindowBase::showMinimalOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::MinimalOverlays);
}

void
MainWindowBase::showStandardOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::StandardOverlays);
}

void
MainWindowBase::showAllOverlays()
{
    m_viewManager->setOverlayMode(ViewManager::AllOverlays);
}

void
MainWindowBase::toggleZoomWheels()
{
    if (m_viewManager->getZoomWheelsEnabled()) {
        m_viewManager->setZoomWheelsEnabled(false);
    } else {
        m_viewManager->setZoomWheelsEnabled(true);
    }
}

void
MainWindowBase::togglePropertyBoxes()
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
MainWindowBase::toggleStatusBar()
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
MainWindowBase::preferenceChanged(PropertyContainer::PropertyName name)
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
    } else if (name == "Background Mode" && m_viewManager) {
        Preferences::BackgroundMode mode =
            Preferences::getInstance()->getBackgroundMode();
        if (mode == Preferences::BackgroundFromTheme) {
            m_viewManager->setGlobalDarkBackground(m_initialDarkBackground);
        } else if (mode == Preferences::DarkBackground) {
            m_viewManager->setGlobalDarkBackground(true);
        } else {
            m_viewManager->setGlobalDarkBackground(false);
        }
    }            
}

void
MainWindowBase::play()
{
    if (m_playSource->isPlaying()) {
        stop();
    } else {
        playbackFrameChanged(m_viewManager->getPlaybackFrame());
	m_playSource->play(m_viewManager->getPlaybackFrame());
    }
}

void
MainWindowBase::ffwd()
{
    if (!getMainModel()) return;

    int frame = m_viewManager->getPlaybackFrame();
    ++frame;

    Layer *layer = getSnapLayer();
    size_t sr = getMainModel()->getSampleRate();

    if (!layer) {

        frame = RealTime::realTime2Frame
            (RealTime::frame2RealTime(frame, sr) + RealTime(2, 0), sr);
        if (frame > int(getMainModel()->getEndFrame())) {
            frame = getMainModel()->getEndFrame();
        }

    } else {

        size_t resolution = 0;
        if (!layer->snapToFeatureFrame(m_paneStack->getCurrentPane(),
                                       frame, resolution, Layer::SnapRight)) {
            frame = getMainModel()->getEndFrame();
        }
    }
        
    if (frame < 0) frame = 0;

    if (m_viewManager->getPlaySelectionMode()) {
        frame = m_viewManager->constrainFrameToSelection(size_t(frame));
    }
    
    m_viewManager->setPlaybackFrame(frame);
}

void
MainWindowBase::ffwdEnd()
{
    if (!getMainModel()) return;

    size_t frame = getMainModel()->getEndFrame();

    if (m_viewManager->getPlaySelectionMode()) {
        frame = m_viewManager->constrainFrameToSelection(frame);
    }

    m_viewManager->setPlaybackFrame(frame);
}

void
MainWindowBase::rewind()
{
    if (!getMainModel()) return;

    int frame = m_viewManager->getPlaybackFrame();
    if (frame > 0) --frame;

    Layer *layer = getSnapLayer();
    size_t sr = getMainModel()->getSampleRate();
    
    // when rewinding during playback, we want to allow a period
    // following a rewind target point at which the rewind will go to
    // the prior point instead of the immediately neighbouring one
    if (m_playSource && m_playSource->isPlaying()) {
        RealTime ct = RealTime::frame2RealTime(frame, sr);
        ct = ct - RealTime::fromSeconds(0.25);
        if (ct < RealTime::zeroTime) ct = RealTime::zeroTime;
//        std::cerr << "rewind: frame " << frame << " -> ";
        frame = RealTime::realTime2Frame(ct, sr);
//        std::cerr << frame << std::endl;
    }

    if (!layer) {
        
        frame = RealTime::realTime2Frame
            (RealTime::frame2RealTime(frame, sr) - RealTime(2, 0), sr);
        if (frame < int(getMainModel()->getStartFrame())) {
            frame = getMainModel()->getStartFrame();
        }

    } else {

        size_t resolution = 0;
        if (!layer->snapToFeatureFrame(m_paneStack->getCurrentPane(),
                                       frame, resolution, Layer::SnapLeft)) {
            frame = getMainModel()->getStartFrame();
        }
    }

    if (frame < 0) frame = 0;

    if (m_viewManager->getPlaySelectionMode()) {
        frame = m_viewManager->constrainFrameToSelection(size_t(frame));
    }

    m_viewManager->setPlaybackFrame(frame);
}

void
MainWindowBase::rewindStart()
{
    if (!getMainModel()) return;

    size_t frame = getMainModel()->getStartFrame();

    if (m_viewManager->getPlaySelectionMode()) {
        frame = m_viewManager->constrainFrameToSelection(frame);
    }

    m_viewManager->setPlaybackFrame(frame);
}

Layer *
MainWindowBase::getSnapLayer() const
{
    Pane *pane = m_paneStack->getCurrentPane();
    if (!pane) return 0;

    Layer *layer = pane->getSelectedLayer();

    if (!dynamic_cast<TimeInstantLayer *>(layer) &&
        !dynamic_cast<TimeValueLayer *>(layer) &&
        !dynamic_cast<TimeRulerLayer *>(layer)) {

        layer = 0;

        for (int i = pane->getLayerCount(); i > 0; --i) {
            Layer *l = pane->getLayer(i-1);
            if (dynamic_cast<TimeRulerLayer *>(l)) {
                layer = l;
                break;
            }
        }
    }

    return layer;
}

void
MainWindowBase::stop()
{
    m_playSource->stop();

    if (m_paneStack && m_paneStack->getCurrentPane()) {
        updateVisibleRangeDisplay(m_paneStack->getCurrentPane());
    } else {
        m_myStatusMessage = "";
        statusBar()->showMessage("");
    }
}

MainWindowBase::AddPaneCommand::AddPaneCommand(MainWindowBase *mw) :
    m_mw(mw),
    m_pane(0),
    m_prevCurrentPane(0),
    m_added(false)
{
}

MainWindowBase::AddPaneCommand::~AddPaneCommand()
{
    if (m_pane && !m_added) {
	m_mw->m_paneStack->deletePane(m_pane);
    }
}

QString
MainWindowBase::AddPaneCommand::getName() const
{
    return tr("Add Pane");
}

void
MainWindowBase::AddPaneCommand::execute()
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
    m_added = true;
}

void
MainWindowBase::AddPaneCommand::unexecute()
{
    m_mw->m_paneStack->hidePane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_added = false;
}

MainWindowBase::RemovePaneCommand::RemovePaneCommand(MainWindowBase *mw, Pane *pane) :
    m_mw(mw),
    m_pane(pane),
    m_added(true)
{
}

MainWindowBase::RemovePaneCommand::~RemovePaneCommand()
{
    if (m_pane && !m_added) {
	m_mw->m_paneStack->deletePane(m_pane);
    }
}

QString
MainWindowBase::RemovePaneCommand::getName() const
{
    return tr("Remove Pane");
}

void
MainWindowBase::RemovePaneCommand::execute()
{
    m_prevCurrentPane = m_mw->m_paneStack->getCurrentPane();
    m_mw->m_paneStack->hidePane(m_pane);
    m_added = false;
}

void
MainWindowBase::RemovePaneCommand::unexecute()
{
    m_mw->m_paneStack->showPane(m_pane);
    m_mw->m_paneStack->setCurrentPane(m_prevCurrentPane);
    m_added = true;
}

void
MainWindowBase::deleteCurrentPane()
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
MainWindowBase::deleteCurrentLayer()
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
MainWindowBase::playbackFrameChanged(unsigned long frame)
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
MainWindowBase::globalCentreFrameChanged(unsigned long )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (!p->getFollowGlobalPan()) return;
    updateVisibleRangeDisplay(p);
}

void
MainWindowBase::viewCentreFrameChanged(View *v, unsigned long )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (v == p) updateVisibleRangeDisplay(p);
}

void
MainWindowBase::viewZoomLevelChanged(View *v, unsigned long , bool )
{
    if ((m_playSource && m_playSource->isPlaying()) || !getMainModel()) return;
    Pane *p = 0;
    if (!m_paneStack || !(p = m_paneStack->getCurrentPane())) return;
    if (v == p) updateVisibleRangeDisplay(p);
}

void
MainWindowBase::layerAdded(Layer *)
{
//    std::cerr << "MainWindowBase::layerAdded(" << layer << ")" << std::endl;
    updateMenuStates();
}

void
MainWindowBase::layerRemoved(Layer *)
{
//    std::cerr << "MainWindowBase::layerRemoved(" << layer << ")" << std::endl;
    updateMenuStates();
}

void
MainWindowBase::layerAboutToBeDeleted(Layer *layer)
{
//    std::cerr << "MainWindowBase::layerAboutToBeDeleted(" << layer << ")" << std::endl;
    if (layer == m_timeRulerLayer) {
//	std::cerr << "(this is the time ruler layer)" << std::endl;
	m_timeRulerLayer = 0;
    }
}

void
MainWindowBase::layerInAView(Layer *layer, bool inAView)
{
//    std::cerr << "MainWindowBase::layerInAView(" << layer << "," << inAView << ")" << std::endl;

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

    updateMenuStates();
}

void
MainWindowBase::modelAdded(Model *model)
{
//    std::cerr << "MainWindowBase::modelAdded(" << model << ")" << std::endl;
    m_playSource->addModel(model);
}

void
MainWindowBase::mainModelChanged(WaveFileModel *model)
{
//    std::cerr << "MainWindowBase::mainModelChanged(" << model << ")" << std::endl;
    updateDescriptionLabel();
    if (model) m_viewManager->setMainModelSampleRate(model->getSampleRate());
    if (model && !m_playTarget && m_audioOutput) createPlayTarget();
}

void
MainWindowBase::modelAboutToBeDeleted(Model *model)
{
//    std::cerr << "MainWindowBase::modelAboutToBeDeleted(" << model << ")" << std::endl;
    if (model == m_viewManager->getPlaybackModel()) {
        m_viewManager->setPlaybackModel(0);
    }
    m_playSource->removeModel(model);
    FFTDataServer::modelAboutToBeDeleted(model);
}

void
MainWindowBase::pollOSC()
{
    if (!m_oscQueue || m_oscQueue->isEmpty()) return;
    std::cerr << "MainWindowBase::pollOSC: have " << m_oscQueue->getMessagesAvailable() << " messages" << std::endl;

    if (m_openingAudioFile) return;

    OSCMessage message = m_oscQueue->readMessage();

    if (message.getTarget() != 0) {
        return; //!!! for now -- this class is target 0, others not handled yet
    }

    handleOSCMessage(message);
}

void
MainWindowBase::inProgressSelectionChanged()
{
    Pane *currentPane = 0;
    if (m_paneStack) currentPane = m_paneStack->getCurrentPane();
    if (currentPane) updateVisibleRangeDisplay(currentPane);
}

void
MainWindowBase::contextHelpChanged(const QString &s)
{
    if (s == "" && m_myStatusMessage != "") {
        statusBar()->showMessage(m_myStatusMessage);
        return;
    }
    statusBar()->showMessage(s);
}

void
MainWindowBase::openHelpUrl(QString url)
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

