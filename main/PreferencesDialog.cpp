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
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QString>

#include <fftw3.h>

#include "base/Preferences.h"
#include "base/ConfigFile.h"

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WFlags flags) :
    QDialog(parent, flags)
{
    setWindowTitle(tr("Application Preferences"));

    Preferences *prefs = Preferences::getInstance();

    QGridLayout *grid = new QGridLayout;
    setLayout(grid);
    
    QGroupBox *groupBox = new QGroupBox;
    groupBox->setTitle(tr("Sonic Visualiser Application Preferences"));
    grid->addWidget(groupBox, 0, 0);
    
    QGridLayout *subgrid = new QGridLayout;
    groupBox->setLayout(subgrid);

    // Create this first, as slots that get called from the ctor will
    // refer to it
    m_applyButton = new QPushButton(tr("Apply"));

    // The WindowType enum is in rather a ragbag order -- reorder it here
    // in a more sensible order
    m_windows = new WindowType[9];
    m_windows[0] = HanningWindow;
    m_windows[1] = HammingWindow;
    m_windows[2] = BlackmanWindow;
    m_windows[3] = BlackmanHarrisWindow;
    m_windows[4] = NuttallWindow;
    m_windows[5] = GaussianWindow;
    m_windows[6] = ParzenWindow;
    m_windows[7] = BartlettWindow;
    m_windows[8] = RectangularWindow;

    QComboBox *windowCombo = new QComboBox;
    int min, max, i;
    int window = prefs->getPropertyRangeAndValue("Window Type", &min, &max);
    m_windowType = window;
    int index = 0;
    
    for (i = 0; i <= 8; ++i) {
        windowCombo->addItem(prefs->getPropertyValueLabel("Window Type",
                                                          m_windows[i]));
        if (m_windows[i] == window) index = i;
    }

    windowCombo->setCurrentIndex(index);

    m_windowTimeExampleLabel = new QLabel;
    m_windowFreqExampleLabel = new QLabel;

    connect(windowCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(windowTypeChanged(int)));
    windowTypeChanged(index);

    QCheckBox *smoothing = new QCheckBox;
    m_smoothSpectrogram = prefs->getSmoothSpectrogram();
    smoothing->setCheckState(m_smoothSpectrogram ?
                             Qt::Checked : Qt::Unchecked);

    connect(smoothing, SIGNAL(stateChanged(int)),
            this, SLOT(smoothSpectrogramChanged(int)));

    QComboBox *propertyLayout = new QComboBox;
    int pl = prefs->getPropertyRangeAndValue("Property Box Layout", &min, &max);
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

    int row = 0;

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Property Box Layout"))),
                       row, 0);
    subgrid->addWidget(propertyLayout, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Tuning Frequency"))),
                       row, 0);
    subgrid->addWidget(frequency, row++, 1, 1, 2);

    subgrid->addWidget(new QLabel(prefs->getPropertyLabel
                                  ("Smooth Spectrogram")),
                       row, 0, 1, 2);
    subgrid->addWidget(smoothing, row++, 2);

    subgrid->addWidget(new QLabel(tr("%1:").arg(prefs->getPropertyLabel
                                                ("Window Type"))),
                       row, 0);
    subgrid->addWidget(windowCombo, row++, 1, 1, 2);

    subgrid->addWidget(m_windowTimeExampleLabel, row, 1);
    subgrid->addWidget(m_windowFreqExampleLabel, row, 2);
    
    QHBoxLayout *hbox = new QHBoxLayout;
    grid->addLayout(hbox, 1, 0);
    
    QPushButton *ok = new QPushButton(tr("OK"));
    QPushButton *cancel = new QPushButton(tr("Cancel"));
    hbox->addStretch(10);
    hbox->addWidget(ok);
    hbox->addWidget(m_applyButton);
    hbox->addWidget(cancel);
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(m_applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    m_applyButton->setEnabled(false);
}

PreferencesDialog::~PreferencesDialog()
{
    std::cerr << "PreferencesDialog::~PreferencesDialog()" << std::endl;

    delete[] m_windows;
}

