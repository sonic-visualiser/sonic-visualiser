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

#include "AudioCallbackPlaySource.h"

#include "AudioGenerator.h"

#include "data/model/Model.h"
#include "view/ViewManager.h"
#include "base/PlayParameterRepository.h"
#include "data/model/DenseTimeValueModel.h"
#include "data/model/SparseOneDimensionalModel.h"
#include "IntegerTimeStretcher.h"

#include <iostream>
#include <cassert>

//#define DEBUG_AUDIO_PLAY_SOURCE 1
//#define DEBUG_AUDIO_PLAY_SOURCE_PLAYING 1

//const size_t AudioCallbackPlaySource::m_ringBufferSize = 102400;
const size_t AudioCallbackPlaySource::m_ringBufferSize = 131071;

AudioCallbackPlaySource::AudioCallbackPlaySource(ViewManager *manager) :
    m_viewManager(manager),
    m_audioGenerator(new AudioGenerator()),
    m_readBuffers(0),
    m_writeBuffers(0),
    m_readBufferFill(0),
    m_writeBufferFill(0),
    m_bufferScavenger(1),
    m_sourceChannelCount(0),
    m_blockSize(1024),
    m_sourceSampleRate(0),
    m_targetSampleRate(0),
    m_playLatency(0),
    m_playing(false),
    m_exiting(false),
    m_lastModelEndFrame(0),
    m_outputLeft(0.0),
    m_outputRight(0.0),
    m_slowdownCounter(0),
    m_timeStretcher(0),
    m_fillThread(0),
    m_converter(0)
{
    m_viewManager->setAudioPlaySource(this);

    connect(m_viewManager, SIGNAL(selectionChanged()),
	    this, SLOT(selectionChanged()));
    connect(m_viewManager, SIGNAL(playLoopModeChanged()),
	    this, SLOT(playLoopModeChanged()));
    connect(m_viewManager, SIGNAL(playSelectionModeChanged()),
	    this, SLOT(playSelectionModeChanged()));

    connect(PlayParameterRepository::getInstance(),
	    SIGNAL(playParametersChanged(PlayParameters *)),
	    this, SLOT(playParametersChanged(PlayParameters *)));
}

AudioCallbackPlaySource::~AudioCallbackPlaySource()
{
    m_exiting = true;

    if (m_fillThread) {
	m_condition.wakeAll();
	m_fillThread->wait();
	delete m_fillThread;
    }

    clearModels();
    
    if (m_readBuffers != m_writeBuffers) {
	delete m_readBuffers;
    }

    delete m_writeBuffers;

    delete m_audioGenerator;

    m_bufferScavenger.scavenge(true);
}

void
AudioCallbackPlaySource::addModel(Model *model)
{
    if (m_models.find(model) != m_models.end()) return;

    bool canPlay = m_audioGenerator->addModel(model);

    m_mutex.lock();

    m_models.insert(model);
    if (model->getEndFrame() > m_lastModelEndFrame) {
	m_lastModelEndFrame = model->getEndFrame();
    }

    bool buffersChanged = false, srChanged = false;

    size_t modelChannels = 1;
    DenseTimeValueModel *dtvm = dynamic_cast<DenseTimeValueModel *>(model);
    if (dtvm) modelChannels = dtvm->getChannelCount();
    if (modelChannels > m_sourceChannelCount) {
	m_sourceChannelCount = modelChannels;
    }

//    std::cerr << "Adding model with " << modelChannels << " channels " << std::endl;

    if (m_sourceSampleRate == 0) {

	m_sourceSampleRate = model->getSampleRate();
	srChanged = true;

    } else if (model->getSampleRate() != m_sourceSampleRate) {

        // If this is a dense time-value model and we have no other, we
        // can just switch to this model's sample rate

        if (dtvm) {

            bool conflicting = false;

            for (std::set<Model *>::const_iterator i = m_models.begin();
                 i != m_models.end(); ++i) {
                if (*i != dtvm && dynamic_cast<DenseTimeValueModel *>(*i)) {
                    std::cerr << "AudioCallbackPlaySource::addModel: Conflicting dense time-value model " << *i << " found" << std::endl;
                    conflicting = true;
                    break;
                }
            }

            if (conflicting) {

                std::cerr << "AudioCallbackPlaySource::addModel: ERROR: "
                          << "New model sample rate does not match" << std::endl
                          << "existing model(s) (new " << model->getSampleRate()
                          << " vs " << m_sourceSampleRate
                          << "), playback will be wrong"
                          << std::endl;
                
                emit sampleRateMismatch(model->getSampleRate(), m_sourceSampleRate,
                                        false);
            } else {
                m_sourceSampleRate = model->getSampleRate();
                srChanged = true;
            }
        }
    }

    if (!m_writeBuffers || (m_writeBuffers->size() < getTargetChannelCount())) {
	clearRingBuffers(true, getTargetChannelCount());
	buffersChanged = true;
    } else {
	if (canPlay) clearRingBuffers(true);
    }

    if (buffersChanged || srChanged) {
	if (m_converter) {
	    src_delete(m_converter);
	    m_converter = 0;
	}
    }

    m_mutex.unlock();

    m_audioGenerator->setTargetChannelCount(getTargetChannelCount());

    if (!m_fillThread) {
	m_fillThread = new AudioCallbackPlaySourceFillThread(*this);
	m_fillThread->start();
    }

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cerr << "AudioCallbackPlaySource::addModel: emitting modelReplaced" << std::endl;
#endif

    if (buffersChanged || srChanged) {
	emit modelReplaced();
    }

    m_condition.wakeAll();
}

