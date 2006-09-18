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

#include <QMutexLocker>

//#define DEBUG_PHASE_VOCODER_TIME_STRETCHER 1

PhaseVocoderTimeStretcher::PhaseVocoderTimeStretcher(size_t sampleRate,
                                                     size_t channels,
                                                     float ratio,
                                                     bool sharpen,
                                                     size_t maxProcessInputBlockSize) :
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_maxProcessInputBlockSize(maxProcessInputBlockSize),
    m_ratio(ratio),
    m_sharpen(sharpen),
    m_totalCount(0),
    m_transientCount(0),
    m_n2sum(0),
    m_mutex(new QMutex())
{
    initialise();

}

PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher()
{
    std::cerr << "PhaseVocoderTimeStretcher::~PhaseVocoderTimeStretcher" << std::endl;

    cleanup();
    
    delete m_mutex;
}

void
PhaseVocoderTimeStretcher::initialise()
{
    std::cerr << "PhaseVocoderTimeStretcher::initialise" << std::endl;

    calculateParameters();
        
    m_analysisWindow = new Window<float>(HanningWindow, m_wlen);
    m_synthesisWindow = new Window<float>(HanningWindow, m_wlen);

    m_prevPhase = new float *[m_channels];
    m_prevAdjustedPhase = new float *[m_channels];

    m_prevTransientMag = (float *)fftwf_malloc(sizeof(float) * (m_wlen / 2 + 1));
    m_prevTransientScore = 0;
    m_prevTransient = false;

    m_tempbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);

    m_time = new float *[m_channels];
    m_freq = new fftwf_complex *[m_channels];
    m_plan = new fftwf_plan[m_channels];
    m_iplan = new fftwf_plan[m_channels];

    m_inbuf = new RingBuffer<float> *[m_channels];
    m_outbuf = new RingBuffer<float> *[m_channels];
    m_mashbuf = new float *[m_channels];

    m_modulationbuf = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        
    for (size_t c = 0; c < m_channels; ++c) {

        m_prevPhase[c] = (float *)fftwf_malloc(sizeof(float) * (m_wlen / 2 + 1));
        m_prevAdjustedPhase[c] = (float *)fftwf_malloc(sizeof(float) * (m_wlen / 2 + 1));

        m_time[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        m_freq[c] = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) *
                                                  (m_wlen / 2 + 1));
        
        m_plan[c] = fftwf_plan_dft_r2c_1d(m_wlen, m_time[c], m_freq[c], FFTW_ESTIMATE);
        m_iplan[c] = fftwf_plan_dft_c2r_1d(m_wlen, m_freq[c], m_time[c], FFTW_ESTIMATE);

        m_inbuf[c] = new RingBuffer<float>(m_wlen);
        m_outbuf[c] = new RingBuffer<float>
            (lrintf((m_maxProcessInputBlockSize + m_wlen) * m_ratio));
            
        m_mashbuf[c] = (float *)fftwf_malloc(sizeof(float) * m_wlen);
        
        for (int i = 0; i < m_wlen; ++i) {
            m_mashbuf[c][i] = 0.0;
        }

        for (int i = 0; i <= m_wlen/2; ++i) {
            m_prevPhase[c][i] = 0.0;
            m_prevAdjustedPhase[c][i] = 0.0;
        }
    }

    for (int i = 0; i < m_wlen; ++i) {
        m_modulationbuf[i] = 0.0;
    }

    for (int i = 0; i <= m_wlen/2; ++i) {
        m_prevTransientMag[i] = 0.0;
    }
}

void
PhaseVocoderTimeStretcher::calculateParameters()
{
    std::cerr << "PhaseVocoderTimeStretcher::calculateParameters" << std::endl;

    m_wlen = 1024;

    //!!! In transient sharpening mode, we need to pick the window
    //length so as to be more or less fixed in audio duration (i.e. we
    //need to exploit the sample rate)

    //!!! have to work out the relationship between wlen and transient
    //threshold

    if (m_ratio < 1) {
        if (m_ratio < 0.4) {
            m_n1 = 1024;
            m_wlen = 2048;
        } else if (m_ratio < 0.8) {
            m_n1 = 512;
        } else {
            m_n1 = 256;
        }
        if (m_sharpen) {
            m_wlen = 2048;
        }
        m_n2 = m_n1 * m_ratio;
    } else {
        if (m_ratio > 2) {
            m_n2 = 512;
            m_wlen = 4096; 
        } else if (m_ratio > 1.6) {
            m_n2 = 384;
            m_wlen = 2048;
        } else {
            m_n2 = 256;
        }
        if (m_sharpen) {
            if (m_wlen < 2048) m_wlen = 2048;
        }
        m_n1 = m_n2 / m_ratio;
    }

    m_transientThreshold = m_wlen / 4.5;

    m_totalCount = 0;
    m_transientCount = 0;
    m_n2sum = 0;


    std::cerr << "PhaseVocoderTimeStretcher: channels = " << m_channels
              << ", ratio = " << m_ratio
              << ", n1 = " << m_n1 << ", n2 = " << m_n2 << ", wlen = "
              << m_wlen << ", max = " << m_maxProcessInputBlockSize << std::endl;
//              << ", outbuflen = " << m_outbuf[0]->getSize() << std::endl;
}

