/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2006 Chris Cannam.
*/

#ifndef _OSC_MESSAGE_H_
#define _OSC_MESSAGE_H_

#include <QString>
#include <QVariant>

#include <vector>
#include <map>

class OSCMessage
{
public:
    OSCMessage() { }
    ~OSCMessage();

    void setTarget(const int &target) { m_target = target; }
    int getTarget() const { return m_target; }

    void setTargetData(const int &targetData) { m_targetData = targetData; }
    int getTargetData() const { return m_targetData; }

    void setMethod(QString method) { m_method = method; }
    QString getMethod() const { return m_method; }

    void clearArgs();
    void addArg(QVariant arg);

    size_t getArgCount() const;
    const QVariant &getArg(size_t i) const;

private:
    int m_target;
    int m_targetData;
    QString m_method;
    std::vector<QVariant> m_args;
};

#endif
