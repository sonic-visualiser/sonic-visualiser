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

#include "OSCMessage.h"


OSCMessage::~OSCMessage()
{
    clearArgs();
}

void
OSCMessage::clearArgs()
{
    m_args.clear();
}

void
OSCMessage::addArg(QVariant arg)
{
    m_args.push_back(arg);
}

size_t
OSCMessage::getArgCount() const
{
    return m_args.size();
}

const QVariant &
OSCMessage::getArg(size_t i) const
{
    return m_args[i];
}

