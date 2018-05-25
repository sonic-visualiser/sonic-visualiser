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

#ifndef SV_PREFERENCES_DIALOG_H
#define SV_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QMap>
#include <QColor>

#include "base/Window.h"

class WindowTypeSelector;
class QPushButton;
class QLineEdit;
class QTabWidget;
class QComboBox;
class PluginPathConfigurator;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget *parent = 0);
    virtual ~PreferencesDialog();

    enum Tab {
        GeneralTab,
        AudioIOTab,
        AppearanceTab,
        AnalysisTab,
        TemplateTab,
        PluginTab
    };
    void switchToTab(Tab tab);

signals:
    void audioDeviceChanged();
    void coloursChanged();
                             
public slots:
    void applicationClosing(bool quickly);

protected slots:
    void windowTypeChanged(WindowType type);
    void spectrogramSmoothingChanged(int state);
    void spectrogramXSmoothingChanged(int state);
    void spectrogramGColourChanged(int state);
    void spectrogramMColourChanged(int state);
    void colour3DColourChanged(int state);
    void overviewColourChanged(int state);
    void propertyLayoutChanged(int layout);
    void tuningFrequencyChanged(double freq);
    void audioImplementationChanged(int impl);
    void audioPlaybackDeviceChanged(int device);
    void audioRecordDeviceChanged(int device);
    void resampleOnLoadChanged(int state);
    void gaplessModeChanged(int state);
    void vampProcessSeparationChanged(int state);
    void tempDirRootChanged(QString root);
    void backgroundModeChanged(int mode);
    void timeToTextModeChanged(int mode);
    void showHMSChanged(int state);
    void octaveSystemChanged(int system);
    void viewFontSizeChanged(int sz);
    void showSplashChanged(int state);
    void defaultTemplateChanged(int);
    void localeChanged(int);
    void networkPermissionChanged(int state);
    void retinaChanged(int state);
    void pluginPathsChanged();

    void tempDirButtonClicked();

    void okClicked();
    void applyClicked();
    void cancelClicked();

protected:
    WindowTypeSelector *m_windowTypeSelector;
    QPushButton *m_applyButton;

    QTabWidget *m_tabs;
    QMap<Tab, int> m_tabOrdering;

    QLineEdit *m_tempDirRootEdit;

    QComboBox *m_audioPlaybackDeviceCombo;
    QComboBox *m_audioRecordDeviceCombo;
    void rebuildDeviceCombos();

    PluginPathConfigurator *m_pluginPathConfigurator;
    
    QString m_currentTemplate;
    QStringList m_templates;

    QString m_currentLocale;
    QStringList m_locales;
    
    WindowType m_windowType;
    int m_spectrogramSmoothing;
    int m_spectrogramXSmoothing;
    int m_spectrogramGColour;
    int m_spectrogramMColour;
    int m_colour3DColour;
    QColor m_overviewColour;
    int m_propertyLayout;
    double m_tuningFrequency;
    int m_audioImplementation;
    int m_audioPlaybackDevice;
    int m_audioRecordDevice;
    bool m_resampleOnLoad;
    bool m_gapless;
    bool m_runPluginsInProcess;
    bool m_networkPermission;
    bool m_retina;
    QString m_tempDirRoot;
    int m_backgroundMode;
    int m_timeToTextMode;
    bool m_showHMS;
    int m_octaveSystem;
    int m_viewFontSize;
    bool m_showSplash;

    bool m_audioDeviceChanged;
    bool m_coloursChanged;
    bool m_changesOnRestart;
};

#endif
