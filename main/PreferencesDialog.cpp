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

#include "PreferencesDialog.h"

#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QString>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QSpinBox>
#include <QListWidget>
#include <QSettings>

#include <set>

#include "widgets/WindowTypeSelector.h"
#include "widgets/IconLoader.h"
#include "widgets/ColourMapComboBox.h"
#include "widgets/ColourComboBox.h"
#include "widgets/PluginPathConfigurator.h"
#include "widgets/WidgetScale.h"
#include "base/Preferences.h"
#include "base/ResourceFinder.h"
#include "layer/ColourMapper.h"
#include "layer/ColourDatabase.h"

#include "bqaudioio/AudioFactory.h"

#include "../version.h"

using namespace std;

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    m_audioImplementation(0),
    m_audioPlaybackDevice(0),
    m_audioRecordDevice(0),
    m_audioDeviceChanged(false),
    m_coloursChanged(false),
    m_changesOnRestart(false)
{
    setWindowTitle(tr("Sonic Visualiser: Application Preferences"));

    Preferences *prefs = Preferences::getInstance();

    QGridLayout *grid = new QGridLayout;
    setLayout(grid);

    m_tabs = new QTabWidget;
    grid->addWidget(m_tabs, 0, 0);
    
    m_tabs->setTabPosition(QTabWidget::North);

    // Create this first, as slots that get called from the ctor will
    // refer to it
    m_applyButton = new QPushButton(tr("Apply"));

    // Create all the preference widgets first, then create the
    // individual tab widgets and place the preferences in their
    // appropriate places in one go afterwards

    int min, max, deflt, i;

    m_windowType = WindowType(prefs->getPropertyRangeAndValue
                              ("Window Type", &min, &max, &deflt));
    m_windowTypeSelector = new WindowTypeSelector(m_windowType);

    connect(m_windowTypeSelector, SIGNAL(windowTypeChanged(WindowType)),
            this, SLOT(windowTypeChanged(WindowType)));

    QCheckBox *vampProcessSeparation = new QCheckBox;
    m_runPluginsInProcess = prefs->getRunPluginsInProcess();
    vampProcessSeparation->setCheckState(m_runPluginsInProcess ? Qt::Unchecked :
                                         Qt::Checked);
    connect(vampProcessSeparation, SIGNAL(stateChanged(int)),
            this, SLOT(vampProcessSeparationChanged(int)));
    
    QComboBox *smoothing = new QComboBox;
    
    int sm = prefs->getPropertyRangeAndValue("Spectrogram Y Smoothing", &min, &max,
                                             &deflt);
    m_spectrogramSmoothing = sm;

    for (i = min; i <= max; ++i) {
        smoothing->addItem(prefs->getPropertyValueLabel("Spectrogram Y Smoothing", i));
    }

    smoothing->setCurrentIndex(sm);

    connect(smoothing, SIGNAL(currentIndexChanged(int)),
            this, SLOT(spectrogramSmoothingChanged(int)));

    QComboBox *xsmoothing = new QComboBox;
    
    int xsm = prefs->getPropertyRangeAndValue("Spectrogram X Smoothing", &min, &max,
                                             &deflt);
    m_spectrogramXSmoothing = xsm;

    for (i = min; i <= max; ++i) {
        xsmoothing->addItem(prefs->getPropertyValueLabel("Spectrogram X Smoothing", i));
    }

    xsmoothing->setCurrentIndex(xsm);

    connect(xsmoothing, SIGNAL(currentIndexChanged(int)),
            this, SLOT(spectrogramXSmoothingChanged(int)));

    QComboBox *propertyLayout = new QComboBox;
    int pl = prefs->getPropertyRangeAndValue("Property Box Layout", &min, &max,
                                         &deflt);
    m_propertyLayout = pl;

    for (i = min; i <= max; ++i) {
        propertyLayout->addItem(prefs->getPropertyValueLabel("Property Box Layout", i));
    }

    propertyLayout->setCurrentIndex(pl);

    connect(propertyLayout, SIGNAL(currentIndexChanged(int)),
            this, SLOT(propertyLayoutChanged(int)));

    QSettings settings;
    settings.beginGroup("Preferences");
    m_spectrogramGColour = (settings.value("spectrogram-colour",
                                           int(ColourMapper::Green)).toInt());
    m_spectrogramMColour = (settings.value("spectrogram-melodic-colour",
                                           int(ColourMapper::Sunset)).toInt());
    m_colour3DColour = (settings.value("colour-3d-plot-colour",
                                       int(ColourMapper::Green)).toInt());
    m_overviewColour = ColourDatabase::getInstance()->getColour(tr("Green"));
    if (settings.contains("overview-colour")) {
        QString qcolorName =
            settings.value("overview-colour", m_overviewColour.name())
            .toString();
        m_overviewColour.setNamedColor(qcolorName);
        SVCERR << "loaded colour " << m_overviewColour.name() << " from settings" << endl;
    }
    settings.endGroup();

    ColourMapComboBox *spectrogramGColour = new ColourMapComboBox(true);
    spectrogramGColour->setCurrentIndex(m_spectrogramGColour);

    ColourMapComboBox *spectrogramMColour = new ColourMapComboBox(true);
    spectrogramMColour->setCurrentIndex(m_spectrogramMColour);

    ColourMapComboBox *colour3DColour = new ColourMapComboBox(true);
    colour3DColour->setCurrentIndex(m_colour3DColour);

    // can't have "add new colour", as it gets saved in the session not in prefs
    ColourComboBox *overviewColour = new ColourComboBox(false);
    int overviewColourIndex =
        ColourDatabase::getInstance()->getColourIndex(m_overviewColour);
    SVCERR << "index = " << overviewColourIndex << " for colour " << m_overviewColour.name() << endl;
    if (overviewColourIndex >= 0) {
        overviewColour->setCurrentIndex(overviewColourIndex);
    }

    connect(spectrogramGColour, SIGNAL(colourMapChanged(int)),
            this, SLOT(spectrogramGColourChanged(int)));
    connect(spectrogramMColour, SIGNAL(colourMapChanged(int)),
            this, SLOT(spectrogramMColourChanged(int)));
    connect(colour3DColour, SIGNAL(colourMapChanged(int)),
            this, SLOT(colour3DColourChanged(int)));
    connect(overviewColour, SIGNAL(colourChanged(int)),
            this, SLOT(overviewColourChanged(int)));

    m_tuningFrequency = prefs->getTuningFrequency();

    QDoubleSpinBox *frequency = new QDoubleSpinBox;
    frequency->setMinimum(100.0);
    frequency->setMaximum(5000.0);
    frequency->setSuffix(" Hz");
    frequency->setSingleStep(1);
    frequency->setValue(m_tuningFrequency);
    frequency->setDecimals(2);

    connect(frequency, SIGNAL(valueChanged(double)),
            this, SLOT(tuningFrequencyChanged(double)));

    QComboBox *octaveSystem = new QComboBox;
    int oct = prefs->getPropertyRangeAndValue
        ("Octave Numbering System", &min, &max, &deflt);
    m_octaveSystem = oct;
    for (i = min; i <= max; ++i) {
        octaveSystem->addItem(prefs->getPropertyValueLabel
                              ("Octave Numbering System", i));
    }
    octaveSystem->setCurrentIndex(oct);

    connect(octaveSystem, SIGNAL(currentIndexChanged(int)),
            this, SLOT(octaveSystemChanged(int)));

    settings.beginGroup("Preferences");

    QComboBox *audioImplementation = new QComboBox;
    connect(audioImplementation, SIGNAL(currentIndexChanged(int)),
            this, SLOT(audioImplementationChanged(int)));

    m_audioPlaybackDeviceCombo = new QComboBox;
    connect(m_audioPlaybackDeviceCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(audioPlaybackDeviceChanged(int)));

    m_audioRecordDeviceCombo = new QComboBox;
    connect(m_audioRecordDeviceCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(audioRecordDeviceChanged(int)));

    vector<string> implementationNames =
        breakfastquay::AudioFactory::getImplementationNames();

    QString implementationName = settings.value("audio-target", "").toString();
    if (implementationName == "auto") implementationName = "";
    if (implementationName == "" && implementationNames.size() == 1) {
        // We won't be showing the implementations menu in this case
        implementationName = implementationNames[0].c_str();
    }

    audioImplementation->addItem(tr("(auto)"));
    m_audioImplementation = 0;

    for (int i = 0; in_range_for(implementationNames, i); ++i) {
        audioImplementation->addItem
            (breakfastquay::AudioFactory::getImplementationDescription
             (implementationNames[i]).c_str());
        if (implementationName.toStdString() == implementationNames[i]) {
            audioImplementation->setCurrentIndex(i+1);
            m_audioImplementation = i+1;
        }
    }
    
    settings.endGroup();

    rebuildDeviceCombos();
    m_audioDeviceChanged = false; // the rebuild will have changed this

    QCheckBox *resampleOnLoad = new QCheckBox;
    m_resampleOnLoad = prefs->getResampleOnLoad();
    resampleOnLoad->setCheckState(m_resampleOnLoad ? Qt::Checked :
                                  Qt::Unchecked);
    connect(resampleOnLoad, SIGNAL(stateChanged(int)),
            this, SLOT(resampleOnLoadChanged(int)));

    QCheckBox *gaplessMode = new QCheckBox;
    m_gapless = prefs->getUseGaplessMode();
    gaplessMode->setCheckState(m_gapless ? Qt::Checked : Qt::Unchecked);
    connect(gaplessMode, SIGNAL(stateChanged(int)),
            this, SLOT(gaplessModeChanged(int)));

    m_tempDirRootEdit = new QLineEdit;
    QString dir = prefs->getTemporaryDirectoryRoot();
    m_tempDirRoot = dir;
    dir.replace("$HOME", tr("<home directory>"));
    m_tempDirRootEdit->setText(dir);
    m_tempDirRootEdit->setReadOnly(true);
    QPushButton *tempDirButton = new QPushButton;
    tempDirButton->setIcon(IconLoader().load("fileopen"));
    connect(tempDirButton, SIGNAL(clicked()),
            this, SLOT(tempDirButtonClicked()));
    tempDirButton->setFixedSize(WidgetScale::scaleQSize(QSize(24, 24)));

    QCheckBox *showSplash = new QCheckBox;
    m_showSplash = prefs->getShowSplash();
    showSplash->setCheckState(m_showSplash ? Qt::Checked : Qt::Unchecked);
    connect(showSplash, SIGNAL(stateChanged(int)),
            this, SLOT(showSplashChanged(int)));

#ifdef NOT_DEFINED // This no longer works correctly on any platform AFAICS
    QComboBox *bgMode = new QComboBox;
    int bg = prefs->getPropertyRangeAndValue("Background Mode", &min, &max,
                                             &deflt);
    m_backgroundMode = bg;
    for (i = min; i <= max; ++i) {
        bgMode->addItem(prefs->getPropertyValueLabel("Background Mode", i));
    }
    bgMode->setCurrentIndex(bg);

    connect(bgMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(backgroundModeChanged(int)));
#endif

    settings.beginGroup("Preferences");

#ifdef Q_OS_MAC
    m_retina = settings.value("scaledHiDpi", true).toBool();
    QCheckBox *retina = new QCheckBox;
    retina->setCheckState(m_retina ? Qt::Checked : Qt::Unchecked);
    connect(retina, SIGNAL(stateChanged(int)), this, SLOT(retinaChanged(int)));
#else
    m_retina = false;
#endif

    QString userLocale = settings.value("locale", "").toString();
    m_currentLocale = userLocale;
    
    QString permishTag = QString("network-permission-%1").arg(SV_VERSION);
    m_networkPermission = settings.value(permishTag, false).toBool();

    settings.endGroup();

    QComboBox *locale = new QComboBox;
    QStringList localeFiles = QDir(":i18n").entryList(QStringList() << "*.qm");
    locale->addItem(tr("Follow system locale"));
    m_locales.push_back("");
    if (userLocale == "") {
        locale->setCurrentIndex(0);
    }
    foreach (QString f, localeFiles) {
        QString f0 = f;
        f.replace("sonic-visualiser_", "").replace(".qm", "");
        if (f == f0) { // our expectations about filename format were not met
            SVCERR << "INFO: Unexpected filename " << f << " in i18n resource directory" << endl;
        } else {
            m_locales.push_back(f);
            QString displayText;
            // Add new translations here
            if (f == "ru") displayText = tr("Russian");
            else if (f == "en_GB") displayText = tr("British English");
            else if (f == "en_US") displayText = tr("American English");
            else if (f == "cs_CZ") displayText = tr("Czech");
            else displayText = f;
            locale->addItem(QString("%1 [%2]").arg(displayText).arg(f));
            if (userLocale == f) {
                locale->setCurrentIndex(locale->count() - 1);
            }
        }
    }
    connect(locale, SIGNAL(currentIndexChanged(int)),
            this, SLOT(localeChanged(int)));

    QCheckBox *networkPermish = new QCheckBox;
    networkPermish->setCheckState(m_networkPermission ? Qt::Checked : Qt::Unchecked);
    connect(networkPermish, SIGNAL(stateChanged(int)),
            this, SLOT(networkPermissionChanged(int)));

    QSpinBox *fontSize = new QSpinBox;
    int fs = prefs->getPropertyRangeAndValue("View Font Size", &min, &max,
                                             &deflt);
    m_viewFontSize = fs;
    fontSize->setMinimum(min);
    fontSize->setMaximum(max);
    fontSize->setSuffix(" pt");
    fontSize->setSingleStep(1);
    fontSize->setValue(fs);

    connect(fontSize, SIGNAL(valueChanged(int)),
            this, SLOT(viewFontSizeChanged(int)));

    QComboBox *ttMode = new QComboBox;
    int tt = prefs->getPropertyRangeAndValue("Time To Text Mode", &min, &max,
                                             &deflt);
    m_timeToTextMode = tt;
    for (i = min; i <= max; ++i) {
        ttMode->addItem(prefs->getPropertyValueLabel("Time To Text Mode", i));
    }
    ttMode->setCurrentIndex(tt);

    connect(ttMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(timeToTextModeChanged(int)));

    QCheckBox *hms = new QCheckBox;
    int showHMS = prefs->getPropertyRangeAndValue
        ("Show Hours And Minutes", &min, &max, &deflt);
    m_showHMS = (showHMS != 0);
    hms->setCheckState(m_showHMS ? Qt::Checked : Qt::Unchecked);
    connect(hms, SIGNAL(stateChanged(int)),
            this, SLOT(showHMSChanged(int)));

    QFrame *frame = nullptr;
    QGridLayout *subgrid = nullptr;
    int row = 0;

    // Appearance tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

#ifdef Q_OS_MAC
    if (devicePixelRatio() > 1) {
        subgrid->addWidget(new QLabel(tr("Draw layers at Retina resolution:")), row, 0);
        subgrid->addWidget(retina, row++, 1, 1, 1);
    }
#endif

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Property Box Layout"))),
                       row, 0);
    subgrid->addWidget(propertyLayout, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("Default spectrogram colour:")),
                       row, 0);
    subgrid->addWidget(spectrogramGColour, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("Default melodic spectrogram colour:")),
                       row, 0);
    subgrid->addWidget(spectrogramMColour, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("Default colour 3D plot colour:")),
                       row, 0);
    subgrid->addWidget(colour3DColour, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("Overview waveform colour:")),
                       row, 0);
    subgrid->addWidget(overviewColour, row++, 1, 1, 2);