void
AudioCallbackPlaySource::removeModel(Model *model)
{
    m_mutex.lock();

    m_models.erase(model);

    if (m_models.empty()) {
	if (m_converter) {
	    src_delete(m_converter);
	    m_converter = 0;
	}
	m_sourceSampleRate = 0;
    }

    size_t lastEnd = 0;
    for (std::set<Model *>::const_iterator i = m_models.begin();
	 i != m_models.end(); ++i) {
//	std::cerr << "AudioCallbackPlaySource::removeModel(" << model << "): checking end frame on model " << *i << std::endl;
	if ((*i)->getEndFrame() > lastEnd) lastEnd = (*i)->getEndFrame();
//	std::cerr << "(done, lastEnd now " << lastEnd << ")" << std::endl;
    }
    m_lastModelEndFrame = lastEnd;

    m_mutex.unlock();

    m_audioGenerator->removeModel(model);

    clearRingBuffers();
}

void
AudioCallbackPlaySource::clearModels()
{
    m_mutex.lock();

    m_models.clear();

    if (m_converter) {
	src_delete(m_converter);
	m_converter = 0;
    }

    m_lastModelEndFrame = 0;

    m_sourceSampleRate = 0;

    m_mutex.unlock();

    m_audioGenerator->clearModels();
}    

void
AudioCallbackPlaySource::clearRingBuffers(bool haveLock, size_t count)
{
    if (!haveLock) m_mutex.lock();

    if (count == 0) {
	if (m_writeBuffers) count = m_writeBuffers->size();
    }

    size_t sf = m_readBufferFill;
    RingBuffer<float> *rb = getReadRingBuffer(0);
    if (rb) {
	//!!! This is incorrect if we're in a non-contiguous selection
	//Same goes for all related code (subtracting the read space
	//from the fill frame to try to establish where the effective
	//pre-resample/timestretch read pointer is)
	size_t rs = rb->getReadSpace();
	if (rs < sf) sf -= rs;
	else sf = 0;
    }
    m_writeBufferFill = sf;

    if (m_readBuffers != m_writeBuffers) {
	delete m_writeBuffers;
    }

    m_writeBuffers = new RingBufferVector;

    for (size_t i = 0; i < count; ++i) {
	m_writeBuffers->push_back(new RingBuffer<float>(m_ringBufferSize));
    }

//    std::cerr << "AudioCallbackPlaySource::clearRingBuffers: Created "
//	      << count << " write buffers" << std::endl;

    if (!haveLock) {
	m_mutex.unlock();
    }
}

void
AudioCallbackPlaySource::play(size_t startFrame)
{
    if (m_viewManager->getPlaySelectionMode() &&
	!m_viewManager->getSelections().empty()) {
	MultiSelection::SelectionList selections = m_viewManager->getSelections();
	MultiSelection::SelectionList::iterator i = selections.begin();
	if (i != selections.end()) {
	    if (startFrame < i->getStartFrame()) {
		startFrame = i->getStartFrame();
	    } else {
		MultiSelection::SelectionList::iterator j = selections.end();
		--j;
		if (startFrame >= j->getEndFrame()) {
		    startFrame = i->getStartFrame();
		}
	    }
	}
    } else {
	if (startFrame >= m_lastModelEndFrame) {
	    startFrame = 0;
	}
    }

    // The fill thread will automatically empty its buffers before
    // starting again if we have not so far been playing, but not if
    // we're just re-seeking.

    m_mutex.lock();
    if (m_playing) {
	m_readBufferFill = m_writeBufferFill = startFrame;
	if (m_readBuffers) {
	    for (size_t c = 0; c < getTargetChannelCount(); ++c) {
		RingBuffer<float> *rb = getReadRingBuffer(c);
		if (rb) rb->reset();
	    }
	}
	if (m_converter) src_reset(m_converter);
    } else {
	if (m_converter) src_reset(m_converter);
	m_readBufferFill = m_writeBufferFill = startFrame;
    }
    m_mutex.unlock();

    m_audioGenerator->reset();

    bool changed = !m_playing;
    m_playing = true;
    m_condition.wakeAll();
    if (changed) emit playStatusChanged(m_playing);
}

