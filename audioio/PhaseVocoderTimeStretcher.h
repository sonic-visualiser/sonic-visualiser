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

#ifndef _PHASE_VOCODER_TIME_STRETCHER_H_
#define _PHASE_VOCODER_TIME_STRETCHER_H_

#include "base/Window.h"
#include "base/RingBuffer.h"

#include <fftw3.h>

/**
 * A time stretcher that alters the performance speed of audio,
 * preserving pitch.  This uses the simple phase vocoder technique
 * from DAFX pp275-276, adding a block-based stream oriented API.
 *
 * Causes significant transient smearing, but sounds good for steady
 * notes and is generally predictable.
 */

class PhaseVocoderTimeStretcher
{
public:
    PhaseVocoderTimeStretcher(size_t channels,
                              float ratio,
                              bool sharpen,
                              size_t maxProcessInputBlockSize);
    virtual ~PhaseVocoderTimeStretcher();

    /**
     * Process a block.  The input array contains the given number of
     * samples (on each channel); the output must have space for
     * lrintf(samples * m_ratio).
     * 
     * This should work correctly for some ratios, e.g. small powers
     * of two.  For other ratios it may drop samples -- use putInput
     * in a loop followed by getOutput (when getAvailableOutputSamples
     * reports enough) instead.
     *
     * Do not mix process calls with putInput/getOutput calls.
     */
    void process(float **input, float **output, size_t samples);

    /**
     * Return the number of samples that would need to be added via
     * putInput in order to provoke the time stretcher into doing some
     * time stretching and making more output samples available.
     */
    size_t getRequiredInputSamples() const;

    /**
     * Put (and possibly process) a given number of input samples.
     * Number must not exceed the maxProcessInputBlockSize passed to
     * constructor.
     */
    void putInput(float **input, size_t samples);

    size_t getAvailableOutputSamples() const;

    void getOutput(float **output, size_t samples);

    //!!! and reset?

    /**
     * Get the hop size for input.
     */
    size_t getInputIncrement() const { return m_n1; }

    /**
     * Get the hop size for output.
     */
    size_t getOutputIncrement() const { return m_n2; }

    /**
     * Get the window size for FFT processing.
     */
    size_t getWindowSize() const { return m_wlen; }

    /**
     * Get the window type.
     */
    WindowType getWindowType() const { return m_window->getType(); }

    /**
     * Get the stretch ratio.
     */
    float getRatio() const { return float(m_n2) / float(m_n1); }

    /**
     * Return whether this time stretcher will attempt to sharpen transients.
     */
    bool getSharpening() const { return m_sharpen; }

    /**
     * Get the latency added by the time stretcher, in sample frames.
     */
    size_t getProcessingLatency() const;

protected:
    /**
     * Process a single phase vocoder frame.
     * 
     * Take m_wlen time-domain source samples from in, perform an FFT,
     * phase shift, and IFFT, and add the results to out (presumably
     * overlapping parts of existing data from prior frames).
     *
     * Also add to the modulation output the results of windowing a
     * set of 1s with the resynthesis window -- this can then be used
     * to ensure the output has the correct magnitude in cases where
     * the window overlap varies or otherwise results in something
     * other than a flat sum.
     */
    bool processBlock(size_t channel,
                      float *in, float *out,
                      float *modulation,
                      size_t lastStep);

    size_t m_channels;
    float m_ratio;
    bool m_sharpen;
    size_t m_n1;
    size_t m_n2;
    size_t m_wlen;
    Window<float> *m_window;

    float **m_prevPhase;
    float **m_prevAdjustedPhase;
    float **m_prevMag;
    int *m_prevPercussiveCount;
    bool m_prevPercussive;

    float *m_dbuf;
    float *m_time;
    fftwf_complex *m_freq;
    fftwf_plan m_plan;
    fftwf_plan m_iplan;
    
    RingBuffer<float> **m_inbuf;
    RingBuffer<float> **m_outbuf;
    float **m_mashbuf;
    float *m_modulationbuf;
};

#endif
