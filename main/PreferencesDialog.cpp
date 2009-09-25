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
#include <QMessageBox>
#include <QSpinBox>
#include <QSettings>

#include "widgets/WindowTypeSelector.h"
#include "widgets/IconLoader.h"
#include "base/Preferences.h"
#include "audioio/AudioTargetFactory.h"

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WFlags flags) :
    QDialog(parent, flags),
    m_audioDevice(0),
    m_changesOnRestart(false)
{
    setWindowTitle(tr("Sonic Visualiser: Application Preferences"));

    Preferences *prefs = Preferences::getInstance();

    QGridLayout *grid = new QGridLayout;
    setLayout(grid);

    QTabWidget *tab = new QTabWidget;
    grid->addWidget(tab, 0, 0);
    
    tab->setTabPosition(QTabWidget::North);

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

    QComboBox *audioDevice = new QComboBox;
    std::vector<QString> devices =
        AudioTargetFactory::getInstance()->getCallbackTargetNames();
    
    QSettings settings;
    settings.beginGroup("Preferences");
    QString targetName = settings.value("audio-target", "").toString();
    settings.endGroup();

    for (int i = 0; i < devices.size(); ++i) {
        audioDevice->addItem(AudioTargetFactory::getInstance()
                             ->getCallbackTargetDescription(devices[i]));
        if (targetName == devices[i]) audioDevice->setCurrentIndex(i);
    }

    connect(audioDevice, SIGNAL(currentIndexChanged(int)),
            this, SLOT(audioDeviceChanged(int)));

    QComboBox *resampleQuality = new QComboBox;

    int rsq = prefs->getPropertyRangeAndValue("Resample Quality", &min, &max,
                                              &deflt);
    m_resampleQuality = rsq;

    for (i = min; i <= max; ++i) {
        resampleQuality->addItem(prefs->getPropertyValueLabel("Resample Quality", i));
    }

    resampleQuality->setCurrentIndex(rsq);

    connect(resampleQuality, SIGNAL(currentIndexChanged(int)),
            this, SLOT(resampleQualityChanged(int)));

    QCheckBox *resampleOnLoad = new QCheckBox;
    m_resampleOnLoad = prefs->getResampleOnLoad();
    resampleOnLoad->setCheckState(m_resampleOnLoad ? Qt::Checked :
                                  Qt::Unchecked);
    connect(resampleOnLoad, SIGNAL(stateChanged(int)),
            this, SLOT(resampleOnLoadChanged(int)));

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
    tempDirButton->setFixedSize(QSize(24, 24));

    QCheckBox *showSplash = new QCheckBox;
    m_showSplash = prefs->getShowSplash();
    showSplash->setCheckState(m_showSplash ? Qt::Checked : Qt::Unchecked);
    connect(showSplash, SIGNAL(stateChanged(int)),
            this, SLOT(showSplashChanged(int)));

#ifndef Q_WS_MAC
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

    // General tab

    QFrame *frame = new QFrame;
    
    QGridLayout *subgrid = new QGridLayout;
    frame->setLayout(subgrid);

    int row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Temporary Directory Root"))),
                       row, 0);
    subgrid->addWidget(m_tempDirRootEdit, row, 1, 1, 1);
    subgrid->addWidget(tempDirButton, row, 2, 1, 1);
    row++;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Resample On Load"))),
                       row, 0);
    subgrid->addWidget(resampleOnLoad, row++, 1, 1, 1);

    subgrid->addWidget(new QLabel(tr("Playback audio device:")), row, 0);
    subgrid->addWidget(audioDevice, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Resample Quality"))),
                       row, 0);
    subgrid->addWidget(resampleQuality, row++, 1, 1, 2);

    subgrid->setRowStretch(row, 10);
    
    tab->addTab(frame, tr("&General"));

    // Appearance tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Property Box Layout"))),
                       row, 0);
    subgrid->addWidget(propertyLayout, row++, 1, 1, 2);

#ifndef Q_WS_MAC
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
                                                ("Show Splash Screen"))),
                       row, 0);
    subgrid->addWidget(showSplash, row++, 1, 1, 1);

    subgrid->setRowStretch(row, 10);
    
    tab->addTab(frame, tr("&Appearance"));

    // Analysis tab

    frame = new QFrame;
    subgrid = new QGridLayout;
    frame->setLayout(subgrid);
    row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Tuning Frequency"))),
                       row, 0);
    subgrid->addWidget(frequency, row++, 1, 1, 2);

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
    
    subgrid->setRowStretch(row, 10);
    
    tab->addTab(frame, tr("Anal&ysis"));

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
    std::cerr << "PreferencesDialog::~PreferencesDialog()" << std::endl;
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
PreferencesDialog::audioDeviceChanged(int s)
{
    m_audioDevice = s;
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::resampleQualityChanged(int q)
{
    m_resampleQuality = q;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::resampleOnLoadChanged(int state)
{
    m_resampleOnLoad = (state == Qt::Checked);
    m_applyButton->setEnabled(true);
    m_changesOnRestart = true;
}

void
PreferencesDialog::showSplashChanged(int state)
{
    m_showSplash = (state == Qt::Checked);
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
PreferencesDialog::viewFontSizeChanged(int sz)
{
    m_viewFontSize = sz;
    m_applyButton->setEnabled(true);
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
    prefs->setResampleQuality(m_resampleQuality);
    prefs->setResampleOnLoad(m_resampleOnLoad);
    prefs->setShowSplash(m_showSplash);
    prefs->setTemporaryDirectoryRoot(m_tempDirRoot);
    prefs->setBackgroundMode(Preferences::BackgroundMode(m_backgroundMode));
    prefs->setTimeToTextMode(Preferences::TimeToTextMode(m_timeToTextMode));
    prefs->setViewFontSize(m_viewFontSize);
    
    std::vector<QString> devices =
        AudioTargetFactory::getInstance()->getCallbackTargetNames();

    QSettings settings;
    settings.beginGroup("Preferences");
    settings.setValue("audio-target", devices[m_audioDevice]);
    settings.endGroup();

    m_applyButton->setEnabled(false);

    if (m_changesOnRestart) {
        QMessageBox::information(this, tr("Preferences"),
                                 tr("<b>Restart required</b><p>One or more of the application preferences you have changed may not take full effect until Sonic Visualiser is restarted.</p><p>Please exit and restart the application now if you want these changes to take effect immediately.</p>"));
        m_changesOnRestart = false;
    }
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