void
AudioCallbackPlaySource::stop()
{
    bool changed = m_playing;
    m_playing = false;
    m_condition.wakeAll();
    if (changed) emit playStatusChanged(m_playing);
}

void
AudioCallbackPlaySource::selectionChanged()
{
    if (m_viewManager->getPlaySelectionMode()) {
	clearRingBuffers();
    }
}

void
AudioCallbackPlaySource::playLoopModeChanged()
{
    clearRingBuffers();
}

void
AudioCallbackPlaySource::playSelectionModeChanged()
{
    if (!m_viewManager->getSelections().empty()) {
	clearRingBuffers();
    }
}

void
AudioCallbackPlaySource::playParametersChanged(PlayParameters *params)
{
    clearRingBuffers();
}

void
AudioCallbackPlaySource::setTargetBlockSize(size_t size)
{
//    std::cerr << "AudioCallbackPlaySource::setTargetBlockSize() -> " << size << std::endl;
    assert(size < m_ringBufferSize);
    m_blockSize = size;
}

size_t
AudioCallbackPlaySource::getTargetBlockSize() const
{
//    std::cerr << "AudioCallbackPlaySource::getTargetBlockSize() -> " << m_blockSize << std::endl;
    return m_blockSize;
}

void
AudioCallbackPlaySource::setTargetPlayLatency(size_t latency)
{
    m_playLatency = latency;
}

size_t
AudioCallbackPlaySource::getTargetPlayLatency() const
{
    return m_playLatency;
}

size_t
AudioCallbackPlaySource::getCurrentPlayingFrame()
{
    bool resample = false;
    double ratio = 1.0;

    if (getSourceSampleRate() != getTargetSampleRate()) {
	resample = true;
	ratio = double(getSourceSampleRate()) / double(getTargetSampleRate());
    }

    size_t readSpace = 0;
    for (size_t c = 0; c < getTargetChannelCount(); ++c) {
	RingBuffer<float> *rb = getReadRingBuffer(c);
	if (rb) {
	    size_t spaceHere = rb->getReadSpace();
	    if (c == 0 || spaceHere < readSpace) readSpace = spaceHere;
	}
    }

    if (resample) {
	readSpace = size_t(readSpace * ratio + 0.1);
    }

    size_t latency = m_playLatency;
    if (resample) latency = size_t(m_playLatency * ratio + 0.1);
    
    TimeStretcherData *timeStretcher = m_timeStretcher;
    if (timeStretcher) {
	latency += timeStretcher->getStretcher(0)->getProcessingLatency();
    }

    latency += readSpace;
    size_t bufferedFrame = m_readBufferFill;

    bool looping = m_viewManager->getPlayLoopMode();
    bool constrained = (m_viewManager->getPlaySelectionMode() &&
			!m_viewManager->getSelections().empty());

    size_t framePlaying = bufferedFrame;

    if (looping && !constrained) {
	while (framePlaying < latency) framePlaying += m_lastModelEndFrame;
    }

    if (framePlaying > latency) framePlaying -= latency;
    else framePlaying = 0;

    if (!constrained) {
	if (!looping && framePlaying > m_lastModelEndFrame) {
	    framePlaying = m_lastModelEndFrame;
	    stop();
	}
	return framePlaying;
    }

    MultiSelection::SelectionList selections = m_viewManager->getSelections();
    MultiSelection::SelectionList::const_iterator i;

    i = selections.begin();
    size_t rangeStart = i->getStartFrame();

    i = selections.end();
    --i;
    size_t rangeEnd = i->getEndFrame();

    for (i = selections.begin(); i != selections.end(); ++i) {
	if (i->contains(bufferedFrame)) break;
    }

    size_t f = bufferedFrame;

//    std::cerr << "getCurrentPlayingFrame: f=" << f << ", latency=" << latency << ", rangeEnd=" << rangeEnd << std::endl;

    if (i == selections.end()) {
	--i;
	if (i->getEndFrame() + latency < f) {
//    std::cerr << "framePlaying = " << framePlaying << ", rangeEnd = " << rangeEnd << std::endl;

	    if (!looping && (framePlaying > rangeEnd)) {
//		std::cerr << "STOPPING" << std::endl;
		stop();
		return rangeEnd;
	    } else {
		return framePlaying;
	    }
	} else {
//	    std::cerr << "latency <- " << latency << "-(" << f << "-" << i->getEndFrame() << ")" << std::endl;
	    latency -= (f - i->getEndFrame());
	    f = i->getEndFrame();
	}
    }

//    std::cerr << "i=(" << i->getStartFrame() << "," << i->getEndFrame() << ") f=" << f << ", latency=" << latency << std::endl;

    while (latency > 0) {
	size_t offset = f - i->getStartFrame();
	if (offset >= latency) {
	    if (f > latency) {
		framePlaying = f - latency;
	    } else {
		framePlaying = 0;
	    }
	    break;
	} else {
	    if (i == selections.begin()) {
		if (looping) {
		    i = selections.end();
		}
	    }
	    latency -= offset;
	    --i;
	    f = i->getEndFrame();
	}
    }

    return framePlaying;
}

