
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

#include "RealTimePluginTransform.h"

#include "plugin/RealTimePluginFactory.h"
#include "plugin/RealTimePluginInstance.h"
#include "plugin/PluginXml.h"

#include "base/Model.h"
#include "model/SparseTimeValueModel.h"
#include "model/DenseTimeValueModel.h"

#include <iostream>

RealTimePluginTransform::RealTimePluginTransform(Model *inputModel,
                                                 QString pluginId,
                                                 int channel,
                                                 QString configurationXml,
                                                 QString units,
                                                 int output) :
    Transform(inputModel),
    m_plugin(0),
    m_channel(channel),
    m_outputNo(output)
{
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

    m_plugin = factory->instantiatePlugin(pluginId, 0, 0, m_input->getSampleRate(),
                                          1024, //!!! wants to be configurable
                                          input->getChannelCount());

    if (!m_plugin) {
	std::cerr << "RealTimePluginTransform: Failed to instantiate plugin \""
		  << pluginId.toStdString() << "\"" << std::endl;
	return;
    }

    if (configurationXml != "") {
        PluginXml(m_plugin).setParametersFromXml(configurationXml);
    }

    if (m_outputNo >= m_plugin->getControlOutputCount()) {
        std::cerr << "RealTimePluginTransform: Plugin has fewer than desired " << m_outputNo << " control outputs" << std::endl;
        return;
    }
	
    SparseTimeValueModel *model = new SparseTimeValueModel
        (input->getSampleRate(), 1024, //!!!
         0.0, 0.0, false);

    if (units != "") model->setScaleUnits(units);

    m_output = model;
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

    SparseTimeValueModel *model = dynamic_cast<SparseTimeValueModel *>(m_output);
    if (!model) return;

    if (m_outputNo >= m_plugin->getControlOutputCount()) return;

    size_t sampleRate = input->getSampleRate();
    int channelCount = input->getChannelCount();
    if (m_channel != -1) channelCount = 1;

    size_t blockSize = m_plugin->getBufferSize();

    float **buffers = m_plugin->getAudioInputBuffers();

    size_t startFrame = m_input->getStartFrame();
    size_t   endFrame = m_input->getEndFrame();
    size_t blockFrame = startFrame;

    size_t prevCompletion = 0;

    int i = 0;

    while (blockFrame < endFrame) {

	size_t completion =
	    (((blockFrame - startFrame) / blockSize) * 99) /
	    (   (endFrame - startFrame) / blockSize);

	size_t got = 0;

	if (channelCount == 1) {
	    got = input->getValues
		(m_channel, blockFrame, blockFrame + blockSize, buffers[0]);
	    while (got < blockSize) {
		buffers[0][got++] = 0.0;
	    }
            if (m_channel == -1 && channelCount > 1) {
                // use mean instead of sum, as plugin input
                for (size_t i = 0; i < got; ++i) {
                    buffers[0][i] /= channelCount;
                }
            }                
	} else {
	    for (size_t ch = 0; ch < channelCount; ++ch) {
		got = input->getValues
		    (ch, blockFrame, blockFrame + blockSize, buffers[ch]);
		while (got < blockSize) {
		    buffers[ch][got++] = 0.0;
		}
	    }
	}

        m_plugin->run(Vamp::RealTime::frame2RealTime(blockFrame, sampleRate));

        float value = m_plugin->getControlOutputValue(m_outputNo);

	model->addPoint(SparseTimeValueModel::Point
                        (blockFrame - m_plugin->getLatency(), value, ""));

	if (blockFrame == startFrame || completion > prevCompletion) {
	    model->setCompletion(completion);
	    prevCompletion = completion;
	}
        
	blockFrame += blockSize;
    }
    
    model->setCompletion(100);
}

