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

#include "RealTimePluginTransform.h"

#include "plugin/RealTimePluginFactory.h"
#include "plugin/RealTimePluginInstance.h"
#include "plugin/PluginXml.h"

#include "data/model/Model.h"
#include "data/model/SparseTimeValueModel.h"
#include "data/model/DenseTimeValueModel.h"
#include "data/model/WritableWaveFileModel.h"
#include "data/model/WaveFileModel.h"

#include <iostream>

RealTimePluginTransform::RealTimePluginTransform(Model *inputModel,
                                                 QString pluginId,
                                                 const ExecutionContext &context,
                                                 QString configurationXml,
                                                 QString units,
                                                 int output) :
    PluginTransform(inputModel, context),
    m_pluginId(pluginId),
    m_configurationXml(configurationXml),
    m_units(units),
    m_plugin(0),
    m_outputNo(output)
{
    if (!m_context.blockSize) m_context.blockSize = 1024;

    std::cerr << "RealTimePluginTransform::RealTimePluginTransform: plugin " << pluginId.toStdString() << ", output " << output << std::endl;

    RealTimePluginFactory *factory =
	RealTimePluginFactory::instanceFor(pluginId);

    if (!factory) {
	std::cerr << "RealTimePluginTransform: No factory available for plugin id \""
		  << pluginId.toStdString() << "\"" << std::endl;
	return;
    }

    DenseTimeValueModel *input = getInput();
    if (!input) return;

    m_plugin = factory->instantiatePlugin(pluginId, 0, 0,
                                          m_input->getSampleRate(),
                                          m_context.blockSize,
                                          input->getChannelCount());

    if (!m_plugin) {
	std::cerr << "RealTimePluginTransform: Failed to instantiate plugin \""
		  << pluginId.toStdString() << "\"" << std::endl;
	return;
    }

    if (configurationXml != "") {
        PluginXml(m_plugin).setParametersFromXml(configurationXml);
    }

    if (m_outputNo >= 0 &&
        m_outputNo >= int(m_plugin->getControlOutputCount())) {
        std::cerr << "RealTimePluginTransform: Plugin has fewer than desired " << m_outputNo << " control outputs" << std::endl;
        return;
    }

    if (m_outputNo == -1) {

        size_t outputChannels = m_plugin->getAudioOutputCount();
        if (outputChannels > input->getChannelCount()) {
            outputChannels = input->getChannelCount();
        }

        WritableWaveFileModel *model = new WritableWaveFileModel
            (input->getSampleRate(), outputChannels);

        m_output = model;

    } else {
	
        SparseTimeValueModel *model = new SparseTimeValueModel
            (input->getSampleRate(), m_context.blockSize, 0.0, 0.0, false);

        if (units != "") model->setScaleUnits(units);

        m_output = model;
    }
}

RealTimePluginTransform::~RealTimePluginTransform()
{
    delete m_plugin;
}

DenseTimeValueModel *
RealTimePluginTransform::getInput()
{
    DenseTimeValueModel *dtvm =
	dynamic_cast<DenseTimeValueModel *>(getInputModel());
    if (!dtvm) {
	std::cerr << "RealTimePluginTransform::getInput: WARNING: Input model is not conformable to DenseTimeValueModel" << std::endl;
    }
    return dtvm;
}

