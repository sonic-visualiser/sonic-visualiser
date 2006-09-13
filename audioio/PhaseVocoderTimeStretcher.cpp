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

#include "PhaseVocoderTimeStretcher.h"

#include <iostream>
#include <cassert>

//#define DEBUG_PHASE_VOCODER_TIME_STRETCHER 1

PhaseVocoderTimeStretcher::PhaseVocoderTimeStretcher(size_t channels,
                                                     float ratio,
                                                     bool sharpen,
                                                     size_t maxProcessInputBlockSize) :
    m_channels(channels),
    m_ratio(ratio),
    m_sharpen(sharpen)
{
    m_wlen = 1024;

    if (ratio < 1) {
        if (ratio < 0.4) {
            m_n1 = 1024;
            m_wlen = 2048;
        } else if (ratio < 0.8) {
            m_n1 = 512;
        } else {
            m_n1 = 256;
        }
        if (m_sharpen) {
            m_n1 /= 2;
            m_wlen = 2048;
        }
        m_n2 = m_n1 * ratio;
    } else {
        if (ratio > 2) {
            m_n2 = 512;
            m_wlen = 4096; 
        } else if (ratio > 1.6) {
            m_n2 = 384;
            m_wlen = 2048;
        } else {
            m_n2 = 256;
        }
        if (m_sharpen) {
            m_n2 /= 2;
            if (m_wlen < 2048) m_wlen = 2048;
        }
        m_n1 = m_n2 / ratio;
    }
        
    m_window = new Window<float>(HanningWindow, m_wlen);

    m_prevPhase = new float *[m_channels];
    m_prevAdjustedPhase = new float *[m_channels];
    if (m_sharpen) m_prevMag = new float *[m_channels];
    else m_prevMag = 0;
    m_prevPercussiveCount = new int[m_channels];

    m_dbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
    m_time = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_wlen);
    m_freq = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_wlen);
        
    m_plan = fftwf_plan_dft_1d(m_wlen, m_time, m_freq, FFTW_FORWARD, FFTW_ESTIMATE);
    m_iplan = fftwf_plan_dft_c2r_1d(m_wlen, m_freq, m_dbuf, FFTW_ESTIMATE);

    m_inbuf = new RingBuffer<float> *[m_channels];
    m_outbuf = new RingBuffer<float> *[m_channels];
    m_mashbuf = new float *[m_channels];

    m_modulationbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        
    for (size_t c = 0; c < m_channels; ++c) {

        m_prevPhase[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        m_prevAdjustedPhase[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);

        if (m_sharpen) {
            m_prevMag[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        }

        m_inbuf[c] = new RingBuffer<float>(m_wlen);
        m_outbuf[c] = new RingBuffer<float>
            (lrintf((maxProcessInputBlockSize + m_wlen) * ratio));
            
        m_mashbuf[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        
        for (int i = 0; i < m_wlen; ++i) {
            m_mashbuf[c][i] = 0.0;
            m_prevPhase[c][i] = 0.0;
            m_prevAdjustedPhase[c][i] = 0.0;
            if (m_sharpen) m_prevMag[c][i] = 0.0;
        }
        
        m_prevPercussiveCount[c] = 0;
    }

    for (int i = 0; i < m_wlen; ++i) {
        m_modulationbuf[i] = 0.0;
    }

    std::cerr << "PhaseVocoderTimeStretcher: channels = " << channels
              << ", ratio = " << ratio
              << ", n1 = " << m_n1 << ", n2 = " << m_n2 << ", wlen = "
              << m_wlen << ", max = " << maxProcessInputBlockSize
              << ", outbuflen = " << m_outbuf[0]->getSize() << std::endl;
}

PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher()
{
    std::cerr << "PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher" << std::endl;

    fftwf_destroy_plan(m_plan);
    fftwf_destroy_plan(m_iplan);

    fftwf_free(m_time);
    fftwf_free(m_freq);
    fftwf_free(m_dbuf);

    for (size_t c = 0; c < m_channels; ++c) {

        fftwf_free(m_mashbuf[c]);
        fftwf_free(m_prevPhase[c]);
        fftwf_free(m_prevAdjustedPhase[c]);
        if (m_sharpen) fftwf_free(m_prevMag[c]);

        delete m_inbuf[c];
        delete m_outbuf[c];
    }

    fftwf_free(m_modulationbuf);

    delete[] m_prevPhase;
    delete[] m_prevAdjustedPhase;
    if (m_sharpen) delete[] m_prevMag;
    delete[] m_prevPercussiveCount;
    delete[] m_inbuf;
    delete[] m_outbuf;
    delete[] m_mashbuf;

    delete m_window;
}	

size_t
PhaseVocoderTimeStretcher::getProcessingLatency() const
{
    return getWindowSize() - getInputIncrement();
}

void
PhaseVocoderTimeStretcher::process(float **input, float **output, size_t samples)
{
    putInput(input, samples);
    getOutput(output, lrintf(samples * m_ratio));
}

size_t
PhaseVocoderTimeStretcher::getRequiredInputSamples() const
{
    if (m_inbuf[0]->getReadSpace() >= m_wlen) return 0;
    return m_wlen - m_inbuf[0]->getReadSpace();
}

void
PhaseVocoderTimeStretcher::putInput(float **input, size_t samples)
{
    // We need to add samples from input to our internal buffer.  When
    // we have m_windowSize samples in the buffer, we can process it,
    // move the samples back by m_n1 and write the output onto our
    // internal output buffer.  If we have (samples * ratio) samples
    // in that, we can write m_n2 of them back to output and return
    // (otherwise we have to write zeroes).

    // When we process, we write m_wlen to our fixed output buffer
    // (m_mashbuf).  We then pull out the first m_n2 samples from that
    // buffer, push them into the output ring buffer, and shift
    // m_mashbuf left by that amount.

    // The processing latency is then m_wlen - m_n2.

    size_t consumed = 0;

    while (consumed < samples) {

	size_t writable = m_inbuf[0]->getWriteSpace();
	writable = std::min(writable, samples - consumed);

	if (writable == 0) {
	    //!!! then what? I don't think this should happen, but
	    std::cerr << "WARNING: PhaseVocoderTimeStretcher::putInput: writable == 0" << std::endl;
	    break;
	}

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "writing " << writable << " from index " << consumed << " to inbuf, consumed will be " << consumed + writable << std::endl;
#endif

        for (size_t c = 0; c < m_channels; ++c) {
            m_inbuf[c]->write(input[c] + consumed, writable);
        }
	consumed += writable;

	while (m_inbuf[0]->getReadSpace() >= m_wlen &&
	       m_outbuf[0]->getWriteSpace() >= m_n2) {

	    // We know we have at least m_wlen samples available
	    // in m_inbuf.  We need to peek m_wlen of them for
	    // processing, and then read m_n1 to advance the read
	    // pointer.
            
            size_t n2 = m_n2;
            bool isPercussive = false;

            for (size_t c = 0; c < m_channels; ++c) {

                size_t got = m_inbuf[c]->peek(m_dbuf, m_wlen);
                assert(got == m_wlen);
		
                bool thisChannelPercussive =
                    processBlock(c, m_dbuf, m_mashbuf[c],
                                 c == 0 ? m_modulationbuf : 0,
                                 isPercussive);

                if (thisChannelPercussive && c == 0) {
                    isPercussive = true;
                } 

                if (isPercussive) {
                    n2 = m_n1;
                }

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
                std::cerr << "writing first " << m_n2 << " from mashbuf, skipping " << m_n1 << " on inbuf " << std::endl;
#endif
                m_inbuf[c]->skip(m_n1);

                for (size_t i = 0; i < n2; ++i) {
                    if (m_modulationbuf[i] > 0.f) {
                        m_mashbuf[c][i] /= m_modulationbuf[i];
                    }
                }

                m_outbuf[c]->write(m_mashbuf[c], n2);

                for (size_t i = 0; i < m_wlen - n2; ++i) {
                    m_mashbuf[c][i] = m_mashbuf[c][i + n2];
                }

                for (size_t i = m_wlen - n2; i < m_wlen; ++i) {
                    m_mashbuf[c][i] = 0.0f;
                }
            }

            for (size_t i = 0; i < m_wlen - n2; ++i) {
                m_modulationbuf[i] = m_modulationbuf[i + n2];
	    }

	    for (size_t i = m_wlen - n2; i < m_wlen; ++i) {
                m_modulationbuf[i] = 0.0f;
	    }
	}


#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "loop ended: inbuf read space " << m_inbuf[0]->getReadSpace() << ", outbuf write space " << m_outbuf[0]->getWriteSpace() << std::endl;
#endif
    }

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::putInput returning" << std::endl;
#endif
}

size_t
PhaseVocoderTimeStretcher::getAvailableOutputSamples() const
{
    return m_outbuf[0]->getReadSpace();
}

void
PhaseVocoderTimeStretcher::getOutput(float **output, size_t samples)
{
    if (m_outbuf[0]->getReadSpace() < samples) {
	std::cerr << "WARNING: PhaseVocoderTimeStretcher::getOutput: not enough data (yet?) (" << m_outbuf[0]->getReadSpace() << " < " << samples << ")" << std::endl;
	size_t fill = samples - m_outbuf[0]->getReadSpace();
        for (size_t c = 0; c < m_channels; ++c) {
            for (size_t i = 0; i < fill; ++i) {
                output[c][i] = 0.0;
            }
            m_outbuf[c]->read(output[c] + fill, m_outbuf[c]->getReadSpace());
        }
    } else {
#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "enough data - writing " << samples << " from outbuf" << std::endl;
#endif
        for (size_t c = 0; c < m_channels; ++c) {
            m_outbuf[c]->read(output[c], samples);
        }
    }

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::getOutput returning" << std::endl;
#endif
}

bool
PhaseVocoderTimeStretcher::processBlock(size_t c,
                                        float *buf, float *out,
                                        float *modulation,
                                        bool knownPercussive)
{
    size_t i;
    bool isPercussive = knownPercussive;

    // buf contains m_wlen samples; out contains enough space for
    // m_wlen * ratio samples (we mix into out, rather than replacing)

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::processBlock (channel " << c << ")" << std::endl;
#endif

    m_window->cut(buf);

    for (i = 0; i < m_wlen/2; ++i) {
	float temp = buf[i];
	buf[i] = buf[i + m_wlen/2];
	buf[i + m_wlen/2] = temp;
    }
    
    for (i = 0; i < m_wlen; ++i) {
	m_time[i][0] = buf[i];
	m_time[i][1] = 0.0;
    }

    fftwf_execute(m_plan); // m_time -> m_freq

    if (m_sharpen && c == 0) { //!!!
        
        int count = 0;

        for (i = 0; i < m_wlen; ++i) {
            
            float mag = sqrtf(m_freq[i][0] * m_freq[i][0] +
                              m_freq[i][1] * m_freq[i][1]);

            if (m_prevMag[c][i] > 0) {
                float magdiff = 20.f * log10f(mag / m_prevMag[c][i]);
                if (magdiff > 3.f) ++count;
            }
            
            m_prevMag[c][i] = mag;
        }
        
        if (count > m_wlen / 6 &&
            count > m_prevPercussiveCount[c] * 1.2) {
            isPercussive = true;
            std::cerr << "isPercussive (count = " << count << ", prev = " << m_prevPercussiveCount[c] << ")" << std::endl;
        }

        m_prevPercussiveCount[c] = count;
    }

    size_t n2 = m_n2;
    if (isPercussive) n2 = m_n1;
	
    for (i = 0; i < m_wlen; ++i) {

        float mag;

        if (m_sharpen && c == 0) {
            mag = m_prevMag[c][i]; // can reuse this
        } else {
            mag = sqrtf(m_freq[i][0] * m_freq[i][0] +
                        m_freq[i][1] * m_freq[i][1]);
        }
		
        float phase = princargf(atan2f(m_freq[i][1], m_freq[i][0]));

        float omega = (2 * M_PI * m_n1 * i) / m_wlen;
	
        float expectedPhase = m_prevPhase[c][i] + omega;

        float phaseError = princargf(phase - expectedPhase);

        float phaseIncrement = (omega + phaseError) / m_n1;

        float adjustedPhase = m_prevAdjustedPhase[c][i] + n2 * phaseIncrement;

        if (isPercussive) adjustedPhase = phase;
	
	float real = mag * cosf(adjustedPhase);
	float imag = mag * sinf(adjustedPhase);
	m_freq[i][0] = real;
	m_freq[i][1] = imag;

        m_prevPhase[c][i] = phase;
        m_prevAdjustedPhase[c][i] = adjustedPhase;
    }
    
    fftwf_execute(m_iplan); // m_freq -> in, inverse fft
    
    for (i = 0; i < m_wlen/2; ++i) {
	float temp = buf[i] / m_wlen;
	buf[i] = buf[i + m_wlen/2] / m_wlen;
	buf[i + m_wlen/2] = temp;
    }
    
    m_window->cut(buf);

    for (i = 0; i < m_wlen; ++i) {
        out[i] += buf[i];
    }

    if (modulation) {

        float area = m_window->getArea();

        for (i = 0; i < m_wlen; ++i) {
            float val = m_window->getValue(i);
            modulation[i] += val * area;
        }
    }

    return isPercussive;
}

