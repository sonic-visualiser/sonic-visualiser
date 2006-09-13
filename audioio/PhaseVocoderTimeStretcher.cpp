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

PhaseVocoderTimeStretcher::PhaseVocoderTimeStretcher(float ratio,
                                                     size_t maxProcessInputBlockSize) :
    m_ratio(ratio)
                                                    //,
                                                    //    m_n1(inputIncrement),
                                                    //    m_n2(lrintf(m_n1 * ratio)),
                                                    //    m_wlen(std::max(windowSize, m_n2 * 2)),
                                                    //    m_inbuf(m_wlen),
                                                    //    m_outbuf(maxProcessInputBlockSize * ratio + 1024) //!!!
{
    if (ratio < 1) {
        m_n1 = 512;
        m_n2 = m_n1 * ratio;
        m_wlen = 1024;
    } else {
        m_n2 = 512;
        m_n1 = m_n2 / ratio;
        m_wlen = 1024;
    }
    
    m_inbuf = new RingBuffer<float>(m_wlen);
    m_outbuf = new RingBuffer<float>
        (lrintf((maxProcessInputBlockSize + m_wlen) * ratio));

    std::cerr << "PhaseVocoderTimeStretcher: ratio = " << ratio
              << ", n1 = " << m_n1 << ", n2 = " << m_n2 << ", wlen = "
              << m_wlen << ", max = " << maxProcessInputBlockSize << ", outbuflen = " << m_outbuf->getSize() << std::endl;

    m_window = new Window<float>(HanningWindow, m_wlen),

    m_time = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_wlen);
    m_freq = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_wlen);
    m_dbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
    m_mashbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
    m_modulationbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
    m_prevPhase = (float *)fftwf_malloc(sizeof(float) * m_wlen);
    m_prevAdjustedPhase = (float *)fftwf_malloc(sizeof(float) * m_wlen);

    m_plan = fftwf_plan_dft_1d(m_wlen, m_time, m_freq, FFTW_FORWARD, FFTW_ESTIMATE);
    m_iplan = fftwf_plan_dft_c2r_1d(m_wlen, m_freq, m_dbuf, FFTW_ESTIMATE);

    for (int i = 0; i < m_wlen; ++i) {
	m_mashbuf[i] = 0.0;
	m_modulationbuf[i] = 0.0;
        m_prevPhase[i] = 0.0;
        m_prevAdjustedPhase[i] = 0.0;
    }
}

PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher()
{
    std::cerr << "PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher" << std::endl;

    fftwf_destroy_plan(m_plan);
    fftwf_destroy_plan(m_iplan);

    fftwf_free(m_time);
    fftwf_free(m_freq);
    fftwf_free(m_dbuf);
    fftwf_free(m_mashbuf);
    fftwf_free(m_modulationbuf);
    fftwf_free(m_prevPhase);
    fftwf_free(m_prevAdjustedPhase);

    delete m_inbuf;
    delete m_outbuf;

    delete m_window;
}	

size_t
PhaseVocoderTimeStretcher::getProcessingLatency() const
{
    return getWindowSize() - getInputIncrement();
}

