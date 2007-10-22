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

#ifndef _MAIN_WINDOW_BASE_H_
#define _MAIN_WINDOW_BASE_H_

#include <QFrame>
#include <QString>
#include <QUrl>
#include <QMainWindow>
#include <QPointer>

#include "base/Command.h"
#include "view/ViewManager.h"
#include "base/PropertyContainer.h"
#include "base/RecentFiles.h"
#include "layer/LayerFactory.h"
#include "transform/Transform.h"
#include "document/SVFileReader.h"
#include "data/fileio/FileFinder.h"
#include "data/fileio/FileSource.h"
#include <map>

class Document;
class PaneStack;
class Pane;
class View;
class Fader;
class Overview;
class Layer;
class WaveformLayer;
class WaveFileModel;
class AudioCallbackPlaySource;
class AudioCallbackPlayTarget;
class CommandHistory;
class QMenu;
class AudioDial;
class QLabel;
class QCheckBox;
class PreferencesDialog;
class QTreeView;
class QPushButton;
class OSCQueue;
class OSCMessage;
class KeyReference;
class Labeller;

/**
 * The base class for the SV main window.  This includes everything to
 * do with general document and pane stack management, but nothing
 * that involves user interaction -- this doesn't create the widget or
 * menu structures or editing tools, and if a function needs to open a
 * dialog, it shouldn't be in here.  This permits "variations on SV"
 * to use different subclasses retaining the same general structure.
 */

class MainWindowBase : public QMainWindow
{
    Q_OBJECT

public:
    MainWindowBase(bool withAudioOutput, bool withOSCSupport);
    virtual ~MainWindowBase();
    
    enum AudioFileOpenMode {
        ReplaceMainModel,
        CreateAdditionalModel,
        ReplaceCurrentPane,
        AskUser
    };

    enum FileOpenStatus {
        FileOpenSucceeded,
        FileOpenFailed,
        FileOpenCancelled,
        FileOpenWrongMode // attempted to open layer when no main model present
    };

    virtual FileOpenStatus open(QString fileOrUrl, AudioFileOpenMode = AskUser);
    virtual FileOpenStatus open(FileSource source, AudioFileOpenMode = AskUser);
    
    virtual FileOpenStatus openAudio(FileSource source, AudioFileOpenMode = AskUser);
    virtual FileOpenStatus openPlaylist(FileSource source, AudioFileOpenMode = AskUser);
    virtual FileOpenStatus openLayer(FileSource source);
    virtual FileOpenStatus openImage(FileSource source);

    virtual FileOpenStatus openSessionFile(QString fileOrUrl);
    virtual FileOpenStatus openSession(FileSource source);

    virtual bool saveSessionFile(QString path);

signals:
    // Used to toggle the availability of menu actions
    void canAddPane(bool);
    void canDeleteCurrentPane(bool);
    void canAddLayer(bool);
    void canImportMoreAudio(bool);
    void canImportLayer(bool);
    void canExportAudio(bool);
    void canExportLayer(bool);
    void canExportImage(bool);
    void canRenameLayer(bool);
    void canEditLayer(bool);
    void canMeasureLayer(bool);
    void canSelect(bool);
    void canClearSelection(bool);
    void canEditSelection(bool);
    void canDeleteSelection(bool);
    void canPaste(bool);
    void canInsertInstant(bool);
    void canInsertInstantsAtBoundaries(bool);
    void canRenumberInstants(bool);
    void canDeleteCurrentLayer(bool);
    void canZoom(bool);
    void canScroll(bool);
    void canPlay(bool);
    void canFfwd(bool);
    void canRewind(bool);
    void canPlaySelection(bool);
    void canSpeedUpPlayback(bool);
    void canSlowDownPlayback(bool);
    void canChangePlaybackSpeed(bool);
    void canSave(bool);

public slots:
    virtual void preferenceChanged(PropertyContainer::PropertyName);

protected slots:
    virtual void zoomIn();
    virtual void zoomOut();
    virtual void zoomToFit();
    virtual void zoomDefault();
    virtual void scrollLeft();
    virtual void scrollRight();
    virtual void jumpLeft();
    virtual void jumpRight();

    virtual void showNoOverlays();
    virtual void showMinimalOverlays();
    virtual void showStandardOverlays();
    virtual void showAllOverlays();

    virtual void toggleZoomWheels();
    virtual void togglePropertyBoxes();
    virtual void toggleStatusBar();

    virtual void play();
    virtual void ffwd();
    virtual void ffwdEnd();
    virtual void rewind();
    virtual void rewindStart();
    virtual void stop();

    virtual void deleteCurrentPane();
    virtual void deleteCurrentLayer();

    virtual void playLoopToggled();
    virtual void playSelectionToggled();
    virtual void playSoloToggled();

    virtual void sampleRateMismatch(size_t, size_t, bool) = 0;
    virtual void audioOverloadPluginDisabled() = 0;

