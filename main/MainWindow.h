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

#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include <QFrame>
#include <QString>
#include <QMainWindow>
#include <QPointer>

#include "base/Command.h"
#include "base/ViewManager.h"
#include "base/PropertyContainer.h"
#include "layer/LayerFactory.h"
#include "transform/Transform.h"
#include "fileio/SVFileReader.h"
#include <map>

class Document;
class PaneStack;
class Pane;
class View;
class Fader;
class Panner;
class Layer;
class WaveformLayer;
class WaveFileModel;
class AudioCallbackPlaySource;
class AudioCallbackPlayTarget;
class CommandHistory;
class QMenu;
class AudioDial;
class QLabel;
class PreferencesDialog;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    virtual ~MainWindow();
    
    enum AudioFileOpenMode {
        ReplaceMainModel,
        CreateAdditionalModel,
        AskUser
    };

    bool openSomeFile(QString path);
    bool openAudioFile(QString path, AudioFileOpenMode = AskUser);
    bool openLayerFile(QString path);
    bool openSessionFile(QString path);
    bool saveSessionFile(QString path);

signals:
    // Used to toggle the availability of menu actions
    void canAddPane(bool);
    void canDeleteCurrentPane(bool);
    void canAddLayer(bool);
    void canImportMoreAudio(bool);
    void canImportLayer(bool);
    void canExportAudio(bool);
    void canExportLayer(bool);
    void canRenameLayer(bool);
    void canEditLayer(bool);
    void canSelect(bool);
    void canClearSelection(bool);
    void canEditSelection(bool);
    void canPaste(bool);
    void canInsertInstant(bool);
    void canDeleteCurrentLayer(bool);
    void canZoom(bool);
    void canScroll(bool);
    void canPlay(bool);
    void canFfwd(bool);
    void canRewind(bool);
    void canPlaySelection(bool);
    void canSave(bool);

protected slots:
    void openSession();
    void importAudio();
    void importMoreAudio();
    void openSomething();
    void openRecentFile();
    void exportAudio();
    void importLayer();
    void exportLayer();
    void saveSession();
    void saveSessionAs();
    void newSession();
    void closeSession();
    void preferences();

    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomDefault();
    void scrollLeft();
    void scrollRight();
    void jumpLeft();
    void jumpRight();

    void showNoOverlays();
    void showBasicOverlays();
    void showAllOverlays();

    void play();
    void ffwd();
    void ffwdEnd();
    void rewind();
    void rewindStart();
    void stop();

    void addPane();
    void addLayer();
    void deleteCurrentPane();
    void renameCurrentLayer();
    void deleteCurrentLayer();

    void playLoopToggled();
    void playSelectionToggled();
    void playSpeedChanged(int);
    void sampleRateMismatch(size_t, size_t, bool);

    void outputLevelsChanged(float, float);

    void currentPaneChanged(Pane *);
    void currentLayerChanged(Pane *, Layer *);

    void toolNavigateSelected();
    void toolSelectSelected();
    void toolEditSelected();
    void toolDrawSelected();

    void selectAll();
    void selectToStart();
    void selectToEnd();
    void selectVisible();
    void clearSelection();
    void cut();
    void copy();
    void paste();
    void deleteSelected();
    void insertInstant();

    void documentModified();
    void documentRestored();

    void updateMenuStates();
    void updateDescriptionLabel();

    void layerAdded(Layer *);
    void layerRemoved(Layer *);
    void layerAboutToBeDeleted(Layer *);
    void layerInAView(Layer *, bool);

    void mainModelChanged(WaveFileModel *);
    void modelAdded(Model *);
    void modelAboutToBeDeleted(Model *);

    void modelGenerationFailed(QString);
    void modelRegenerationFailed(QString, QString);

    void rightButtonMenuRequested(Pane *, QPoint point);

    void preferenceChanged(PropertyContainer::PropertyName);

    void setupRecentFilesMenu();

    void showLayerTree();

    void website();
    void help();
    void about();

protected:
    QString                  m_sessionFile;
    QString                  m_audioFile;
    Document                *m_document;

    QLabel                  *m_descriptionLabel;
    PaneStack               *m_paneStack;
    ViewManager             *m_viewManager;
    Panner                  *m_panner;
    Fader                   *m_fader;
    AudioDial               *m_playSpeed;
    WaveformLayer           *m_panLayer;
    Layer                   *m_timeRulerLayer;

    AudioCallbackPlaySource *m_playSource;
    AudioCallbackPlayTarget *m_playTarget;

    bool                     m_mainMenusCreated;
    QMenu                   *m_paneMenu;
    QMenu                   *m_layerMenu;
    QMenu                   *m_existingLayersMenu;
    QMenu                   *m_recentFilesMenu;
    QMenu                   *m_rightButtonMenu;
    QMenu                   *m_rightButtonLayerMenu;

    bool                     m_documentModified;

    QPointer<PreferencesDialog> m_preferencesDialog;

    WaveFileModel *getMainModel();
    void createDocument();

    struct PaneConfiguration {
	PaneConfiguration(LayerFactory::LayerType _layer
			                       = LayerFactory::TimeRuler,
			  int _channel = -1) :
	    layer(_layer), channel(_channel) { }
	LayerFactory::LayerType layer;
	int channel;
    };

    typedef std::map<QAction *, PaneConfiguration> PaneActionMap;
    PaneActionMap m_paneActions;

    typedef std::map<QAction *, TransformName> TransformActionMap;
    TransformActionMap m_layerTransformActions;

    typedef std::map<QAction *, LayerFactory::LayerType> LayerActionMap;
    LayerActionMap m_layerActions;

    typedef std::map<QAction *, Layer *> ExistingLayerActionMap;
    ExistingLayerActionMap m_existingLayerActions;

    typedef std::map<ViewManager::ToolMode, QAction *> ToolActionMap;
    ToolActionMap m_toolActions;

    void setupMenus();
    void setupExistingLayersMenu();
    void setupToolbars();
    Pane *addPaneToStack();

    class PaneCallback : public SVFileReaderPaneCallback
    {
    public:
	PaneCallback(MainWindow *mw) : m_mw(mw) { }
	virtual Pane *addPane() { return m_mw->addPaneToStack(); }
	virtual void setWindowSize(int width, int height) {
	    m_mw->resize(width, height);
	}
	virtual void addSelection(int start, int end) {
	    m_mw->m_viewManager->addSelection(Selection(start, end));
	}
    protected:
	MainWindow *m_mw;
    };

    class AddPaneCommand : public Command
    {
    public:
	AddPaneCommand(MainWindow *mw);
	virtual ~AddPaneCommand();
	
	virtual void execute();
	virtual void unexecute();
	virtual QString getName() const;

	Pane *getPane() { return m_pane; }

    protected:
	MainWindow *m_mw;
	Pane *m_pane; // Main window owns this, but I determine its lifespan
	Pane *m_prevCurrentPane; // I don't own this
	bool m_added;
    };

    class RemovePaneCommand : public Command
    {
    public:
	RemovePaneCommand(MainWindow *mw, Pane *pane);
	virtual ~RemovePaneCommand();
	
	virtual void execute();
	virtual void unexecute();
	virtual QString getName() const;

    protected:
	MainWindow *m_mw;
	Pane *m_pane; // Main window owns this, but I determine its lifespan
	Pane *m_prevCurrentPane; // I don't own this
	bool m_added;
    };

    virtual void closeEvent(QCloseEvent *e);
    bool checkSaveModified();

    void createPlayTarget();

    void openHelpUrl(QString url);

    void toXml(QTextStream &stream);
};


#endif
