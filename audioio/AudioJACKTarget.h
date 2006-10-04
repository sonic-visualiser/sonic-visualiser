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

#ifndef _AUDIO_JACK_TARGET_H_
#define _AUDIO_JACK_TARGET_H_

#ifdef HAVE_JACK

#include <jack/jack.h>
#include <vector>

#include "AudioCallbackPlayTarget.h"

#include <QMutex>

class AudioCallbackPlaySource;

class AudioJACKTarget : public AudioCallbackPlayTarget
{
    Q_OBJECT

public:
    AudioJACKTarget(AudioCallbackPlaySource *source);
    virtual ~AudioJACKTarget();

    virtual bool isOK() const;

public slots:
    virtual void sourceModelReplaced();

protected:
    int process(jack_nframes_t nframes);
    int xrun();

    static int processStatic(jack_nframes_t, void *);
    static int xrunStatic(void *);

    jack_client_t              *m_client;
    std::vector<jack_port_t *>  m_outputs;
    jack_nframes_t              m_bufferSize;
    jack_nframes_t              m_sampleRate;
    QMutex                      m_mutex;
};

#endif /* HAVE_JACK */

#endif