void
PhaseVocoderTimeStretcher::cleanup()
{
    std::cerr << "PhaseVocoderTimeStretcher::cleanup" << std::endl;

    for (size_t c = 0; c < m_channels; ++c) {

        fftwf_destroy_plan(m_plan[c]);
        fftwf_destroy_plan(m_iplan[c]);

        fftwf_free(m_time[c]);
        fftwf_free(m_freq[c]);

        fftwf_free(m_mashbuf[c]);
        fftwf_free(m_prevPhase[c]);
        fftwf_free(m_prevAdjustedPhase[c]);

        delete m_inbuf[c];
        delete m_outbuf[c];
    }

    fftwf_free(m_tempbuf);
    fftwf_free(m_modulationbuf);
    fftwf_free(m_prevTransientMag);

    delete[] m_prevPhase;
    delete[] m_prevAdjustedPhase;
    delete[] m_inbuf;
    delete[] m_outbuf;
    delete[] m_mashbuf;
    delete[] m_time;
    delete[] m_freq;
    delete[] m_plan;
    delete[] m_iplan;

    delete m_analysisWindow;
    delete m_synthesisWindow;
}	

void
PhaseVocoderTimeStretcher::setRatio(float ratio)
{
    QMutexLocker locker(m_mutex);

    float formerRatio = m_ratio;
    size_t formerWlen = m_wlen;

    m_ratio = ratio;

    calculateParameters();

    if (m_wlen == formerWlen) {

        // This is the only container whose size depends on m_ratio

        RingBuffer<float> **newout = new RingBuffer<float> *[m_channels];

        size_t formerSize = m_outbuf[0]->getSize();
        size_t newSize = lrintf((m_maxProcessInputBlockSize + m_wlen) * m_ratio);
        size_t ready = m_outbuf[0]->getReadSpace();

        for (size_t c = 0; c < m_channels; ++c) {
            newout[c] = new RingBuffer<float>(newSize);
        }

        if (ready > 0) {

            size_t copy = std::min(ready, newSize);
            float *tmp = new float[ready];

            for (size_t c = 0; c < m_channels; ++c) {
                m_outbuf[c]->read(tmp, ready);
                newout[c]->write(tmp + ready - copy, copy);
            }

            delete[] tmp;
        }

        for (size_t c = 0; c < m_channels; ++c) {
            delete m_outbuf[c];
        }

        delete[] m_outbuf;
        m_outbuf = newout;

    } else {
        
        std::cerr << "wlen changed" << std::endl;
        cleanup();
        initialise();
    }
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
    QMutexLocker locker(m_mutex);

    if (m_inbuf[0]->getReadSpace() >= m_wlen) return 0;
    return m_wlen - m_inbuf[0]->getReadSpace();
}

void
PhaseVocoderTimeStretcher::putInput(float **input, size_t samples)
{
    QMutexLocker locker(m_mutex);

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
            
            for (size_t c = 0; c < m_channels; ++c) {

                size_t got = m_inbuf[c]->peek(m_tempbuf, m_wlen);
                assert(got == m_wlen);

                analyseBlock(c, m_tempbuf);
            }

            bool transient = false;
            if (m_sharpen) transient = isTransient();

            size_t n2 = m_n2;

            if (transient) {
                n2 = m_n1;
            }

            ++m_totalCount;
            if (transient) ++m_transientCount;
            m_n2sum += n2;

//            std::cerr << "ratio for last 10: " <<last10num << "/" << (10 * m_n1) << " = " << float(last10num) / float(10 * m_n1) << " (should be " << m_ratio << ")" << std::endl;
            
            if (m_totalCount > 50 && m_transientCount < m_totalCount) {

                int fixed = lrintf(m_transientCount * m_n1);
                int squashy = m_n2sum - fixed;

                int idealTotal = lrintf(m_totalCount * m_n1 * m_ratio);
                int idealSquashy = idealTotal - fixed;

                int squashyCount = m_totalCount - m_transientCount;
                
                n2 = lrintf(idealSquashy / squashyCount);

                if (n2 != m_n2) {
                    std::cerr << m_n2 << " -> " << n2 << std::endl;
                }
            }

            for (size_t c = 0; c < m_channels; ++c) {

                synthesiseBlock(c, m_mashbuf[c],
                                c == 0 ? m_modulationbuf : 0,
                                m_prevTransient ? m_n1 : m_n2);


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

            m_prevTransient = transient;

            for (size_t i = 0; i < m_wlen - n2; ++i) {
                m_modulationbuf[i] = m_modulationbuf[i + n2];
	    }

	    for (size_t i = m_wlen - n2; i < m_wlen; ++i) {
                m_modulationbuf[i] = 0.0f;
	    }

            if (!transient) m_n2 = n2;
	}


#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
	std::cerr << "loop ended: inbuf read space " << m_inbuf[0]->getReadSpace() << ", outbuf write space " << m_outbuf[0]->getWriteSpace() << std::endl;
#endif
    }

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::putInput returning" << std::endl;
#endif