void
AudioCallbackPlaySource::setOutputLevels(float left, float right)
{
    m_outputLeft = left;
    m_outputRight = right;
}

bool
AudioCallbackPlaySource::getOutputLevels(float &left, float &right)
{
    left = m_outputLeft;
    right = m_outputRight;
    return true;
}

void
AudioCallbackPlaySource::setTargetSampleRate(size_t sr)
{
    m_targetSampleRate = sr;

    if (getSourceSampleRate() != getTargetSampleRate()) {

	int err = 0;
	m_converter = src_new(SRC_SINC_BEST_QUALITY,
			      getTargetChannelCount(), &err);
	if (!m_converter) {
	    std::cerr
		<< "AudioCallbackPlaySource::setModel: ERROR in creating samplerate converter: "
		<< src_strerror(err) << std::endl;

            emit sampleRateMismatch(getSourceSampleRate(),
                                    getTargetSampleRate(),
                                    false);
	} else {

            emit sampleRateMismatch(getSourceSampleRate(),
                                    getTargetSampleRate(),
                                    true);
        }
    }
}

size_t
AudioCallbackPlaySource::getTargetSampleRate() const
{
    if (m_targetSampleRate) return m_targetSampleRate;
    else return getSourceSampleRate();
}

size_t
AudioCallbackPlaySource::getSourceChannelCount() const
{
    return m_sourceChannelCount;
}

size_t
AudioCallbackPlaySource::getTargetChannelCount() const
{
    if (m_sourceChannelCount < 2) return 2;
    return m_sourceChannelCount;
}

size_t
AudioCallbackPlaySource::getSourceSampleRate() const
{
    return m_sourceSampleRate;
}

AudioCallbackPlaySource::TimeStretcherData::TimeStretcherData(size_t channels,
							      size_t factor,
							      size_t blockSize) :
    m_factor(factor),
    m_blockSize(blockSize)
{
//    std::cerr << "TimeStretcherData::TimeStretcherData(" << channels << ", " << factor << ", " << blockSize << ")" << std::endl;

    for (size_t ch = 0; ch < channels; ++ch) {
	m_stretcher[ch] = StretcherBuffer
	    //!!! We really need to measure performance and work out
	    //what sort of quality level to use -- or at least to
	    //allow the user to configure it
	    (new IntegerTimeStretcher(factor, blockSize, 128),
	     new float[blockSize * factor]);
    }
    m_stretchInputBuffer = new float[blockSize];
}

AudioCallbackPlaySource::TimeStretcherData::~TimeStretcherData()
{
//    std::cerr << "TimeStretcherData::~TimeStretcherData" << std::endl;

    while (!m_stretcher.empty()) {
	delete m_stretcher.begin()->second.first;
	delete[] m_stretcher.begin()->second.second;
	m_stretcher.erase(m_stretcher.begin());
    }
    delete m_stretchInputBuffer;
}

IntegerTimeStretcher *
AudioCallbackPlaySource::TimeStretcherData::getStretcher(size_t channel)
{
    return m_stretcher[channel].first;
}

float *
AudioCallbackPlaySource::TimeStretcherData::getOutputBuffer(size_t channel)
{
    return m_stretcher[channel].second;
}

float *
AudioCallbackPlaySource::TimeStretcherData::getInputBuffer()
{
    return m_stretchInputBuffer;
}

void
AudioCallbackPlaySource::TimeStretcherData::run(size_t channel)
{
    getStretcher(channel)->process(getInputBuffer(),
				   getOutputBuffer(channel),
				   m_blockSize);
}

