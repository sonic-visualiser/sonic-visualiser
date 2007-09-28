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

#ifndef _AUDIO_CALLBACK_PLAY_SOURCE_H_
#define _AUDIO_CALLBACK_PLAY_SOURCE_H_

#include "base/RingBuffer.h"
#include "base/AudioPlaySource.h"
#include "base/PropertyContainer.h"
#include "base/Scavenger.h"

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

#include "base/Thread.h"

#include <samplerate.h>

#include <set>
#include <map>

class Model;
class ViewManager;
class AudioGenerator;
class PlayParameters;
class PhaseVocoderTimeStretcher;
class RealTimePluginInstance;

/**
 * AudioCallbackPlaySource manages audio data supply to callback-based
 * audio APIs such as JACK or CoreAudio.  It maintains one ring buffer
 * per channel, filled during playback by a non-realtime thread, and
 * provides a method for a realtime thread to pick up the latest
 * available sample data from these buffers.
 */
class AudioCallbackPlaySource : public virtual QObject,
				public AudioPlaySource
{
    Q_OBJECT

public:
    AudioCallbackPlaySource(ViewManager *);
    virtual ~AudioCallbackPlaySource();
    
    /**
     * Add a data model to be played from.  The source can mix
     * playback from a number of sources including dense and sparse
     * models.  The models must match in sample rate, but they don't
     * have to have identical numbers of channels.
     */
    virtual void addModel(Model *model);

    /**
     * Remove a model.
     */
    virtual void removeModel(Model *model);

    /**
     * Remove all models.  (Silence will ensue.)
     */
    virtual void clearModels();

    /**
     * Start making data available in the ring buffers for playback,
     * from the given frame.  If playback is already under way, reseek
     * to the given frame and continue.
     */
    virtual void play(size_t startFrame);

    /**
     * Stop playback and ensure that no more data is returned.
     */
    virtual void stop();

    /**
     * Return whether playback is currently supposed to be happening.
     */
    virtual bool isPlaying() const { return m_playing; }

    /**
     * Return the frame number that is currently expected to be coming
     * out of the speakers.  (i.e. compensating for playback latency.)
     */
    virtual size_t getCurrentPlayingFrame();

    /**
     * Return the frame at which playback is expected to end (if not looping).
     */
    virtual size_t getPlayEndFrame() { return m_lastModelEndFrame; }

    /**
     * Set the block size of the target audio device.  This should
     * be called by the target class.
     */
    void setTargetBlockSize(size_t);

    /**
     * Get the block size of the target audio device.
     */
    size_t getTargetBlockSize() const;

    /**
     * Set the playback latency of the target audio device, in frames
     * at the target sample rate.  This is the difference between the
     * frame currently "leaving the speakers" and the last frame (or
     * highest last frame across all channels) requested via
     * getSamples().  The default is zero.
     */
    void setTargetPlayLatency(size_t);

    /**
     * Get the playback latency of the target audio device.
     */
    size_t getTargetPlayLatency() const;

    /**
     * Specify that the target audio device has a fixed sample rate
     * (i.e. cannot accommodate arbitrary sample rates based on the
     * source).  If the target sets this to something other than the
     * source sample rate, this class will resample automatically to
     * fit.
     */
    void setTargetSampleRate(size_t);

    /**
     * Return the sample rate set by the target audio device (or the
     * source sample rate if the target hasn't set one).
     */
    virtual size_t getTargetSampleRate() const;

    /**
     * Set the current output levels for metering (for call from the
     * target)
     */
    void setOutputLevels(float left, float right);

    /**
     * Return the current (or thereabouts) output levels in the range
     * 0.0 -> 1.0, for metering purposes.
     */
    virtual bool getOutputLevels(float &left, float &right);

    /**
     * Get the number of channels of audio that in the source models.
     * This may safely be called from a realtime thread.  Returns 0 if
     * there is no source yet available.
     */
    size_t getSourceChannelCount() const;

    /**
     * Get the number of channels of audio that will be provided
     * to the play target.  This may be more than the source channel
     * count: for example, a mono source will provide 2 channels
     * after pan.
     * This may safely be called from a realtime thread.  Returns 0 if
     * there is no source yet available.
     */
    size_t getTargetChannelCount() const;

    /**
     * Get the actual sample rate of the source material.  This may
     * safely be called from a realtime thread.  Returns 0 if there is
     * no source yet available.
     */
    virtual size_t getSourceSampleRate() const;

    /**
     * Get "count" samples (at the target sample rate) of the mixed
     * audio data, in all channels.  This may safely be called from a
     * realtime thread.
     */
    size_t getSourceSamples(size_t count, float **buffer);

    /**
     * Set the time stretcher factor (i.e. playback speed).  Also
     * specify whether the time stretcher will be variable rate
     * (sharpening transients), and whether time stretching will be
     * carried out on data mixed down to mono for speed.
     */
    void setTimeStretch(float factor, bool sharpen, bool mono);

    /**
     * Set the resampler quality, 0 - 2 where 0 is fastest and 2 is
     * highest quality.
     */
    void setResampleQuality(int q);

    /**
     * Set a single real-time plugin as a processing effect for
     * auditioning during playback.
     *
     * The plugin must have been initialised with
     * getTargetChannelCount() channels and a getTargetBlockSize()
     * sample frame processing block size.
     *
     * This playback source takes ownership of the plugin, which will
     * be deleted at some point after the following call to
     * setAuditioningPlugin (depending on real-time constraints).
     *
     * Pass a null pointer to remove the current auditioning plugin,
     * if any.
     */
    void setAuditioningPlugin(RealTimePluginInstance *plugin);

    /**
     * Specify that only the given set of models should be played.
     */
    void setSoloModelSet(std::set<Model *>s);

    /**
     * Specify that all models should be played as normal (if not
     * muted).
     */
    void clearSoloModelSet();

signals:
    void modelReplaced();

    void playStatusChanged(bool isPlaying);

    void sampleRateMismatch(size_t requested, size_t available, bool willResample);

    void audioOverloadPluginDisabled();

public slots:
    void audioProcessingOverload();

protected slots:
    void selectionChanged();
    void playLoopModeChanged();
    void playSelectionModeChanged();
    void playParametersChanged(PlayParameters *);
    void preferenceChanged(PropertyContainer::PropertyName);
    void modelChanged(size_t startFrame, size_t endFrame);

protected:
    ViewManager                     *m_viewManager;
    AudioGenerator                  *m_audioGenerator;

    class RingBufferVector : public std::vector<RingBuffer<float> *> {
    public:
	virtual ~RingBufferVector() {
	    while (!empty()) {
		delete *begin();
		erase(begin());
	    }
	}
    };

    std::set<Model *>                 m_models;
    RingBufferVector                 *m_readBuffers;
    RingBufferVector                 *m_writeBuffers;
    size_t                            m_readBufferFill;
    size_t                            m_writeBufferFill;
    Scavenger<RingBufferVector>       m_bufferScavenger;
    size_t                            m_sourceChannelCount;
    size_t                            m_blockSize;
    size_t                            m_sourceSampleRate;
    size_t                            m_targetSampleRate;
    size_t                            m_playLatency;
    bool                              m_playing;
    bool                              m_exiting;
    size_t                            m_lastModelEndFrame;
    static const size_t               m_ringBufferSize;
    float                             m_outputLeft;
    float                             m_outputRight;
    RealTimePluginInstance           *m_auditioningPlugin;
    bool                              m_auditioningPluginBypassed;
    Scavenger<RealTimePluginInstance> m_pluginScavenger;

    RingBuffer<float> *getWriteRingBuffer(size_t c) {
	if (m_writeBuffers && c < m_writeBuffers->size()) {
	    return (*m_writeBuffers)[c];
	} else {
	    return 0;
	}
    }

    RingBuffer<float> *getReadRingBuffer(size_t c) {
	RingBufferVector *rb = m_readBuffers;
	if (rb && c < rb->size()) {
	    return (*rb)[c];
	} else {
	    return 0;
	}
    }

    void clearRingBuffers(bool haveLock = false, size_t count = 0);
    void unifyRingBuffers();

    PhaseVocoderTimeStretcher *m_timeStretcher;
    Scavenger<PhaseVocoderTimeStretcher> m_timeStretcherScavenger;

    // Called from fill thread, m_playing true, mutex held
    // Return true if work done
    bool fillBuffers();
    
    // Called from fillBuffers.  Return the number of frames written,
    // which will be count or fewer.  Return in the frame argument the
    // new buffered frame position (which may be earlier than the
    // frame argument passed in, in the case of looping).
    size_t mixModels(size_t &frame, size_t count, float **buffers);

    // Called from getSourceSamples.
    void applyAuditioningEffect(size_t count, float **buffers);

    class FillThread : public Thread
    {
    public:
	FillThread(AudioCallbackPlaySource &source) :
            Thread(Thread::NonRTThread),
	    m_source(source) { }

	virtual void run();

    protected:
	AudioCallbackPlaySource &m_source;
    };

    QMutex m_mutex;
    QWaitCondition m_condition;
    FillThread *m_fillThread;
    SRC_STATE *m_converter;
    SRC_STATE *m_crapConverter; // for use when playing very fast
    int m_resampleQuality;
    void initialiseConverter();
};

#endif


