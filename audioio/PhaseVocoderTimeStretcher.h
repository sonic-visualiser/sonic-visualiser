/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
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

#include "data/fft/FFTapi.h"

#include <QMutex>

/**
 * A time stretcher that alters the performance speed of audio,
 * preserving pitch.
 *
 * This is based on the straightforward phase vocoder with phase
 * unwrapping (as in e.g. the DAFX book pp275-), with optional
 * percussive transient detection to avoid smearing percussive notes
 * and resynchronise phases, and adding a stream API for real-time
 * use.  Principles and methods from Chris Duxbury, AES 2002 and 2004
 * thesis; Emmanuel Ravelli, DAFX 2005; Dan Barry, ISSC 2005 on
 * percussion detection; code by Chris Cannam.
 */

class PhaseVocoderTimeStretcher
{
public:
    PhaseVocoderTimeStretcher(size_t sampleRate,
                              size_t channels,
                              float ratio,
                              bool sharpen,
                              size_t maxOutputBlockSize);
    virtual ~PhaseVocoderTimeStretcher();

    /**
     * Return the number of samples that would need to be added via
     * putInput in order to provoke the time stretcher into doing some
     * time stretching and making more output samples available.
     * This will be an estimate, if transient sharpening is on; the 
     * caller may need to do the put/get/test cycle more than once.
     */
    size_t getRequiredInputSamples() const;

    /**
     * Put (and possibly process) a given number of input samples.
     * Number should usually equal the value returned from
     * getRequiredInputSamples().
     */
    void putInput(float **input, size_t samples);

    /**
     * Get the number of processed samples ready for reading.
     */
    size_t getAvailableOutputSamples() const;

    /**
     * Get some processed samples.
     */
    void getOutput(float **output, size_t samples);

    //!!! and reset?

    /**
     * Change the time stretch ratio.
     */
    void setRatio(float ratio);

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
     * Get the stretch ratio.
     */
    float getRatio() const { return float(m_n2) / float(m_n1); }

    /**
     * Return whether this time stretcher will attempt to sharpen transients.
     */
    bool getSharpening() const { return m_sharpen; }

    /**
     * Return the number of channels for this time stretcher.
     */
    size_t getChannelCount() const { return m_channels; }

    /**
     * Get the latency added by the time stretcher, in sample frames.
     * This will be exact if transient sharpening is off, or approximate
     * if it is on.
     */
    size_t getProcessingLatency() const;

protected:
    /**
     * Process a single phase vocoder frame from "in" into
     * m_freq[channel].
     */
    void analyseBlock(size_t channel, float *in); // into m_freq[channel]

    /**
     * Examine m_freq[0..m_channels-1] and return whether a percussive
     * transient is found.
     */
    bool isTransient(); 

    /**
     * Resynthesise from m_freq[channel] adding in to "out",
     * adjusting phases on the basis of a prior step size of lastStep.
     * Also add the window shape in to the modulation array (if
     * present) -- for use in ensuring the output has the correct
     * magnitude afterwards.
     */
    void synthesiseBlock(size_t channel, float *out, float *modulation,
                         size_t lastStep);

    void initialise();
    void calculateParameters();
    void cleanup();

    bool shouldSharpen() {
        return m_sharpen && (m_ratio > 0.25);
    }

    size_t m_sampleRate;
    size_t m_channels;
    size_t m_maxOutputBlockSize;
    float m_ratio;
    bool m_sharpen;
    size_t m_n1;
    size_t m_n2;
    size_t m_wlen;
    Window<float> *m_analysisWindow;
    Window<float> *m_synthesisWindow;

    int m_totalCount;
    int m_transientCount;
    int m_n2sum;

    float **m_prevPhase;
    float **m_prevAdjustedPhase;

    float *m_prevTransientMag;
    int  m_prevTransientScore;
    int  m_transientThreshold;
    bool m_prevTransient;

    float *m_tempbuf;
    float **m_time;
    fftf_complex **m_freq;
    fftf_plan *m_plan;
    fftf_plan *m_iplan;
    
    RingBuffer<float> **m_inbuf;
    RingBuffer<float> **m_outbuf;
    float **m_mashbuf;
    float *m_modulationbuf;

    QMutex *m_mutex;
};

#endif
