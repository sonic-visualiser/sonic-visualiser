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

#ifdef HAVE_PORTAUDIO

#include "AudioPortAudioTarget.h"
#include "AudioCallbackPlaySource.h"

#include <iostream>
#include <cassert>
#include <cmath>

//#define DEBUG_AUDIO_PORT_AUDIO_TARGET 1

AudioPortAudioTarget::AudioPortAudioTarget(AudioCallbackPlaySource *source) :
    AudioCallbackPlayTarget(source),
    m_stream(0),
    m_bufferSize(0),
    m_sampleRate(0),
    m_latency(0)
{
    PaError err;

#ifdef DEBUG_AUDIO_PORT_AUDIO_TARGET
#ifdef HAVE_PORTAUDIO_v18
    std::cerr << "AudioPortAudioTarget: Initialising for PortAudio v18" << std::endl;
#else
    std::cerr << "AudioPortAudioTarget: Initialising for PortAudio v19" << std::endl;
#endif
#endif

    err = Pa_Initialize();
    if (err != paNoError) {
	std::cerr << "ERROR: AudioPortAudioTarget: Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
	return;
    }

    m_bufferSize = 1024;
    m_sampleRate = 44100;
    if (m_source && (m_source->getSourceSampleRate() != 0)) {
	m_sampleRate = m_source->getSourceSampleRate();
    }

#ifdef HAVE_PORTAUDIO_v18
    m_latency = Pa_GetMinNumBuffers(m_bufferSize, m_sampleRate) * m_bufferSize;
#endif

#ifdef HAVE_PORTAUDIO_v18
    err = Pa_OpenDefaultStream(&m_stream, 0, 2, paFloat32,
			       m_sampleRate, m_bufferSize, 0,
			       processStatic, this);
#else
    err = Pa_OpenDefaultStream(&m_stream, 0, 2, paFloat32,
			       m_sampleRate, m_bufferSize,
			       processStatic, this);
#endif    

    if (err != paNoError) {
	std::cerr << "ERROR: AudioPortAudioTarget: Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
	m_stream = 0;
	Pa_Terminate();
	return;
    }

#ifndef HAVE_PORTAUDIO_v18
    const PaStreamInfo *info = Pa_GetStreamInfo(m_stream);
    m_latency = int(info->outputLatency * m_sampleRate + 0.001);
#endif

    std::cerr << "PortAudio latency = " << m_latency << " frames" << std::endl;

    err = Pa_StartStream(m_stream);

    if (err != paNoError) {
	std::cerr << "ERROR: AudioPortAudioTarget: Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
	Pa_CloseStream(m_stream);
	m_stream = 0;
	Pa_Terminate();
	return;
    }

    if (m_source) {
	std::cerr << "AudioPortAudioTarget: block size " << m_bufferSize << std::endl;
	m_source->setTargetBlockSize(m_bufferSize);
	m_source->setTargetSampleRate(m_sampleRate);
	m_source->setTargetPlayLatency(m_latency);
    }
}

AudioPortAudioTarget::~AudioPortAudioTarget()
{
    if (m_stream) {
	PaError err;
	err = Pa_CloseStream(m_stream);
	if (err != paNoError) {
	    std::cerr << "ERROR: AudioPortAudioTarget: Failed to close PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
	}
	err = Pa_Terminate();
        if (err != paNoError) {
            std::cerr << "ERROR: AudioPortAudioTarget: Failed to terminate PortAudio: " << Pa_GetErrorText(err) << std::endl;
	}   
    }
}

bool
AudioPortAudioTarget::isOK() const
{
    return (m_stream != 0);
}

#ifdef HAVE_PORTAUDIO_v18
int
AudioPortAudioTarget::processStatic(void *input, void *output,
				    unsigned long nframes,
				    PaTimestamp outTime, void *data)
{
    return ((AudioPortAudioTarget *)data)->process(input, output,
						   nframes, outTime);
}
#else
int
AudioPortAudioTarget::processStatic(const void *input, void *output,
                                    unsigned long nframes,
                                    const PaStreamCallbackTimeInfo *timeInfo,
                                    PaStreamCallbackFlags flags, void *data)
{
    return ((AudioPortAudioTarget *)data)->process(input, output,
                                                   nframes, timeInfo,
                                                   flags);
}
#endif

void
AudioPortAudioTarget::sourceModelReplaced()
{
    m_source->setTargetSampleRate(m_sampleRate);
}

#ifdef HAVE_PORTAUDIO_v18
int
AudioPortAudioTarget::process(void *inputBuffer, void *outputBuffer,
			      unsigned long nframes,
			      PaTimestamp)
#else
int
AudioPortAudioTarget::process(const void *inputBuffer, void *outputBuffer,
                              unsigned long nframes,
                              const PaStreamCallbackTimeInfo *,
                              PaStreamCallbackFlags)
#endif
{
#ifdef DEBUG_AUDIO_PORT_AUDIO_TARGET    
    std::cout << "AudioPortAudioTarget::process(" << nframes << ")" << std::endl;
#endif

    if (!m_source) return 0;

    float *output = (float *)outputBuffer;

    assert(nframes <= m_bufferSize);

    static float **tmpbuf = 0;
    static size_t tmpbufch = 0;
    static size_t tmpbufsz = 0;

    size_t sourceChannels = m_source->getSourceChannelCount();

    // Because we offer pan, we always want at least 2 channels
    if (sourceChannels < 2) sourceChannels = 2;

    if (!tmpbuf || tmpbufch != sourceChannels || tmpbufsz < m_bufferSize) {

	if (tmpbuf) {
	    for (size_t i = 0; i < tmpbufch; ++i) {
		delete[] tmpbuf[i];
	    }
	    delete[] tmpbuf;
	}

	tmpbufch = sourceChannels;
	tmpbufsz = m_bufferSize;
	tmpbuf = new float *[tmpbufch];

	for (size_t i = 0; i < tmpbufch; ++i) {
	    tmpbuf[i] = new float[tmpbufsz];
	}
    }
	
    m_source->getSourceSamples(nframes, tmpbuf);

    float peakLeft = 0.0, peakRight = 0.0;

    for (size_t ch = 0; ch < 2; ++ch) {
	
	float peak = 0.0;

	if (ch < sourceChannels) {

	    // PortAudio samples are interleaved
	    for (size_t i = 0; i < nframes; ++i) {
		output[i * 2 + ch] = tmpbuf[ch][i] * m_outputGain;
		float sample = fabsf(output[i * 2 + ch]);
		if (sample > peak) peak = sample;
	    }

	} else if (ch == 1 && sourceChannels == 1) {

	    for (size_t i = 0; i < nframes; ++i) {
		output[i * 2 + ch] = tmpbuf[0][i] * m_outputGain;
		float sample = fabsf(output[i * 2 + ch]);
		if (sample > peak) peak = sample;
	    }

	} else {
	    for (size_t i = 0; i < nframes; ++i) {
		output[i * 2 + ch] = 0;
	    }
	}

	if (ch == 0) peakLeft = peak;
	if (ch > 0 || sourceChannels == 1) peakRight = peak;
    }

    m_source->setOutputLevels(peakLeft, peakRight);

    return 0;
}

#ifdef INCLUDE_MOCFILES
#include "AudioPortAudioTarget.moc.cpp"
#endif

#endif /* HAVE_PORTAUDIO */