void
AudioCallbackPlaySource::setSlowdownFactor(size_t factor)
{
    // Avoid locks -- create, assign, mark old one for scavenging
    // later (as a call to getSourceSamples may still be using it)

    TimeStretcherData *existingStretcher = m_timeStretcher;

    if (existingStretcher && existingStretcher->getFactor() == factor) {
	return;
    }

    if (factor > 1) {
	TimeStretcherData *newStretcher = new TimeStretcherData
	    (getTargetChannelCount(), factor, getTargetBlockSize());
	m_slowdownCounter = 0;
	m_timeStretcher = newStretcher;
    } else {
	m_timeStretcher = 0;
    }

    if (existingStretcher) {
	m_timeStretcherScavenger.claim(existingStretcher);
    }
}
	    
size_t
AudioCallbackPlaySource::getSourceSamples(size_t count, float **buffer)
{
    if (!m_playing) {
	for (size_t ch = 0; ch < getTargetChannelCount(); ++ch) {
	    for (size_t i = 0; i < count; ++i) {
		buffer[ch][i] = 0.0;
	    }
	}
	return 0;
    }

    TimeStretcherData *timeStretcher = m_timeStretcher;

    if (!timeStretcher || timeStretcher->getFactor() == 1) {

	size_t got = 0;

	for (size_t ch = 0; ch < getTargetChannelCount(); ++ch) {

	    RingBuffer<float> *rb = getReadRingBuffer(ch);

	    if (rb) {

		// this is marginally more likely to leave our channels in
		// sync after a processing failure than just passing "count":
		size_t request = count;
		if (ch > 0) request = got;

		got = rb->read(buffer[ch], request);
	    
#ifdef DEBUG_AUDIO_PLAY_SOURCE_PLAYING
		std::cout << "AudioCallbackPlaySource::getSamples: got " << got << " samples on channel " << ch << ", signalling for more (possibly)" << std::endl;
#endif
	    }

	    for (size_t ch = 0; ch < getTargetChannelCount(); ++ch) {
		for (size_t i = got; i < count; ++i) {
		    buffer[ch][i] = 0.0;
		}
	    }
	}

        m_condition.wakeAll();
	return got;
    }

    if (m_slowdownCounter == 0) {

	size_t got = 0;
	float *ib = timeStretcher->getInputBuffer();

	for (size_t ch = 0; ch < getTargetChannelCount(); ++ch) {

	    RingBuffer<float> *rb = getReadRingBuffer(ch);

	    if (rb) {

		size_t request = count;
		if (ch > 0) request = got; // see above
		got = rb->read(buffer[ch], request);

#ifdef DEBUG_AUDIO_PLAY_SOURCE
		std::cout << "AudioCallbackPlaySource::getSamples: got " << got << " samples on channel " << ch << ", running time stretcher" << std::endl;
#endif

		for (size_t i = 0; i < count; ++i) {
		    ib[i] = buffer[ch][i];
		}
	    
		timeStretcher->run(ch);
	    }
	}

    } else if (m_slowdownCounter >= timeStretcher->getFactor()) {
	// reset this in case the factor has changed leaving the
	// counter out of range
	m_slowdownCounter = 0;
    }

    for (size_t ch = 0; ch < getTargetChannelCount(); ++ch) {

	float *ob = timeStretcher->getOutputBuffer(ch);

#ifdef DEBUG_AUDIO_PLAY_SOURCE
	std::cerr << "AudioCallbackPlaySource::getSamples: Copying from (" << (m_slowdownCounter * count) << "," << count << ") to buffer" << std::endl;
#endif

	for (size_t i = 0; i < count; ++i) {
	    buffer[ch][i] = ob[m_slowdownCounter * count + i];
	}
    }

//!!!    if (m_slowdownCounter == 0) m_condition.wakeAll();
    m_slowdownCounter = (m_slowdownCounter + 1) % timeStretcher->getFactor();
    return count;
}

