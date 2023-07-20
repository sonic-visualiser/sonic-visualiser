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

#ifndef SV_MAIN_WINDOW_H
#define SV_MAIN_WINDOW_H

#include "framework/MainWindowBase.h"

class VersionTester;
class Surveyer;
class LayerTreeDialog;
class ActivityLog;
class UnitConverter;

class QFileSystemWatcher;
class QScrollArea;

class MainWindow : public MainWindowBase
{
    Q_OBJECT

public:
    MainWindow(AudioMode audioMode, MIDIMode midiMode, bool withOSCSupport);
    virtual ~MainWindow();

signals:
    void canChangeSolo(bool);
    void canAlign(bool);

public slots:
    void preferenceChanged(PropertyContainer::PropertyName) override;
    virtual void coloursChanged();

    virtual bool commitData(bool mayAskUser);

    void goFullScreen();
    void endFullScreen();

protected slots:
    virtual void importAudio();
    virtual void importMoreAudio();
    virtual void replaceMainAudio();
    virtual void openSomething();
    virtual void openLocation();
    virtual void openRecentFile();
    virtual void applyTemplate();
    virtual void exportAudio();
    virtual void exportAudioData();
    virtual void convertAudio();
    virtual void importLayer();
    virtual void exportLayer();
    virtual void exportImage();
    virtual void exportSVG();
    virtual void browseRecordedAudio();
    virtual void saveSession();
    virtual void saveSessionAs();
    virtual void newSession();
    void closeSession() override;
    virtual void preferences();

    void sampleRateMismatch(sv_samplerate_t, sv_samplerate_t, bool) override;
    void audioOverloadPluginDisabled() override;

    virtual void toolNavigateSelected();
    virtual void toolSelectSelected();
    virtual void toolEditSelected();
    virtual void toolDrawSelected();
    virtual void toolEraseSelected();
    virtual void toolMeasureSelected();

    void documentModified() override;
    void documentRestored() override;
    virtual void documentReplaced();

    void updateMenuStates() override;
    void updateDescriptionLabel() override;

    virtual void setInstantsNumbering();
    virtual void setInstantsCounterCycle();
    virtual void setInstantsCounters();
    virtual void resetInstantsCounters();
    virtual void subdivideInstants();
    virtual void winnowInstants();

    void modelGenerationFailed(QString, QString) override;
    void modelGenerationWarning(QString, QString) override;
    void modelRegenerationFailed(QString, QString, QString) override;
    void modelRegenerationWarning(QString, QString, QString) override;
    void alignmentFailed(ModelId, QString) override;

    void paneRightButtonMenuRequested(Pane *, QPoint point) override;
    void panePropertiesRightButtonMenuRequested(Pane *, QPoint point) override;
    void layerPropertiesRightButtonMenuRequested(Pane *, Layer *, QPoint point) override;

    virtual void propertyStacksResized(int);

    virtual void addPane();
    virtual void addLayer();
    virtual void addLayer(QString transformId);
    virtual void renameCurrentLayer();

    virtual void findTransform();

    void paneAdded(Pane *) override;
    void paneHidden(Pane *) override;
    void paneAboutToBeDeleted(Pane *) override;
    void paneDropAccepted(Pane *, QStringList) override;
    void paneDropAccepted(Pane *, QString) override;

    void paneCancelButtonPressed(Layer *);

    virtual void setupRecentFilesMenu();
    virtual void setupRecentTransformsMenu();
    virtual void setupTemplatesMenu();

    virtual void playSpeedChanged(int);
    void playSoloToggled() override;
    virtual void alignToggled();

    void currentPaneChanged(Pane *) override;

    virtual void speedUpPlayback();
    virtual void slowDownPlayback();
    virtual void restoreNormalPlayback();

    void monitoringLevelsChanged(float, float) override;

    void layerRemoved(Layer *) override;
    void layerInAView(Layer *, bool) override;

    void mainModelChanged(ModelId) override;
    virtual void mainModelGainChanged(float);
    virtual void mainModelPanChanged(float);
    void modelAdded(ModelId) override;

    virtual void showLayerTree();
    virtual void showActivityLog();
    virtual void showUnitConverter();

    virtual void mouseEnteredWidget();
    virtual void mouseLeftWidget();

    void handleOSCMessage(const OSCMessage &) override;
    virtual void midiEventsAvailable();
    virtual void playStatusChanged(bool);

    void installedTransformsPopulated();
    void populateTransformsMenu();
    
    virtual void betaReleaseWarning();
    virtual void pluginPopulationWarning(QString text);

    virtual void saveSessionAsTemplate();
    virtual void manageSavedTemplates();

    virtual void website();
    virtual void help();
    virtual void about();
    virtual void whatsNew();
    virtual void keyReference();
    void newerVersionAvailable(QString) override;

protected:
    Overview                *m_overview;
    LevelPanToolButton      *m_mainLevelPan;
    AudioDial               *m_playSpeed;
    WaveformLayer           *m_panLayer;
    
