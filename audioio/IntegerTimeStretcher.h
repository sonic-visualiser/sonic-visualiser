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

#ifndef _INTEGER_TIME_STRETCHER_H_
#define _INTEGER_TIME_STRETCHER_H_

#include "base/Window.h"
#include "base/RingBuffer.h"

#include <fftw3.h>

/**
 * A time stretcher that slows down audio by an integer multiple of
 * its original duration, preserving pitch.  This uses the simple
 * phase vocoder technique from DAFX pp275-276, adding a block-based
 * stream oriented API.
 *
 * Causes significant transient smearing, but sounds good for steady
 * notes and is generally predictable.
 */

class IntegerTimeStretcher
{
public:
    IntegerTimeStretcher(float ratio,
			 size_t maxProcessInputBlockSize,
			 size_t inputIncrement = 64,
			 size_t windowSize = 2048,
			 WindowType windowType = HanningWindow);
    virtual ~IntegerTimeStretcher();

    /**
     * Process a block.  The input array contains the given number of
     * samples; the output has enough space for samples * m_ratio.
     */
    void process(float *input, float *output, size_t samples);

    /**
     * Get the hop size for input.  Smaller values may produce better
     * results, at a cost in processing time.  Larger values are
     * faster but increase the likelihood of echo-like effects.  The
     * default is 64, which is usually pretty good, though heavy on
     * processor power.
     */
    size_t getInputIncrement() const { return m_n1; }

    /**
     * Get the window size for FFT processing.  Must be larger than
     * the input and output increments.  The default is 2048.
     */
    size_t getWindowSize() const { return m_wlen; }

    /**
     * Get the window type.  The default is a Hanning window.
     */
    WindowType getWindowType() const { return m_window->getType(); }

    float getRatio() const { return m_ratio; }
    size_t getOutputIncrement() const { return getInputIncrement() * getRatio(); }
    size_t getProcessingLatency() const;

protected:
    void processBlock(float *in, float *out);

    float m_ratio;
    size_t m_n1;
    size_t m_n2;
    size_t m_wlen;
    Window<float> *m_window;

    fftwf_complex *m_time;
    fftwf_complex *m_freq;
    float *m_dbuf;
    float *m_prevPhase;
    float *m_prevAdjustedPhase;

    fftwf_plan m_plan;
    fftwf_plan m_iplan;
    
    RingBuffer<float> m_inbuf;
    RingBuffer<float> m_outbuf;
    float *m_mashbuf;
};

#endif