// Called from fill thread, m_playing true, mutex held
bool
AudioCallbackPlaySource::fillBuffers()
{
    static float *tmp = 0;
    static size_t tmpSize = 0;

    size_t space = 0;
    for (size_t c = 0; c < getTargetChannelCount(); ++c) {
	RingBuffer<float> *wb = getWriteRingBuffer(c);
	if (wb) {
	    size_t spaceHere = wb->getWriteSpace();
	    if (c == 0 || spaceHere < space) space = spaceHere;
	}
    }
    
    if (space == 0) return false;

    size_t f = m_writeBufferFill;
	
    bool readWriteEqual = (m_readBuffers == m_writeBuffers);

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cout << "AudioCallbackPlaySourceFillThread: filling " << space << " frames" << std::endl;
#endif

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cout << "buffered to " << f << " already" << std::endl;
#endif

    bool resample = (getSourceSampleRate() != getTargetSampleRate());

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cout << (resample ? "" : "not ") << "resampling (source " << getSourceSampleRate() << ", target " << getTargetSampleRate() << ")" << std::endl;
#endif

    size_t channels = getTargetChannelCount();

    size_t orig = space;
    size_t got = 0;

    static float **bufferPtrs = 0;
    static size_t bufferPtrCount = 0;

    if (bufferPtrCount < channels) {
	if (bufferPtrs) delete[] bufferPtrs;
	bufferPtrs = new float *[channels];
	bufferPtrCount = channels;
    }

    size_t generatorBlockSize = m_audioGenerator->getBlockSize();

    if (resample && !m_converter) {
	static bool warned = false;
	if (!warned) {
	    std::cerr << "WARNING: sample rates differ, but no converter available!" << std::endl;
	    warned = true;
	}
    }

    if (resample && m_converter) {

	double ratio =
	    double(getTargetSampleRate()) / double(getSourceSampleRate());
	orig = size_t(orig / ratio + 0.1);

	// orig must be a multiple of generatorBlockSize
	orig = (orig / generatorBlockSize) * generatorBlockSize;
	if (orig == 0) return false;

	size_t work = std::max(orig, space);

	// We only allocate one buffer, but we use it in two halves.
	// We place the non-interleaved values in the second half of
	// the buffer (orig samples for channel 0, orig samples for
	// channel 1 etc), and then interleave them into the first
	// half of the buffer.  Then we resample back into the second
	// half (interleaved) and de-interleave the results back to
	// the start of the buffer for insertion into the ringbuffers.
	// What a faff -- especially as we've already de-interleaved
	// the audio data from the source file elsewhere before we
	// even reach this point.
	
	if (tmpSize < channels * work * 2) {
	    delete[] tmp;
	    tmp = new float[channels * work * 2];
	    tmpSize = channels * work * 2;
	}

	float *nonintlv = tmp + channels * work;
	float *intlv = tmp;
	float *srcout = tmp + channels * work;
	
	for (size_t c = 0; c < channels; ++c) {
	    for (size_t i = 0; i < orig; ++i) {
		nonintlv[channels * i + c] = 0.0f;
	    }
	}

	for (size_t c = 0; c < channels; ++c) {
	    bufferPtrs[c] = nonintlv + c * orig;
	}

	got = mixModels(f, orig, bufferPtrs);

	// and interleave into first half
	for (size_t c = 0; c < channels; ++c) {
	    for (size_t i = 0; i < got; ++i) {
		float sample = nonintlv[c * got + i];
		intlv[channels * i + c] = sample;
	    }
	}
		
	SRC_DATA data;
	data.data_in = intlv;
	data.data_out = srcout;
	data.input_frames = got;
	data.output_frames = work;
	data.src_ratio = ratio;
	data.end_of_input = 0;
	
	int err = src_process(m_converter, &data);
//	size_t toCopy = size_t(work * ratio + 0.1);
	size_t toCopy = size_t(got * ratio + 0.1);

	if (err) {
	    std::cerr
		<< "AudioCallbackPlaySourceFillThread: ERROR in samplerate conversion: "
		<< src_strerror(err) << std::endl;
	    //!!! Then what?
	} else {
	    got = data.input_frames_used;
	    toCopy = data.output_frames_gen;
#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    std::cerr << "Resampled " << got << " frames to " << toCopy << " frames" << std::endl;
#endif
	}
	
	for (size_t c = 0; c < channels; ++c) {
	    for (size_t i = 0; i < toCopy; ++i) {
		tmp[i] = srcout[channels * i + c];
	    }
	    RingBuffer<float> *wb = getWriteRingBuffer(c);
	    if (wb) wb->write(tmp, toCopy);
	}

	m_writeBufferFill = f;
	if (readWriteEqual) m_readBufferFill = f;

    } else {

	// space must be a multiple of generatorBlockSize
	space = (space / generatorBlockSize) * generatorBlockSize;
	if (space == 0) return false;

	if (tmpSize < channels * space) {
	    delete[] tmp;
	    tmp = new float[channels * space];
	    tmpSize = channels * space;
	}

	for (size_t c = 0; c < channels; ++c) {

	    bufferPtrs[c] = tmp + c * space;
	    
	    for (size_t i = 0; i < space; ++i) {
		tmp[c * space + i] = 0.0f;
	    }
	}

	size_t got = mixModels(f, space, bufferPtrs);

	for (size_t c = 0; c < channels; ++c) {

	    RingBuffer<float> *wb = getWriteRingBuffer(c);
	    if (wb) wb->write(bufferPtrs[c], got);

#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    if (wb)
		std::cerr << "Wrote " << got << " frames for ch " << c << ", now "
			  << wb->getReadSpace() << " to read" 
			  << std::endl;
#endif
	}

	m_writeBufferFill = f;
	if (readWriteEqual) m_readBufferFill = f;

	//!!! how do we know when ended? need to mark up a fully-buffered flag and check this if we find the buffers empty in getSourceSamples
    }

    return true;
}    