void
RealTimePluginTransform::run()
{
    DenseTimeValueModel *input = getInput();
    if (!input) return;

    while (!input->isReady()) {
        if (dynamic_cast<WaveFileModel *>(input)) break; // no need to wait
        std::cerr << "RealTimePluginTransform::run: Waiting for input model to be ready..." << std::endl;
        sleep(1);
    }

    SparseTimeValueModel *stvm = dynamic_cast<SparseTimeValueModel *>(m_output);
    WritableWaveFileModel *wwfm = dynamic_cast<WritableWaveFileModel *>(m_output);
    if (!stvm && !wwfm) return;

    if (stvm && (m_outputNo >= int(m_plugin->getControlOutputCount()))) return;

    size_t sampleRate = input->getSampleRate();
    size_t channelCount = input->getChannelCount();
    if (!wwfm && m_context.channel != -1) channelCount = 1;

    size_t blockSize = m_plugin->getBufferSize();

    float **inbufs = m_plugin->getAudioInputBuffers();

    size_t startFrame = m_input->getStartFrame();
    size_t   endFrame = m_input->getEndFrame();
    size_t blockFrame = startFrame;

    size_t prevCompletion = 0;

    size_t latency = m_plugin->getLatency();

    while (blockFrame < endFrame && !m_abandoned) {

	size_t completion =
	    (((blockFrame - startFrame) / blockSize) * 99) /
	    (   (endFrame - startFrame) / blockSize);

	size_t got = 0;

	if (channelCount == 1) {
            if (inbufs && inbufs[0]) {
                got = input->getValues
                    (m_context.channel, blockFrame, blockFrame + blockSize, inbufs[0]);
                while (got < blockSize) {
                    inbufs[0][got++] = 0.0;
                }          
            }
            for (size_t ch = 1; ch < m_plugin->getAudioInputCount(); ++ch) {
                for (size_t i = 0; i < blockSize; ++i) {
                    inbufs[ch][i] = inbufs[0][i];
                }
            }
	} else {
	    for (size_t ch = 0; ch < channelCount; ++ch) {
                if (inbufs && inbufs[ch]) {
                    got = input->getValues
                        (ch, blockFrame, blockFrame + blockSize, inbufs[ch]);
                    while (got < blockSize) {
                        inbufs[ch][got++] = 0.0;
                    }
                }
	    }
            for (size_t ch = channelCount; ch < m_plugin->getAudioInputCount(); ++ch) {
                for (size_t i = 0; i < blockSize; ++i) {
                    inbufs[ch][i] = inbufs[ch % channelCount][i];
                }
            }
	}

/*
        std::cerr << "Input for plugin: " << m_plugin->getAudioInputCount() << " channels "<< std::endl;

        for (size_t ch = 0; ch < m_plugin->getAudioInputCount(); ++ch) {
            std::cerr << "Input channel " << ch << std::endl;
            for (size_t i = 0; i < 100; ++i) {
                std::cerr << inbufs[ch][i] << " ";
                if (isnan(inbufs[ch][i])) {
                    std::cerr << "\n\nWARNING: NaN in audio input" << std::endl;
                }
            }
        }
*/

        m_plugin->run(Vamp::RealTime::frame2RealTime(blockFrame, sampleRate));

        if (stvm) {

            float value = m_plugin->getControlOutputValue(m_outputNo);

            size_t pointFrame = blockFrame;
            if (pointFrame > latency) pointFrame -= latency;
            else pointFrame = 0;

            stvm->addPoint(SparseTimeValueModel::Point
                           (pointFrame, value, ""));

        } else if (wwfm) {

            float **outbufs = m_plugin->getAudioOutputBuffers();

            if (outbufs) {

                if (blockFrame >= latency) {
                    wwfm->addSamples(outbufs, blockSize);
                } else if (blockFrame + blockSize >= latency) {
                    size_t offset = latency - blockFrame;
                    size_t count = blockSize - offset;
                    float **tmp = new float *[channelCount];
                    for (size_t c = 0; c < channelCount; ++c) {
                        tmp[c] = outbufs[c] + offset;
                    }
                    wwfm->addSamples(tmp, count);
                    delete[] tmp;
                }
            }
        }

	if (blockFrame == startFrame || completion > prevCompletion) {
	    if (stvm) stvm->setCompletion(completion);
	    if (wwfm) wwfm->setCompletion(completion);
	    prevCompletion = completion;
	}
        
	blockFrame += blockSize;
    }

    if (m_abandoned) return;
    
    if (stvm) stvm->setCompletion(100);
    if (wwfm) wwfm->setCompletion(100);
}