void
PhaseVocoderTimeStretcher::process(float *input, float *output, size_t samples)
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

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::process(" << samples << ", consumed = " << consumed << "), writable " << m_inbuf->getWriteSpace() <<", readable "<< m_outbuf->getReadSpace() << std::endl;
#endif

    while (consumed < samples) {

	size_t writable = m_inbuf->getWriteSpace();
	writable = std::min(writable, samples - consumed);

	if (writable == 0) {
	    //!!! then what? I don't think this should happen, but
	    std::cerr << "WARNING: PhaseVocoderTimeStretcher::process: writable == 0" << std::endl;
	    break;
	}

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "writing " << writable << " from index " << consumed << " to inbuf, consumed will be " << consumed + writable << std::endl;
#endif
	m_inbuf->write(input + consumed, writable);
	consumed += writable;

	while (m_inbuf->getReadSpace() >= m_wlen &&
	       m_outbuf->getWriteSpace() >= m_n2) {

	    // We know we have at least m_wlen samples available
	    // in m_inbuf->  We need to peek m_wlen of them for
	    // processing, and then read m_n1 to advance the read
	    // pointer.

	    size_t got = m_inbuf->peek(m_dbuf, m_wlen);
	    assert(got == m_wlen);
		
	    processBlock(m_dbuf, m_mashbuf, m_modulationbuf);

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	    std::cerr << "writing first " << m_n2 << " from mashbuf, skipping " << m_n1 << " on inbuf " << std::endl;
#endif
	    m_inbuf->skip(m_n1);

            for (size_t i = 0; i < m_n2; ++i) {
                if (m_modulationbuf[i] > 0.f) {
                    m_mashbuf[i] /= m_modulationbuf[i];
                }
            }

	    m_outbuf->write(m_mashbuf, m_n2);

	    for (size_t i = 0; i < m_wlen - m_n2; ++i) {
		m_mashbuf[i] = m_mashbuf[i + m_n2];
                m_modulationbuf[i] = m_modulationbuf[i + m_n2];
	    }

	    for (size_t i = m_wlen - m_n2; i < m_wlen; ++i) {
		m_mashbuf[i] = 0.0f;
                m_modulationbuf[i] = 0.0f;
	    }
	}

//	std::cerr << "WARNING: PhaseVocoderTimeStretcher::process: writespace not enough for output increment (" << m_outbuf->getWriteSpace() << " < " << m_n2 << ")" << std::endl;
//	}

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "loop ended: inbuf read space " << m_inbuf->getReadSpace() << ", outbuf write space " << m_outbuf->getWriteSpace() << std::endl;
#endif
    }

    size_t toRead = lrintf(samples * m_ratio);

    if (m_outbuf->getReadSpace() < toRead) {
	std::cerr << "WARNING: PhaseVocoderTimeStretcher::process: not enough data (yet?) (" << m_outbuf->getReadSpace() << " < " << toRead << ")" << std::endl;
	size_t fill = toRead - m_outbuf->getReadSpace();
	for (size_t i = 0; i < fill; ++i) {
	    output[i] = 0.0;
	}
	m_outbuf->read(output + fill, m_outbuf->getReadSpace());
    } else {
#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "enough data - writing " << toRead << " from outbuf" << std::endl;
#endif
	m_outbuf->read(output, toRead);
    }

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::process returning" << std::endl;
#endif
}

void
PhaseVocoderTimeStretcher::processBlock(float *buf, float *out, float *modulation)
{
    size_t i;

    // buf contains m_wlen samples; out contains enough space for
    // m_wlen * ratio samples (we mix into out, rather than replacing)

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::processBlock" << std::endl;
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

    for (i = 0; i < m_wlen; ++i) {
	
	float mag = sqrtf(m_freq[i][0] * m_freq[i][0] +
			  m_freq[i][1] * m_freq[i][1]);
		
        float phase = princargf(atan2f(m_freq[i][1], m_freq[i][0]));

        float omega = (2 * M_PI * m_n1 * i) / m_wlen;
	
        float expectedPhase = m_prevPhase[i] + omega;

        float phaseError = princargf(phase - expectedPhase);

        float phaseIncrement = (omega + phaseError) / m_n1;

        float adjustedPhase = m_prevAdjustedPhase[i] + m_n2 * phaseIncrement;
	
	float real = mag * cosf(adjustedPhase);
	float imag = mag * sinf(adjustedPhase);
	m_freq[i][0] = real;
	m_freq[i][1] = imag;

        m_prevPhase[i] = phase;
        m_prevAdjustedPhase[i] = adjustedPhase;
    }
    
    fftwf_execute(m_iplan); // m_freq -> in, inverse fft
    
    for (i = 0; i < m_wlen/2; ++i) {
	float temp = buf[i] / m_wlen;
	buf[i] = buf[i + m_wlen/2] / m_wlen;
	buf[i + m_wlen/2] = temp;
    }
    
    m_window->cut(buf);
/*    
    int div = m_wlen / m_n2;
    if (div > 1) div /= 2;
    for (i = 0; i < m_wlen; ++i) {
	buf[i] /= div;
    }
*/

    float area = m_window->getArea();

    for (i = 0; i < m_wlen; ++i) {
	out[i] += buf[i];
        float val = m_window->getValue(i);
        modulation[i] += val * area;
    }
}