#ifdef NOT_DEFINED // see earlier
    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Background Mode"))),
                       row, 0);
    subgrid->addWidget(bgMode, row++, 1, 1, 2);
#endif

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("View Font Size"))),
                       row, 0);
    subgrid->addWidget(fontSize, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Time To Text Mode"))),
                       row, 0);
    subgrid->addWidget(ttMode, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Show Hours And Minutes"))),
                       row, 0);
    subgrid->addWidget(hms, row++, 1, 1, 1);

    subgrid->setRowStretch(row, 10);
    
    m_tabOrdering[AppearanceTab] = m_tabs->count();
    m_tabs->addTab(frame, tr("&Appearance"));

    // Analysis tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Tuning Frequency"))),
                       row, 0);
    subgrid->addWidget(frequency, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Octave Numbering System"))),
                       row, 0);
    subgrid->addWidget(octaveSystem, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(prefs->getPropertyLabel
                                  ("Spectrogram Y Smoothing")),
                       row, 0);
    subgrid->addWidget(smoothing, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(prefs->getPropertyLabel
                                  ("Spectrogram X Smoothing")),
                       row, 0);
    subgrid->addWidget(xsmoothing, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Window Type"))),
                       row, 0);
    subgrid->addWidget(m_windowTypeSelector, row++, 1, 2, 2);

    subgrid->setRowStretch(row, 10);
    row++;

    subgrid->addWidget(new QLabel(tr("Run Vamp plugins in separate process:")),
                       row, 0);
    subgrid->addWidget(vampProcessSeparation, row++, 1, 1, 1);
    
    subgrid->setRowStretch(row, 10);
    
    m_tabOrdering[AnalysisTab] = m_tabs->count();
    m_tabs->addTab(frame, tr("Anal&ysis"));

    // Template tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;
    
    subgrid->addWidget(new QLabel(tr("Default session template when loading audio files:")), row++, 0);

    QListWidget *lw = new QListWidget();
    subgrid->addWidget(lw, row, 0);
    subgrid->setRowStretch(row, 10);
    row++;

    subgrid->addWidget(new QLabel(tr("(Use \"%1\" in the File menu to add to these.)")
                                  .arg(tr("Export Session as Template..."))),
                       row++, 0);

    settings.beginGroup("MainWindow");
    m_currentTemplate = settings.value("sessiontemplate", "").toString();
    settings.endGroup();

    lw->addItem(tr("Standard Waveform"));
    if (m_currentTemplate == "" || m_currentTemplate == "default") {
        lw->setCurrentRow(lw->count()-1);
    }
    m_templates.push_back("");

    QStringList templates = ResourceFinder().getResourceFiles("templates", "svt");

    set<QString> byName;
    foreach (QString t, templates) {
        byName.insert(QFileInfo(t).baseName());
    }

    foreach (QString t, byName) {
        if (t.toLower() == "default") continue;
        m_templates.push_back(t);
        lw->addItem(t);
        if (m_currentTemplate == t) {
            lw->setCurrentRow(lw->count()-1);
        }
    }

    connect(lw, SIGNAL(currentRowChanged(int)), this, SLOT(defaultTemplateChanged(int)));

    m_tabOrdering[TemplateTab] = m_tabs->count();
    m_tabs->addTab(frame, tr("Session &Template"));

    // Audio IO tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

    if (implementationNames.size() > 1) {
        subgrid->addWidget(new QLabel(tr("Audio service:")), row, 0);
        subgrid->addWidget(audioImplementation, row++, 1, 1, 2);
    }

    subgrid->addWidget(new QLabel(tr("Audio playback device:")), row, 0);
    subgrid->addWidget(m_audioPlaybackDeviceCombo, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("Audio record device:")), row, 0);
    subgrid->addWidget(m_audioRecordDeviceCombo, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Use Gapless Mode"))),
                       row, 0);
    subgrid->addWidget(gaplessMode, row++, 1, 1, 1);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Resample On Load"))),
                       row, 0);
    subgrid->addWidget(resampleOnLoad, row++, 1, 1, 1);

    subgrid->setRowStretch(row, 10);
    
    m_tabOrdering[AudioIOTab] = m_tabs->count();
    m_tabs->addTab(frame, tr("A&udio I/O"));

    // Plugins tab

    m_pluginPathConfigurator = new PluginPathConfigurator(this);
    m_pluginPathConfigurator->setPaths(PluginPathSetter::getPaths());
    connect(m_pluginPathConfigurator, SIGNAL(pathsChanged()),
            this, SLOT(pluginPathsChanged()));
    
    m_tabOrdering[PluginTab] = m_tabs->count();
    m_tabs->addTab(m_pluginPathConfigurator, tr("&Plugins"));
    
    // General tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(tr("User interface language"))),
                       row, 0);
    subgrid->addWidget(locale, row++, 1, 1, 1);

    subgrid->addWidget(new QLabel(tr("%1:").arg(tr("Allow network usage"))),
                       row, 0);
    subgrid->addWidget(networkPermish, row++, 1, 1, 1);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Show Splash Screen"))),
                       row, 0);
    subgrid->addWidget(showSplash, row++, 1, 1, 1);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Temporary Directory Root"))),
                       row, 0);
    subgrid->addWidget(m_tempDirRootEdit, row, 1, 1, 1);
    subgrid->addWidget(tempDirButton, row, 2, 1, 1);
    row++;
    
    subgrid->setRowStretch(row, 10);
    
    m_tabOrdering[GeneralTab] = m_tabs->count();
    m_tabs->addTab(frame, tr("&Other"));

    QDialogButtonBox *bb = new QDialogButtonBox(Qt::Horizontal);
    grid->addWidget(bb, 1, 0);
    
    QPushButton *ok = new QPushButton(tr("OK"));
    QPushButton *cancel = new QPushButton(tr("Cancel"));
    bb->addButton(ok, QDialogButtonBox::AcceptRole);
    bb->addButton(m_applyButton, QDialogButtonBox::ApplyRole);
    bb->addButton(cancel, QDialogButtonBox::RejectRole);
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(m_applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    m_applyButton->setEnabled(false);
}

