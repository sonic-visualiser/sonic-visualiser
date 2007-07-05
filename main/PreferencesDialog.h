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

#ifndef _PREFERENCES_DIALOG_H_
#define _PREFERENCES_DIALOG_H_

#include <QDialog>

#include "base/Window.h"

class WindowTypeSelector;
class QPushButton;

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
    virtual ~PreferencesDialog();

public slots:
    void applicationClosing(bool quickly);

protected slots:
    void windowTypeChanged(WindowType type);
    void spectrogramSmoothingChanged(int state);
    void propertyLayoutChanged(int layout);
    void tuningFrequencyChanged(double freq);
    void resampleQualityChanged(int quality);

    void okClicked();
    void applyClicked();
    void cancelClicked();

protected:
    WindowTypeSelector *m_windowTypeSelector;
    QPushButton *m_applyButton;
    
    WindowType m_windowType;
    int   m_spectrogramSmoothing;
    int   m_propertyLayout;
    float m_tuningFrequency;
    int   m_resampleQuality;
};

#endif
