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
   This file copyright 2000-2006 Chris Cannam and QMUL.
*/

#ifndef _OSC_QUEUE_H_
#define _OSC_QUEUE_H_

#include "OSCMessage.h"

#include "base/RingBuffer.h"

#include <QObject>

#ifdef HAVE_LIBLO
#include <lo/lo.h>
#endif

class OSCQueue : public QObject
{
    Q_OBJECT

public:
    OSCQueue();
    virtual ~OSCQueue();

    bool isOK() const;

    bool isEmpty() const { return getMessagesAvailable() == 0; }
    size_t getMessagesAvailable() const;
    OSCMessage readMessage();

    QString getOSCURL() const;

signals:
    void messagesAvailable();

protected:
#ifdef HAVE_LIBLO
    lo_server_thread m_thread;

    static void oscError(int, const char *, const char *);
    static int oscMessageHandler(const char *, const char *, lo_arg **,
                                 int, lo_message, void *);
#endif

    void postMessage(OSCMessage);
    bool parseOSCPath(QString path, int &target, int &targetData, QString &method);

    RingBuffer<OSCMessage *> m_buffer;
};

#endif