PreferencesDialog::~PreferencesDialog()
{
    SVDEBUG << "PreferencesDialog::~PreferencesDialog()" << endl;
}

void
PreferencesDialog::rebuildDeviceCombos()
{
    QSettings settings;
    settings.beginGroup("Preferences");

    vector<string> names = breakfastquay::AudioFactory::getImplementationNames();
    string implementationName;
    
    if (in_range_for(names, m_audioImplementation-1)) {
        implementationName = names[m_audioImplementation-1];
    }

    QString suffix;
    if (implementationName != "") {
        suffix = "-" + QString(implementationName.c_str());
    }
    
    names = breakfastquay::AudioFactory::getPlaybackDeviceNames(implementationName);
    QString playbackDeviceName = settings.value
        ("audio-playback-device" + suffix, "").toString();
    m_audioPlaybackDeviceCombo->clear();
    m_audioPlaybackDeviceCombo->addItem(tr("(auto)"));
    m_audioPlaybackDeviceCombo->setCurrentIndex(0);
    m_audioPlaybackDevice = 0;
    for (int i = 0; in_range_for(names, i); ++i) {
        m_audioPlaybackDeviceCombo->addItem(names[i].c_str());
        if (playbackDeviceName.toStdString() == names[i]) {
            m_audioPlaybackDeviceCombo->setCurrentIndex(i+1);
            m_audioPlaybackDevice = i+1;
        }
    }
    
    names = breakfastquay::AudioFactory::getRecordDeviceNames(implementationName);
    QString recordDeviceName = settings.value
        ("audio-record-device" + suffix, "").toString();
    m_audioRecordDeviceCombo->clear();
    m_audioRecordDeviceCombo->addItem(tr("(auto)"));
    m_audioRecordDeviceCombo->setCurrentIndex(0);
    m_audioRecordDevice = 0;
    for (int i = 0; in_range_for(names, i); ++i) {
        m_audioRecordDeviceCombo->addItem(names[i].c_str());
        if (recordDeviceName.toStdString() == names[i]) {
            m_audioRecordDeviceCombo->setCurrentIndex(i+1);
            m_audioRecordDevice = i+1;
        }
    }

    settings.endGroup();
}