size_t
AudioCallbackPlaySource::mixModels(size_t &frame, size_t count, float **buffers)
{
    size_t processed = 0;
    size_t chunkStart = frame;
    size_t chunkSize = count;
    size_t selectionSize = 0;
    size_t nextChunkStart = chunkStart + chunkSize;
    
    bool looping = m_viewManager->getPlayLoopMode();
    bool constrained = (m_viewManager->getPlaySelectionMode() &&
			!m_viewManager->getSelections().empty());

    static float **chunkBufferPtrs = 0;
    static size_t chunkBufferPtrCount = 0;
    size_t channels = getTargetChannelCount();

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cerr << "Selection playback: start " << frame << ", size " << count <<", channels " << channels << std::endl;
#endif

    if (chunkBufferPtrCount < channels) {
	if (chunkBufferPtrs) delete[] chunkBufferPtrs;
	chunkBufferPtrs = new float *[channels];
	chunkBufferPtrCount = channels;
    }

    for (size_t c = 0; c < channels; ++c) {
	chunkBufferPtrs[c] = buffers[c];
    }

    while (processed < count) {
	
	chunkSize = count - processed;
	nextChunkStart = chunkStart + chunkSize;
	selectionSize = 0;

	size_t fadeIn = 0, fadeOut = 0;

	if (constrained) {
	    
	    Selection selection =
		m_viewManager->getContainingSelection(chunkStart, true);
	    
	    if (selection.isEmpty()) {
		if (looping) {
		    selection = *m_viewManager->getSelections().begin();
		    chunkStart = selection.getStartFrame();
		    fadeIn = 50;
		}
	    }

	    if (selection.isEmpty()) {

		chunkSize = 0;
		nextChunkStart = chunkStart;

	    } else {

		selectionSize =
		    selection.getEndFrame() -
		    selection.getStartFrame();

		if (chunkStart < selection.getStartFrame()) {
		    chunkStart = selection.getStartFrame();
		    fadeIn = 50;
		}

		nextChunkStart = chunkStart + chunkSize;

		if (nextChunkStart >= selection.getEndFrame()) {
		    nextChunkStart = selection.getEndFrame();
		    fadeOut = 50;
		}

		chunkSize = nextChunkStart - chunkStart;
	    }
	
	} else if (looping && m_lastModelEndFrame > 0) {

	    if (chunkStart >= m_lastModelEndFrame) {
		chunkStart = 0;
	    }
	    if (chunkSize > m_lastModelEndFrame - chunkStart) {
		chunkSize = m_lastModelEndFrame - chunkStart;
	    }
	    nextChunkStart = chunkStart + chunkSize;
	}
	
//	std::cerr << "chunkStart " << chunkStart << ", chunkSize " << chunkSize << ", nextChunkStart " << nextChunkStart << ", frame " << frame << ", count " << count << ", processed " << processed << std::endl;

	if (!chunkSize) {
#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    std::cerr << "Ending selection playback at " << nextChunkStart << std::endl;
#endif
	    // We need to maintain full buffers so that the other
	    // thread can tell where it's got to in the playback -- so
	    // return the full amount here
	    frame = frame + count;
	    return count;
	}

#ifdef DEBUG_AUDIO_PLAY_SOURCE
	std::cerr << "Selection playback: chunk at " << chunkStart << " -> " << nextChunkStart << " (size " << chunkSize << ")" << std::endl;
#endif

	size_t got = 0;

	if (selectionSize < 100) {
	    fadeIn = 0;
	    fadeOut = 0;
	} else if (selectionSize < 300) {
	    if (fadeIn > 0) fadeIn = 10;
	    if (fadeOut > 0) fadeOut = 10;
	}

	if (fadeIn > 0) {
	    if (processed * 2 < fadeIn) {
		fadeIn = processed * 2;
	    }
	}

	if (fadeOut > 0) {
	    if ((count - processed - chunkSize) * 2 < fadeOut) {
		fadeOut = (count - processed - chunkSize) * 2;
	    }
	}

	for (std::set<Model *>::iterator mi = m_models.begin();
	     mi != m_models.end(); ++mi) {
	    
	    got = m_audioGenerator->mixModel(*mi, chunkStart, 
					     chunkSize, chunkBufferPtrs,
					     fadeIn, fadeOut);
	}

	for (size_t c = 0; c < channels; ++c) {
	    chunkBufferPtrs[c] += chunkSize;
	}

	processed += chunkSize;
	chunkStart = nextChunkStart;
    }

#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cerr << "Returning selection playback " << processed << " frames to " << nextChunkStart << std::endl;
#endif

    frame = nextChunkStart;
    return processed;
}