//    std::cerr << "ratio: nominal: " << getRatio() << " actual: "
//              << m_total2 << "/" << m_total1 << " = " << float(m_total2) / float(m_total1) << " ideal: " << m_ratio << std::endl;
}

size_t
PhaseVocoderTimeStretcher::getAvailableOutputSamples() const
{
    QMutexLocker locker(m_mutex);

    return m_outbuf[0]->getReadSpace();
}

void
PhaseVocoderTimeStretcher::getOutput(float **output, size_t samples)
{
    QMutexLocker locker(m_mutex);

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

void
PhaseVocoderTimeStretcher::analyseBlock(size_t c, float *buf)
{
    size_t i;

    // buf contains m_wlen samples

#ifdef DEBUG_PHASE_VOCODER_TIME_STRETCHER
    std::cerr << "PhaseVocoderTimeStretcher::analyseBlock (channel " << c << ")" << std::endl;
#endif

    m_analysisWindow->cut(buf);

    for (i = 0; i < m_wlen/2; ++i) {
	float temp = buf[i];
	buf[i] = buf[i + m_wlen/2];
	buf[i + m_wlen/2] = temp;
    }

    for (i = 0; i < m_wlen; ++i) {
	m_time[c][i] = buf[i];
    }

    fftwf_execute(m_plan[c]); // m_time -> m_freq
}

bool
PhaseVocoderTimeStretcher::isTransient()
{
    int count = 0;

    for (int i = 0; i <= m_wlen/2; ++i) {

        float real = 0.f, imag = 0.f;

        for (size_t c = 0; c < m_channels; ++c) {
            real += m_freq[c][i][0];
            imag += m_freq[c][i][1];
        }

        float sqrmag = (real * real + imag * imag);

        if (m_prevTransientMag[i] > 0.f) {
            float diff = 10.f * log10f(sqrmag / m_prevTransientMag[i]);
            if (diff > 3.f) ++count;
        }

        m_prevTransientMag[i] = sqrmag;
    }

    bool isTransient = false;

//    if (count > m_transientThreshold &&
//        count > m_prevTransientScore * 1.2) {
    if (count > m_prevTransientScore &&
        count > m_transientThreshold &&
        count - m_prevTransientScore > m_wlen / 20) {
        isTransient = true;


        std::cerr << "isTransient (count = " << count << ", prev = " << m_prevTransientScore << ", diff = " << count - m_prevTransientScore << ", ratio = " << (m_totalCount > 0 ? (float (m_n2sum) / float(m_totalCount * m_n1)) : 1.f) << ", ideal = " << m_ratio << ")" << std::endl;
//    } else {
//        std::cerr << " !transient (count = " << count << ", prev = " << m_prevTransientScore << ", diff = " << count - m_prevTransientScore << ")" << std::endl;
    }

    m_prevTransientScore = count;

    return isTransient;
}

void
PhaseVocoderTimeStretcher::synthesiseBlock(size_t c,
                                           float *out,
                                           float *modulation,
                                           size_t lastStep)
{
    int i;

    bool unchanged = (lastStep == m_n1);

    for (i = 0; i <= m_wlen/2; ++i) {
		
        float phase = princargf(atan2f(m_freq[c][i][1], m_freq[c][i][0]));
        float adjustedPhase = phase;

        if (!unchanged) {

            float mag = sqrtf(m_freq[c][i][0] * m_freq[c][i][0] +
                              m_freq[c][i][1] * m_freq[c][i][1]);

            float omega = (2 * M_PI * m_n1 * i) / m_wlen;
	
            float expectedPhase = m_prevPhase[c][i] + omega;

            float phaseError = princargf(phase - expectedPhase);

            float phaseIncrement = (omega + phaseError) / m_n1;
            
            adjustedPhase = m_prevAdjustedPhase[c][i] +
                lastStep * phaseIncrement;
            
            float real = mag * cosf(adjustedPhase);
            float imag = mag * sinf(adjustedPhase);
            m_freq[c][i][0] = real;
            m_freq[c][i][1] = imag;
        }

        m_prevPhase[c][i] = phase;
        m_prevAdjustedPhase[c][i] = adjustedPhase;
    }

    fftwf_execute(m_iplan[c]); // m_freq -> m_time, inverse fft

    for (i = 0; i < m_wlen/2; ++i) {
        float temp = m_time[c][i];
        m_time[c][i] = m_time[c][i + m_wlen/2];
        m_time[c][i + m_wlen/2] = temp;
    }
    
    for (i = 0; i < m_wlen; ++i) {
        m_time[c][i] = m_time[c][i] / m_wlen;
    }

    m_synthesisWindow->cut(m_time[c]);

    for (i = 0; i < m_wlen; ++i) {
        out[i] += m_time[c][i];
    }

    if (modulation) {

        float area = m_analysisWindow->getArea();

        for (i = 0; i < m_wlen; ++i) {
            float val = m_synthesisWindow->getValue(i);
            modulation[i] += val * area;
        }
    }
}


