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

#ifndef _AUDIO_CORE_AUDIO_TARGET_H_
#define _AUDIO_CORE_AUDIO_TARGET_H_

#ifdef HAVE_COREAUDIO

#include <jack/jack.h>
#include <vector>

#include <CoreAudio/CoreAudio.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioUnit/AudioUnitParameters.h>
#include <AudioUnit/AudioOutputUnit.h>

#include "AudioCallbackPlayTarget.h"

class AudioCallbackPlaySource;

class AudioCoreAudioTarget : public AudioCallbackPlayTarget
{
    Q_OBJECT

public:
    AudioCoreAudioTarget(AudioCallbackPlaySource *source);
    ~AudioCoreAudioTarget();

    virtual bool isOK() const;

public slots:
    virtual void sourceModelReplaced();

protected:
    OSStatus process(void *data,
		     AudioUnitRenderActionFlags *flags,
		     const AudioTimeStamp *timestamp,
		     unsigned int inbus,
		     unsigned int inframes,
		     AudioBufferList *ioData);

    int m_bufferSize;
    int m_sampleRate;
    int m_latency;
};

#endif /* HAVE_COREAUDIO */

#endif

