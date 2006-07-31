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

#ifdef HAVE_JACK

#include "AudioJACKTarget.h"
#include "AudioCallbackPlaySource.h"

#include <iostream>
#include <cmath>

//#define DEBUG_AUDIO_JACK_TARGET 1

AudioJACKTarget::AudioJACKTarget(AudioCallbackPlaySource *source) :
    AudioCallbackPlayTarget(source),
    m_client(0),
    m_bufferSize(0),
    m_sampleRate(0)
{
    char name[20];
    strcpy(name, "Sonic Visualiser");
    m_client = jack_client_new(name);

    if (!m_client) {
	sprintf(name, "Sonic Visualiser (%d)", (int)getpid());
	m_client = jack_client_new(name);
	if (!m_client) {
	    std::cerr
		<< "ERROR: AudioJACKTarget: Failed to connect to JACK server"
		<< std::endl;
	}
    }

    if (!m_client) return;

    m_bufferSize = jack_get_buffer_size(m_client);
    m_sampleRate = jack_get_sample_rate(m_client);

    jack_set_process_callback(m_client, processStatic, this);

    if (jack_activate(m_client)) {
	std::cerr << "ERROR: AudioJACKTarget: Failed to activate JACK client"
		  << std::endl;
    }

    if (m_source) {
	sourceModelReplaced();
    }
}

AudioJACKTarget::~AudioJACKTarget()
{
    if (m_client) {
	jack_deactivate(m_client);
	jack_client_close(m_client);
    }
}

bool
AudioJACKTarget::isOK() const
{
    return (m_client != 0);
}

int
AudioJACKTarget::processStatic(jack_nframes_t nframes, void *arg)
{
    return ((AudioJACKTarget *)arg)->process(nframes);
}

void
AudioJACKTarget::sourceModelReplaced()
{
    m_mutex.lock();

    m_source->setTargetBlockSize(m_bufferSize);
    m_source->setTargetSampleRate(m_sampleRate);

    size_t channels = m_source->getSourceChannelCount();

    // Because we offer pan, we always want at least 2 channels
    if (channels < 2) channels = 2;

    if (channels == m_outputs.size() || !m_client) {
	m_mutex.unlock();
	return;
    }

    const char **ports =
	jack_get_ports(m_client, NULL, NULL,
		       JackPortIsPhysical | JackPortIsInput);
    size_t physicalPortCount = 0;
    while (ports[physicalPortCount]) ++physicalPortCount;

#ifdef DEBUG_AUDIO_JACK_TARGET    
    std::cerr << "AudioJACKTarget::sourceModelReplaced: have " << channels << " channels and " << physicalPortCount << " physical ports" << std::endl;
#endif

    while (m_outputs.size() < channels) {
	
	char name[20];
	jack_port_t *port;

	sprintf(name, "out %d", m_outputs.size() + 1);

	port = jack_port_register(m_client,
				  name,
				  JACK_DEFAULT_AUDIO_TYPE,
				  JackPortIsOutput,
				  0);

	if (!port) {
	    std::cerr
		<< "ERROR: AudioJACKTarget: Failed to create JACK output port "
		<< m_outputs.size() << std::endl;
	} else {
	    m_source->setTargetPlayLatency(jack_port_get_latency(port));
	}

	if (m_outputs.size() < physicalPortCount) {
	    jack_connect(m_client, jack_port_name(port), ports[m_outputs.size()]);
	}

	m_outputs.push_back(port);
    }

    while (m_outputs.size() > channels) {
	std::vector<jack_port_t *>::iterator itr = m_outputs.end();
	--itr;
	jack_port_t *port = *itr;
	if (port) jack_port_unregister(m_client, port);
	m_outputs.erase(itr);
    }

    m_mutex.unlock();
}

int
AudioJACKTarget::process(jack_nframes_t nframes)
{
    if (!m_mutex.tryLock()) {
	return 0;
    }

    if (m_outputs.empty()) {
	m_mutex.unlock();
	return 0;
    }

#ifdef DEBUG_AUDIO_JACK_TARGET    
    std::cout << "AudioJACKTarget::process(" << nframes << "): have a source" << std::endl;
#endif

#ifdef DEBUG_AUDIO_JACK_TARGET    
    if (m_bufferSize != nframes) {
	std::cerr << "WARNING: m_bufferSize != nframes (" << m_bufferSize << " != " << nframes << ")" << std::endl;
    }
#endif

    float **buffers = (float **)alloca(m_outputs.size() * sizeof(float *));

    for (size_t ch = 0; ch < m_outputs.size(); ++ch) {
	buffers[ch] = (float *)jack_port_get_buffer(m_outputs[ch], nframes);
    }

    if (m_source) {
	m_source->getSourceSamples(nframes, buffers);
    } else {
	for (size_t ch = 0; ch < m_outputs.size(); ++ch) {
	    for (size_t i = 0; i < nframes; ++i) {
		buffers[ch][i] = 0.0;
	    }
	}
    }

    float peakLeft = 0.0, peakRight = 0.0;

    for (size_t ch = 0; ch < m_outputs.size(); ++ch) {

	float peak = 0.0;

	for (size_t i = 0; i < nframes; ++i) {
	    buffers[ch][i] *= m_outputGain;
	    float sample = fabsf(buffers[ch][i]);
	    if (sample > peak) peak = sample;
	}

	if (ch == 0) peakLeft = peak;
	if (ch > 0 || m_outputs.size() == 1) peakRight = peak;
    }
	    
    if (m_source) {
	m_source->setOutputLevels(peakLeft, peakRight);
    }

    m_mutex.unlock();
    return 0;
}


#ifdef INCLUDE_MOCFILES
#include "AudioJACKTarget.moc.cpp"
#endif

#endif /* HAVE_JACK */