void
AudioCallbackPlaySource::unifyRingBuffers()
{
    if (m_readBuffers == m_writeBuffers) return;

    // only unify if there will be something to read
    for (size_t c = 0; c < getTargetChannelCount(); ++c) {
	RingBuffer<float> *wb = getWriteRingBuffer(c);
	if (wb) {
	    if (wb->getReadSpace() < m_blockSize * 2) {
		if ((m_writeBufferFill + m_blockSize * 2) < 
		    m_lastModelEndFrame) {
		    // OK, we don't have enough and there's more to
		    // read -- don't unify until we can do better
		    return;
		}
	    }
	    break;
	}
    }

    size_t rf = m_readBufferFill;
    RingBuffer<float> *rb = getReadRingBuffer(0);
    if (rb) {
	size_t rs = rb->getReadSpace();
	//!!! incorrect when in non-contiguous selection, see comments elsewhere
//	std::cerr << "rs = " << rs << std::endl;
	if (rs < rf) rf -= rs;
	else rf = 0;
    }
    
    //std::cerr << "m_readBufferFill = " << m_readBufferFill << ", rf = " << rf << ", m_writeBufferFill = " << m_writeBufferFill << std::endl;

    size_t wf = m_writeBufferFill;
    size_t skip = 0;
    for (size_t c = 0; c < getTargetChannelCount(); ++c) {
	RingBuffer<float> *wb = getWriteRingBuffer(c);
	if (wb) {
	    if (c == 0) {
		
		size_t wrs = wb->getReadSpace();
//		std::cerr << "wrs = " << wrs << std::endl;

		if (wrs < wf) wf -= wrs;
		else wf = 0;
//		std::cerr << "wf = " << wf << std::endl;
		
		if (wf < rf) skip = rf - wf;
		if (skip == 0) break;
	    }

//	    std::cerr << "skipping " << skip << std::endl;
	    wb->skip(skip);
	}
    }
		    
    m_bufferScavenger.claim(m_readBuffers);
    m_readBuffers = m_writeBuffers;
    m_readBufferFill = m_writeBufferFill;
//    std::cerr << "unified" << std::endl;
}

void
AudioCallbackPlaySource::AudioCallbackPlaySourceFillThread::run()
{
    AudioCallbackPlaySource &s(m_source);
    
#ifdef DEBUG_AUDIO_PLAY_SOURCE
    std::cerr << "AudioCallbackPlaySourceFillThread starting" << std::endl;
#endif

    s.m_mutex.lock();

    bool previouslyPlaying = s.m_playing;
    bool work = false;

    while (!s.m_exiting) {

	s.unifyRingBuffers();
	s.m_bufferScavenger.scavenge();
	s.m_timeStretcherScavenger.scavenge();

	if (work && s.m_playing && s.getSourceSampleRate()) {
	    
#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    std::cout << "AudioCallbackPlaySourceFillThread: not waiting" << std::endl;
#endif

	    s.m_mutex.unlock();
	    s.m_mutex.lock();

	} else {
	    
	    float ms = 100;
	    if (s.getSourceSampleRate() > 0) {
		ms = float(m_ringBufferSize) / float(s.getSourceSampleRate()) * 1000.0;
	    }
	    
	    if (s.m_playing) ms /= 10;
	    
#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    std::cout << "AudioCallbackPlaySourceFillThread: waiting for " << ms << "ms..." << std::endl;
#endif
	    
	    s.m_condition.wait(&s.m_mutex, size_t(ms));
	}

#ifdef DEBUG_AUDIO_PLAY_SOURCE
	std::cout << "AudioCallbackPlaySourceFillThread: awoken" << std::endl;
#endif

	work = false;

	if (!s.getSourceSampleRate()) continue;

	bool playing = s.m_playing;

	if (playing && !previouslyPlaying) {
#ifdef DEBUG_AUDIO_PLAY_SOURCE
	    std::cout << "AudioCallbackPlaySourceFillThread: playback state changed, resetting" << std::endl;
#endif
	    for (size_t c = 0; c < s.getTargetChannelCount(); ++c) {
		RingBuffer<float> *rb = s.getReadRingBuffer(c);
		if (rb) rb->reset();
	    }
	}
	previouslyPlaying = playing;

	work = s.fillBuffers();
    }

    s.m_mutex.unlock();
}



#ifdef INCLUDE_MOCFILES
#include "AudioCallbackPlaySource.moc.cpp"
#endif