    QScrollArea             *m_mainScroll;

    bool                     m_mainMenusCreated;
    QMenu                   *m_paneMenu;
    QMenu                   *m_layerMenu;
    QMenu                   *m_transformsMenu;
    QMenu                   *m_playbackMenu;
    QMenu                   *m_existingLayersMenu;
    QMenu                   *m_sliceMenu;
    QMenu                   *m_recentFilesMenu;
    QMenu                   *m_recentTransformsMenu;
    QMenu                   *m_templatesMenu;
    QMenu                   *m_rightButtonMenu;
    QMenu                   *m_rightButtonLayerMenu;
    QMenu                   *m_rightButtonTransformsMenu;
    QMenu                   *m_rightButtonPlaybackMenu;
    QMenu                   *m_lastRightButtonPropertyMenu;

    QAction                 *m_deleteSelectedAction;
    QAction                 *m_soloAction;
    QAction                 *m_rwdStartAction;
    QAction                 *m_rwdSimilarAction;
    QAction                 *m_rwdAction;
    QAction                 *m_ffwdAction;
    QAction                 *m_ffwdSimilarAction;
    QAction                 *m_ffwdEndAction;
    QAction                 *m_playAction;
    QAction                 *m_recordAction;
    QAction                 *m_playSelectionAction;
    QAction                 *m_playLoopAction;
    QAction                 *m_manageTemplatesAction;
    QAction                 *m_zoomInAction;
    QAction                 *m_zoomOutAction;
    QAction                 *m_zoomFitAction;
    QAction                 *m_scrollLeftAction;
    QAction                 *m_scrollRightAction;
    QAction                 *m_showPropertyBoxesAction;

    bool                     m_soloModified;
    bool                     m_prevSolo;

    QFrame                  *m_playControlsSpacer;
    int                      m_playControlsWidth;

    QLabel                  *m_descriptionLabel;
    QLabel                  *m_currentLabel;

    QPointer<PreferencesDialog> m_preferencesDialog;
    QPointer<LayerTreeDialog>   m_layerTreeDialog;

    ActivityLog             *m_activityLog;
    UnitConverter           *m_unitConverter;
    KeyReference            *m_keyReference;

    QFileSystemWatcher      *m_templateWatcher;

    bool                     m_shouldStartOSCQueue;
    
    Surveyer                *m_surveyer;
    VersionTester           *m_versionTester;
    QString                  m_newerVersionIs;

    struct LayerConfiguration {
        LayerConfiguration(LayerFactory::LayerType _layer
                                               = LayerFactory::TimeRuler,
                           ModelId _source = ModelId(),
                           int _channel = -1) :
            layer(_layer), sourceModel(_source), channel(_channel) { }
        LayerFactory::LayerType layer;
        ModelId sourceModel;
        int channel;
    };

    QString shortcutFor(LayerFactory::LayerType, bool isPaneMenu);
    void updateLayerShortcutsFor(ModelId);

    // Map from menu action to the resulting layer configurations
    // etc. These all used to be std::maps, but we sometimes want to
    // iterate through actions in order of creation, not in order of
    // arbitrary QAction pointer. And speed of random lookup is not
    // important.
    //
    // Some of these would still be fine as maps, but we might as well
    // consistently use the same arrangement throughout.
    
    typedef std::vector<std::pair<QAction *, LayerConfiguration>> PaneActions;
    PaneActions m_paneActions;

    typedef std::vector<std::pair<QAction *, LayerConfiguration>> LayerActions;
    LayerActions m_layerActions;

    typedef std::vector<std::pair<QAction *, Layer *>> ExistingLayerActions;
    ExistingLayerActions m_existingLayerActions;
    ExistingLayerActions m_sliceActions;

    typedef std::vector<std::pair<ViewManager::ToolMode, QAction *>> ToolActions;
    ToolActions m_toolActions;

    typedef std::vector<std::pair<QAction *, int>> NumberingActions;
    NumberingActions m_numberingActions;

    typedef std::vector<std::pair<QAction *, TransformId>> TransformActions;
    TransformActions m_transformActions;

    // This one only makes sense as a map though
    typedef std::map<TransformId, QAction *> TransformActionReverseMap;
    TransformActionReverseMap m_transformActionsReverse;

    QString getReleaseText() const;
    
    void setupMenus() override;

    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void setupPaneAndLayerMenus();
    void prepareTransformsMenu();
    void setupHelpMenu();
    void setupExistingLayersMenus();
    void setupToolbars();
    
    void addPane(const LayerConfiguration &configuration, QString text);

    void closeEvent(QCloseEvent *e) override;
    bool checkSaveModified() override;

    void exportAudio(bool asData);

    void updateVisibleRangeDisplay(Pane *p) const override;
    void updatePositionStatusDisplays() const override;

    bool shouldCreateNewSessionForRDFAudio(bool *cancel) override;
    
    void connectLayerEditDialog(ModelDataTableDialog *) override;
};


#endif
