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

#ifndef _AUDIO_PORT_AUDIO_TARGET_H_
#define _AUDIO_PORT_AUDIO_TARGET_H_

#ifdef HAVE_PORTAUDIO

// This code can be compiled for either PortAudio v18 or v19.
// PortAudio v19 is the default.  If you want to use v18, define
// the preprocessor symbol HAVE_PORTAUDIO_v18.

#include <portaudio.h>
#include <vector>

#include "AudioCallbackPlayTarget.h"

class AudioCallbackPlaySource;

class AudioPortAudioTarget : public AudioCallbackPlayTarget
{
    Q_OBJECT

public:
    AudioPortAudioTarget(AudioCallbackPlaySource *source);
    virtual ~AudioPortAudioTarget();

    virtual bool isOK() const;

public slots:
    virtual void sourceModelReplaced();

protected:
#ifdef HAVE_PORTAUDIO_V18

    int process(void *input, void *output, unsigned long frames,
		PaTimestamp outTime);

    static int processStatic(void *, void *, unsigned long,
			     PaTimestamp, void *);

    PortAudioStream *m_stream;

#else

    int process(const void *input, void *output, unsigned long frames,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags);

    static int processStatic(const void *, void *, unsigned long,
                             const PaStreamCallbackTimeInfo *,
                             PaStreamCallbackFlags, void *);

    PaStream *m_stream;

#endif

    int m_bufferSize;
    int m_sampleRate;
    int m_latency;
};

#endif /* HAVE_PORTAUDIO */

#endif