    virtual void playbackFrameChanged(unsigned long);
    virtual void globalCentreFrameChanged(unsigned long);
    virtual void viewCentreFrameChanged(View *, unsigned long);
    virtual void viewZoomLevelChanged(View *, unsigned long, bool);
    virtual void outputLevelsChanged(float, float) = 0;

    virtual void currentPaneChanged(Pane *);
    virtual void currentLayerChanged(Pane *, Layer *);

    virtual void selectAll();
    virtual void selectToStart();
    virtual void selectToEnd();
    virtual void selectVisible();
    virtual void clearSelection();

    virtual void cut();
    virtual void copy();
    virtual void paste();
    virtual void deleteSelected();

    virtual void insertInstant();
    virtual void insertInstantAt(size_t);
    virtual void insertInstantsAtBoundaries();
    virtual void renumberInstants();

    virtual void documentModified();
    virtual void documentRestored();

    virtual void layerAdded(Layer *);
    virtual void layerRemoved(Layer *);
    virtual void layerAboutToBeDeleted(Layer *);
    virtual void layerInAView(Layer *, bool);

    virtual void mainModelChanged(WaveFileModel *);
    virtual void modelAdded(Model *);
    virtual void modelAboutToBeDeleted(Model *);

    virtual void updateMenuStates();
    virtual void updateDescriptionLabel() = 0;

    virtual void modelGenerationFailed(QString) = 0;
    virtual void modelRegenerationFailed(QString, QString) = 0;

    virtual void rightButtonMenuRequested(Pane *, QPoint point) = 0;

    virtual void paneAdded(Pane *) = 0;
    virtual void paneHidden(Pane *) = 0;
    virtual void paneAboutToBeDeleted(Pane *) = 0;
    virtual void paneDropAccepted(Pane *, QStringList) = 0;
    virtual void paneDropAccepted(Pane *, QString) = 0;

    virtual void pollOSC();
    virtual void handleOSCMessage(const OSCMessage &) = 0;

    virtual void contextHelpChanged(const QString &);
    virtual void inProgressSelectionChanged();

    virtual void closeSession() = 0;

protected:
    QString                  m_sessionFile;
    QString                  m_audioFile;
    Document                *m_document;

    QLabel                  *m_descriptionLabel;
    PaneStack               *m_paneStack;
    ViewManager             *m_viewManager;
    Layer                   *m_timeRulerLayer;

    bool                     m_audioOutput;
    AudioCallbackPlaySource *m_playSource;
    AudioCallbackPlayTarget *m_playTarget;

    OSCQueue                *m_oscQueue;

    RecentFiles              m_recentFiles;
    RecentFiles              m_recentTransforms;

    bool                     m_documentModified;
    bool                     m_openingAudioFile;
    bool                     m_abandoning;

    Labeller                *m_labeller;

    int                      m_lastPlayStatusSec;
    mutable QString          m_myStatusMessage;

    bool                     m_initialDarkBackground;

    WaveFileModel *getMainModel();
    const WaveFileModel *getMainModel() const;
    void createDocument();

    Pane *addPaneToStack();
    Layer *getSnapLayer() const;

    class PaneCallback : public SVFileReaderPaneCallback
    {
    public:
	PaneCallback(MainWindowBase *mw) : m_mw(mw) { }
	virtual Pane *addPane() { return m_mw->addPaneToStack(); }
	virtual void setWindowSize(int width, int height) {
	    m_mw->resize(width, height);
	}
	virtual void addSelection(int start, int end) {
	    m_mw->m_viewManager->addSelection(Selection(start, end));
	}
    protected:
	MainWindowBase *m_mw;
    };

    class AddPaneCommand : public Command
    {
    public:
	AddPaneCommand(MainWindowBase *mw);
	virtual ~AddPaneCommand();
	
	virtual void execute();
	virtual void unexecute();
	virtual QString getName() const;

	Pane *getPane() { return m_pane; }

    protected:
	MainWindowBase *m_mw;
	Pane *m_pane; // Main window owns this, but I determine its lifespan
	Pane *m_prevCurrentPane; // I don't own this
	bool m_added;
    };

    class RemovePaneCommand : public Command
    {
    public:
	RemovePaneCommand(MainWindowBase *mw, Pane *pane);
	virtual ~RemovePaneCommand();
	
	virtual void execute();
	virtual void unexecute();
	virtual QString getName() const;

    protected:
	MainWindowBase *m_mw;
	Pane *m_pane; // Main window owns this, but I determine its lifespan
	Pane *m_prevCurrentPane; // I don't own this
	bool m_added;
    };

    virtual bool checkSaveModified() = 0;

    virtual QString getOpenFileName(FileFinder::FileType type);
    virtual QString getSaveFileName(FileFinder::FileType type);
    virtual void registerLastOpenedFilePath(FileFinder::FileType type, QString path);

    virtual void createPlayTarget();
    virtual void openHelpUrl(QString url);

    virtual void setupMenus() = 0;
    virtual void updateVisibleRangeDisplay(Pane *p) const = 0;

    virtual void toXml(QTextStream &stream);
};


#endif