void
PreferencesDialog::switchToTab(Tab t)
{
    if (m_tabOrdering.contains(t)) {
        m_tabs->setCurrentIndex(m_tabOrdering[t]);
    }
}

void
PreferencesDialog::windowTypeChanged(WindowType type)
{
    m_windowType = type;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::spectrogramSmoothingChanged(int smoothing)
{
    m_spectrogramSmoothing = smoothing;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::spectrogramXSmoothingChanged(int smoothing)
{
    m_spectrogramXSmoothing = smoothing;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::spectrogramGColourChanged(int colour)
{
    m_spectrogramGColour = colour;
    m_coloursChanged = true;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::spectrogramMColourChanged(int colour)
{
    m_spectrogramMColour = colour;
    m_coloursChanged = true;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::colour3DColourChanged(int colour)
{
    m_colour3DColour = colour;
    m_coloursChanged = true;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::overviewColourChanged(int colour)
{
    m_overviewColour = ColourDatabase::getInstance()->getColour(colour);
    m_coloursChanged = true;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::propertyLayoutChanged(int layout)
{
    m_propertyLayout = layout;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::tuningFrequencyChanged(double freq)
{
    m_tuningFrequency = freq;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::audioImplementationChanged(int s)
{
    if (m_audioImplementation != s) {
        m_audioImplementation = s;
        rebuildDeviceCombos();
        m_applyButton->setEnabled(true);
        m_audioDeviceChanged = true;
    }
}

void
PreferencesDialog::audioPlaybackDeviceChanged(int s)
{
    if (m_audioPlaybackDevice != s) {
        m_audioPlaybackDevice = s;
        m_applyButton->setEnabled(true);
        m_audioDeviceChanged = true;
    }
}

void
PreferencesDialog::audioRecordDeviceChanged(int s)
{
    if (m_audioRecordDevice != s) {
        m_audioRecordDevice = s;
        m_applyButton->setEnabled(true);
        m_audioDeviceChanged = true;
    }
}

void
PreferencesDialog::resampleOnLoadChanged(int state)
{
    m_resampleOnLoad = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::gaplessModeChanged(int state)
{
    m_gapless = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::vampProcessSeparationChanged(int state)
{
    m_runPluginsInProcess = (state == Qt::Unchecked);
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::networkPermissionChanged(int state)
{
    m_networkPermission = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::retinaChanged(int state)
{
    m_retina = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
    // Does not require a restart
}

void
PreferencesDialog::showSplashChanged(int state)
{
    m_showSplash = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::defaultTemplateChanged(int i)
{
    m_currentTemplate = m_templates[i];
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::localeChanged(int i)
{
    m_currentLocale = m_locales[i];
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::tempDirRootChanged(QString r)
{
    m_tempDirRoot = r;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::tempDirButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory
        (this, tr("Select a directory to create cache subdirectory in"),
         m_tempDirRoot);
    if (dir == "") return;
    m_tempDirRootEdit->setText(dir);
    tempDirRootChanged(dir);
    m_changesOnRestart = true;
}

void
PreferencesDialog::backgroundModeChanged(int mode)
{
    m_backgroundMode = mode;
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::timeToTextModeChanged(int mode)
{
    m_timeToTextMode = mode;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::showHMSChanged(int state)
{
    m_showHMS = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::octaveSystemChanged(int system)
{
    m_octaveSystem = system;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::viewFontSizeChanged(int sz)
{
    m_viewFontSize = sz;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::pluginPathsChanged()
{
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::okClicked()
{
    applyClicked();
    accept();
}

void
PreferencesDialog::applyClicked()
{
    Preferences *prefs = Preferences::getInstance();
    prefs->setWindowType(WindowType(m_windowType));
    prefs->setSpectrogramSmoothing(Preferences::SpectrogramSmoothing
                                   (m_spectrogramSmoothing));
    prefs->setSpectrogramXSmoothing(Preferences::SpectrogramXSmoothing
                                   (m_spectrogramXSmoothing));
    prefs->setPropertyBoxLayout(Preferences::PropertyBoxLayout
                                (m_propertyLayout));
    prefs->setTuningFrequency(m_tuningFrequency);
    prefs->setResampleOnLoad(m_resampleOnLoad);
    prefs->setUseGaplessMode(m_gapless);
    prefs->setRunPluginsInProcess(m_runPluginsInProcess);
    prefs->setShowSplash(m_showSplash);
    prefs->setTemporaryDirectoryRoot(m_tempDirRoot);
    prefs->setBackgroundMode(Preferences::BackgroundMode(m_backgroundMode));
    prefs->setTimeToTextMode(Preferences::TimeToTextMode(m_timeToTextMode));
    prefs->setShowHMS(m_showHMS);
    prefs->setViewFontSize(m_viewFontSize);
    
    prefs->setProperty("Octave Numbering System", m_octaveSystem);

    QSettings settings;
    settings.beginGroup("Preferences");
    QString permishTag = QString("network-permission-%1").arg(SV_VERSION);
    settings.setValue(permishTag, m_networkPermission);

    vector<string> names = breakfastquay::AudioFactory::getImplementationNames();
    string implementationName;
    if (m_audioImplementation > int(names.size())) {
        m_audioImplementation = 0;
    }
    if (m_audioImplementation > 0) {
        implementationName = names[m_audioImplementation-1];
    }
    settings.setValue("audio-target", implementationName.c_str());

    QString suffix;
    if (implementationName != "") {
        suffix = "-" + QString(implementationName.c_str());
    }
    
    names = breakfastquay::AudioFactory::getPlaybackDeviceNames(implementationName);
    string deviceName;
    if (m_audioPlaybackDevice > int(names.size())) {
        m_audioPlaybackDevice = 0;
    }
    if (m_audioPlaybackDevice > 0) {
        deviceName = names[m_audioPlaybackDevice-1];
    }
    settings.setValue("audio-playback-device" + suffix, deviceName.c_str());

    names = breakfastquay::AudioFactory::getRecordDeviceNames(implementationName);
    deviceName = "";
    if (m_audioRecordDevice > int(names.size())) {
        m_audioRecordDevice = 0;
    }
    if (m_audioRecordDevice > 0) {
        deviceName = names[m_audioRecordDevice-1];
    }
    settings.setValue("audio-record-device" + suffix, deviceName.c_str());
    
    settings.setValue("locale", m_currentLocale);
#ifdef Q_OS_MAC
    settings.setValue("scaledHiDpi", m_retina);
#endif
    settings.setValue("spectrogram-colour", m_spectrogramGColour);
    settings.setValue("spectrogram-melodic-colour", m_spectrogramMColour);
    settings.setValue("colour-3d-plot-colour", m_colour3DColour);
    settings.setValue("overview-colour", m_overviewColour.name());
    settings.endGroup();

    settings.beginGroup("MainWindow");
    settings.setValue("sessiontemplate", m_currentTemplate);
    settings.endGroup();

    m_applyButton->setEnabled(false);

    if (m_changesOnRestart) {
        QMessageBox::information(this, tr("Preferences"),
                                 tr("<b>Restart required</b><p>One or more of the application preferences you have changed may not take full effect until Sonic Visualiser is restarted.</p><p>Please exit and restart the application now if you want these changes to take effect immediately.</p>"));
        m_changesOnRestart = false;
    }

    if (m_audioDeviceChanged) {
        emit audioDeviceChanged();
        m_audioDeviceChanged = false;
    }

    if (m_coloursChanged) {
        emit coloursChanged();
        m_coloursChanged = false;
    }

    PluginPathSetter::savePathSettings(m_pluginPathConfigurator->getPaths());
}    

void
PreferencesDialog::cancelClicked()
{
    reject();
}

void
PreferencesDialog::applicationClosing(bool quickly)
{
    if (quickly) {
        reject();
        return;
    }

    if (m_applyButton->isEnabled()) {
        int rv = QMessageBox::warning
            (this, tr("Preferences Changed"),
             tr("Some preferences have been changed but not applied.\n"
                "Apply them before closing?"),
             QMessageBox::Apply | QMessageBox::Discard,
             QMessageBox::Discard);
        if (rv == QMessageBox::Apply) {
            applyClicked();
            accept();
        } else {
            reject();
        }
    } else {
        accept();
    }
}