void
PreferencesDialog::windowTypeChanged(int value)
{
    int step = 24;
    int peak = 48;
    int w = step * 4, h = 64;
    WindowType type = m_windows[value];
    Window<float> windower = Window<float>(type, step * 2);

    QPixmap timeLabel(w, h + 1);
    timeLabel.fill(Qt::white);
    QPainter timePainter(&timeLabel);

    QPainterPath path;

    path.moveTo(0, h - peak + 1);
    path.lineTo(w, h - peak + 1);

    timePainter.setPen(Qt::gray);
    timePainter.setRenderHint(QPainter::Antialiasing, true);
    timePainter.drawPath(path);
    
    path = QPainterPath();

    float acc[w];
    for (int i = 0; i < w; ++i) acc[i] = 0.f;
    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < step * 2; ++i) {
            acc[j * step + i] += windower.getValue(i);
        }
    }
    for (int i = 0; i < w; ++i) {
        int y = h - int(peak * acc[i] + 0.001) + 1;
        if (i == 0) path.moveTo(i, y);
        else path.lineTo(i, y);
    }

    timePainter.drawPath(path);
    timePainter.setRenderHint(QPainter::Antialiasing, false);

    path = QPainterPath();

    timePainter.setPen(Qt::black);
    
    for (int i = 0; i < step * 2; ++i) {
        int y = h - int(peak * windower.getValue(i) + 0.001) + 1;
        if (i == 0) path.moveTo(i + step, float(y));
        else path.lineTo(i + step, float(y));
    }

    if (type == RectangularWindow) {
        timePainter.drawPath(path);
        path = QPainterPath();
    }

    timePainter.setRenderHint(QPainter::Antialiasing, true);
    path.addRect(0, 0, w, h + 1);
    timePainter.drawPath(path);

    QFont font;
    font.setPixelSize(10);
    font.setItalic(true);
    timePainter.setFont(font);
    QString label = tr("V / time");
    timePainter.drawText(w - timePainter.fontMetrics().width(label) - 4,
                         timePainter.fontMetrics().ascent() + 1, label);

    m_windowTimeExampleLabel->setPixmap(timeLabel);
    
    int fw = 100;

    QPixmap freqLabel(fw, h + 1);
    freqLabel.fill(Qt::white);
    QPainter freqPainter(&freqLabel);
    path = QPainterPath();

    size_t fftsize = 512;

    float *input = (float *)fftwf_malloc(fftsize * sizeof(float));
    fftwf_complex *output =
        (fftwf_complex *)fftwf_malloc(fftsize * sizeof(fftwf_complex));
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(fftsize, input, output,
                                            FFTW_ESTIMATE);
    for (int i = 0; i < fftsize; ++i) input[i] = 0.f;
    for (int i = 0; i < step * 2; ++i) {
        input[fftsize/2 - step + i] = windower.getValue(i);
    }
    
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);

    float maxdb = 0.f;
    float mindb = 0.f;
    bool first = true;
    for (int i = 0; i < fftsize/2; ++i) {
        float power = output[i][0] * output[i][0] + output[i][1] * output[i][1];
        float db = mindb;
        if (power > 0) {
            db = 20 * log10(power);
            if (first || db > maxdb) maxdb = db;
            if (first || db < mindb) mindb = db;
            first = false;
        }
    }

    if (mindb > -80.f) mindb = -80.f;

    // -- no, don't use the actual mindb -- it's easier to compare
    // plots with a fixed min value
    mindb = -170.f;

    float maxval = maxdb + -mindb;

    float ly = h - ((-80.f + -mindb) / maxval) * peak + 1;

    path.moveTo(0, h - peak + 1);
    path.lineTo(fw, h - peak + 1);

    freqPainter.setPen(Qt::gray);
    freqPainter.setRenderHint(QPainter::Antialiasing, true);
    freqPainter.drawPath(path);
    
    path = QPainterPath();
    freqPainter.setPen(Qt::black);

//    std::cerr << "maxdb = " << maxdb << ", mindb = " << mindb << ", maxval = " <<maxval << std::endl;

    for (int i = 0; i < fftsize/2; ++i) {
        float power = output[i][0] * output[i][0] + output[i][1] * output[i][1];
        float db = 20 * log10(power);
        float val = db + -mindb;
        if (val < 0) val = 0;
        float norm = val / maxval;
        float x = (fw / float(fftsize/2)) * i;
        float y = h - norm * peak + 1;
        if (i == 0) path.moveTo(x, y);
        else path.lineTo(x, y);
    }

    freqPainter.setRenderHint(QPainter::Antialiasing, true);
    path.addRect(0, 0, fw, h + 1);
    freqPainter.drawPath(path);

    fftwf_free(input);
    fftwf_free(output);

    freqPainter.setFont(font);
    label = tr("dB / freq");
    freqPainter.drawText(fw - freqPainter.fontMetrics().width(label) - 4,
                         freqPainter.fontMetrics().ascent() + 1, label);

    m_windowFreqExampleLabel->setPixmap(freqLabel);

    m_windowType = type;
    m_applyButton->setEnabled(true);
}

void
PreferencesDialog::smoothSpectrogramChanged(int state)
{
    m_smoothSpectrogram = (state == Qt::Checked);
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
PreferencesDialog::okClicked()
{
    applyClicked();
    Preferences::getInstance()->getConfigFile()->commit();
    accept();
}

void
PreferencesDialog::applyClicked()
{
    Preferences *prefs = Preferences::getInstance();
    prefs->setWindowType(WindowType(m_windowType));
    prefs->setSmoothSpectrogram(m_smoothSpectrogram);
    prefs->setPropertyBoxLayout(Preferences::PropertyBoxLayout
                                (m_propertyLayout));
    prefs->setTuningFrequency(m_tuningFrequency);
    m_applyButton->setEnabled(false);
}    

void
PreferencesDialog::cancelClicked()
{
    reject();
}

