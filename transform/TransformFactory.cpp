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

#include "TransformFactory.h"

#include "FeatureExtractionPluginTransform.h"
#include "RealTimePluginTransform.h"

#include "plugin/FeatureExtractionPluginFactory.h"
#include "plugin/RealTimePluginFactory.h"
#include "plugin/PluginXml.h"

#include "widgets/PluginParameterDialog.h"

#include "data/model/DenseTimeValueModel.h"

#include "vamp-sdk/PluginHostAdapter.h"

#include "sv/audioio/AudioCallbackPlaySource.h" //!!! shouldn't include here

#include <iostream>
#include <set>

#include <QRegExp>

TransformFactory *
TransformFactory::m_instance = new TransformFactory;

TransformFactory *
TransformFactory::getInstance()
{
    return m_instance;
}

TransformFactory::~TransformFactory()
{
}

TransformFactory::TransformList
TransformFactory::getAllTransforms()
{
    if (m_transforms.empty()) populateTransforms();

    std::set<TransformDesc> dset;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
	dset.insert(i->second);
    }

    TransformList list;
    for (std::set<TransformDesc>::const_iterator i = dset.begin();
	 i != dset.end(); ++i) {
	list.push_back(*i);
    }

    return list;
}

std::vector<QString>
TransformFactory::getAllTransformTypes()
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> types;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
	 i != m_transforms.end(); ++i) {
        types.insert(i->second.type);
    }

    std::vector<QString> rv;
    for (std::set<QString>::iterator i = types.begin(); i != types.end(); ++i) {
        rv.push_back(*i);
    }

    return rv;
}

std::vector<QString>
TransformFactory::getTransformCategories(QString transformType)
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> categories;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            categories.insert(i->second.category);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (std::set<QString>::iterator i = categories.begin(); 
         i != categories.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

std::vector<QString>
TransformFactory::getTransformMakers(QString transformType)
{
    if (m_transforms.empty()) populateTransforms();

    std::set<QString> makers;
    for (TransformDescriptionMap::const_iterator i = m_transforms.begin();
         i != m_transforms.end(); ++i) {
        if (i->second.type == transformType) {
            makers.insert(i->second.maker);
        }
    }

    bool haveEmpty = false;
    
    std::vector<QString> rv;
    for (std::set<QString>::iterator i = makers.begin(); 
         i != makers.end(); ++i) {
        if (*i != "") rv.push_back(*i);
        else haveEmpty = true;
    }

    if (haveEmpty) rv.push_back(""); // make sure empty category sorts last

    return rv;
}

void
TransformFactory::populateTransforms()
{
    TransformDescriptionMap transforms;

    populateFeatureExtractionPlugins(transforms);
    populateRealTimePlugins(transforms);

    // disambiguate plugins with similar descriptions

    std::map<QString, int> descriptions;

    for (TransformDescriptionMap::iterator i = transforms.begin();
         i != transforms.end(); ++i) {

        TransformDesc desc = i->second;

	++descriptions[desc.description];
	++descriptions[QString("%1 [%2]").arg(desc.description).arg(desc.maker)];
    }

    std::map<QString, int> counts;
    m_transforms.clear();

    for (TransformDescriptionMap::iterator i = transforms.begin();
         i != transforms.end(); ++i) {

        TransformDesc desc = i->second;
	QString name = desc.name;
        QString description = desc.description;
        QString maker = desc.maker;

	if (descriptions[description] > 1) {
	    description = QString("%1 [%2]").arg(description).arg(maker);
	    if (descriptions[description] > 1) {
		description = QString("%1 <%2>")
		    .arg(description).arg(++counts[description]);
	    }
	}

        desc.description = description;
	m_transforms[name] = desc;
    }	    
}

void
TransformFactory::populateFeatureExtractionPlugins(TransformDescriptionMap &transforms)
{
    std::vector<QString> plugs =
	FeatureExtractionPluginFactory::getAllPluginIdentifiers();

    for (size_t i = 0; i < plugs.size(); ++i) {

	QString pluginId = plugs[i];

	FeatureExtractionPluginFactory *factory =
	    FeatureExtractionPluginFactory::instanceFor(pluginId);

	if (!factory) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: No feature extraction plugin factory for instance " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}

	Vamp::Plugin *plugin = 
	    factory->instantiatePlugin(pluginId, 48000);

	if (!plugin) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: Failed to instantiate plugin " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}
		
	QString pluginDescription = plugin->getDescription().c_str();
        QString category = factory->getPluginCategory(pluginId);

	Vamp::Plugin::OutputList outputs =
	    plugin->getOutputDescriptors();

	for (size_t j = 0; j < outputs.size(); ++j) {

	    QString transformName = QString("%1:%2")
		    .arg(pluginId).arg(outputs[j].name.c_str());

	    QString userDescription;
            QString friendlyName;
            QString units = outputs[j].unit.c_str();

	    if (outputs.size() == 1) {
		userDescription = pluginDescription;
                friendlyName = pluginDescription;
	    } else {
		userDescription = QString("%1: %2")
		    .arg(pluginDescription)
		    .arg(outputs[j].description.c_str());
                friendlyName = outputs[j].description.c_str();
	    }

            bool configurable = (!plugin->getPrograms().empty() ||
                                 !plugin->getParameterDescriptors().empty());

	    transforms[transformName] = 
                TransformDesc(tr("Analysis"),
                              category,
                              transformName,
                              userDescription,
                              friendlyName,
                              plugin->getMaker().c_str(),
                              units,
                              configurable);
	}
    }
}

void
TransformFactory::populateRealTimePlugins(TransformDescriptionMap &transforms)
{
    std::vector<QString> plugs =
	RealTimePluginFactory::getAllPluginIdentifiers();

    static QRegExp unitRE("[\\[\\(]([A-Za-z0-9/]+)[\\)\\]]$");

    for (size_t i = 0; i < plugs.size(); ++i) {
        
	QString pluginId = plugs[i];

        RealTimePluginFactory *factory =
            RealTimePluginFactory::instanceFor(pluginId);

	if (!factory) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: No real time plugin factory for instance " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}

        const RealTimePluginDescriptor *descriptor =
            factory->getPluginDescriptor(pluginId);

        if (!descriptor) {
	    std::cerr << "WARNING: TransformFactory::populateTransforms: Failed to query plugin " << pluginId.toLocal8Bit().data() << std::endl;
	    continue;
	}
	
//!!!        if (descriptor->controlOutputPortCount == 0 ||
//            descriptor->audioInputPortCount == 0) continue;

//        std::cout << "TransformFactory::populateRealTimePlugins: plugin " << pluginId.toStdString() << " has " << descriptor->controlOutputPortCount << " control output ports, " << descriptor->audioOutputPortCount << " audio outputs, " << descriptor->audioInputPortCount << " audio inputs" << std::endl;
	
	QString pluginDescription = descriptor->name.c_str();
        QString category = factory->getPluginCategory(pluginId);
        bool configurable = (descriptor->parameterCount > 0);

        if (descriptor->audioInputPortCount > 0) {

            for (size_t j = 0; j < descriptor->controlOutputPortCount; ++j) {

                QString transformName = QString("%1:%2").arg(pluginId).arg(j);
                QString userDescription;
                QString units;

                if (j < descriptor->controlOutputPortNames.size() &&
                    descriptor->controlOutputPortNames[j] != "") {

                    QString portName = descriptor->controlOutputPortNames[j].c_str();

                    userDescription = tr("%1: %2")
                        .arg(pluginDescription)
                        .arg(portName);

                    if (unitRE.indexIn(portName) >= 0) {
                        units = unitRE.cap(1);
                    }

                } else if (descriptor->controlOutputPortCount > 1) {

                    userDescription = tr("%1: Output %2")
                        .arg(pluginDescription)
                        .arg(j + 1);

                } else {

                    userDescription = pluginDescription;
                }


                transforms[transformName] = 
                    TransformDesc(tr("Effects Data"),
                                  category,
                                  transformName,
                                  userDescription,
                                  userDescription,
                                  descriptor->maker.c_str(),
                                  units,
                                  configurable);
            }
        }

        if (!descriptor->isSynth || descriptor->audioInputPortCount > 0) {

            if (descriptor->audioOutputPortCount > 0) {

                QString transformName = QString("%1:A").arg(pluginId);
                QString type = tr("Effects");
                if (descriptor->audioInputPortCount == 0) {
                    type = tr("Generators");
                }

                transforms[transformName] =
                    TransformDesc(type,
                                  category,
                                  transformName,
                                  pluginDescription,
                                  pluginDescription,
                                  descriptor->maker.c_str(),
                                  "",
                                  configurable);
            }
        }
    }
}

QString
TransformFactory::getTransformDescription(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name].description;
    } else return "";
}

QString
TransformFactory::getTransformFriendlyName(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name].friendlyName;
    } else return "";
}

QString
TransformFactory::getTransformUnits(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name].units;
    } else return "";
}

bool
TransformFactory::isTransformConfigurable(TransformName name)
{
    if (m_transforms.find(name) != m_transforms.end()) {
	return m_transforms[name].configurable;
    } else return false;
}

bool
TransformFactory::getTransformChannelRange(TransformName name,
                                           int &min, int &max)
{
    QString id = name.section(':', 0, 2);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *plugin = 
            FeatureExtractionPluginFactory::instanceFor(id)->
            instantiatePlugin(id, 48000);
        if (!plugin) return false;

        min = plugin->getMinChannelCount();
        max = plugin->getMaxChannelCount();
        delete plugin;

        return true;

    } else if (RealTimePluginFactory::instanceFor(id)) {

        const RealTimePluginDescriptor *descriptor = 
            RealTimePluginFactory::instanceFor(id)->
            getPluginDescriptor(id);
        if (!descriptor) return false;

        min = descriptor->audioInputPortCount;
        max = descriptor->audioInputPortCount;

        return true;
    }

    return false;
}

bool
TransformFactory::getChannelRange(TransformName name, Vamp::PluginBase *plugin,
                                  int &minChannels, int &maxChannels)
{
    Vamp::Plugin *vp = 0;
    if ((vp = dynamic_cast<Vamp::Plugin *>(plugin)) ||
        (vp = dynamic_cast<Vamp::PluginHostAdapter *>(plugin))) {
        minChannels = vp->getMinChannelCount();
        maxChannels = vp->getMaxChannelCount();
        return true;
    } else {
        return getTransformChannelRange(name, minChannels, maxChannels);
    }
}

bool
TransformFactory::getConfigurationForTransform(TransformName name,
                                               Model *inputModel,
                                               PluginTransform::ExecutionContext &context,
                                               QString &configurationXml,
                                               AudioCallbackPlaySource *source)
{
    QString id = name.section(':', 0, 2);
    QString output = name.section(':', 3);
    
    bool ok = false;
    configurationXml = m_lastConfigurations[name];

//    std::cerr << "last configuration: " << configurationXml.toStdString() << std::endl;

    Vamp::PluginBase *plugin = 0;

    bool frequency = false;
    bool effect = false;

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel->getSampleRate());
        if (vp) {
            plugin = vp;
            frequency = (vp->getInputDomain() == Vamp::Plugin::FrequencyDomain);
        }

    } else if (RealTimePluginFactory::instanceFor(id)) {

        RealTimePluginFactory *factory = RealTimePluginFactory::instanceFor(id);
        const RealTimePluginDescriptor *desc = factory->getPluginDescriptor(id);

        if (desc->audioInputPortCount > 0 && 
            desc->audioOutputPortCount > 0 &&
            !desc->isSynth) {
            effect = true;
        }

        size_t sampleRate = inputModel->getSampleRate();
        size_t blockSize = 1024;
        size_t channels = 1;
        if (effect && source) {
            sampleRate = source->getTargetSampleRate();
            blockSize = source->getTargetBlockSize();
            channels = source->getTargetChannelCount();
        }

        RealTimePluginInstance *rtp = factory->instantiatePlugin
            (id, 0, 0, sampleRate, blockSize, channels);

        plugin = rtp;

        if (effect && source && rtp) {
            source->setAuditioningPlugin(rtp);
        }
    }

    if (plugin) {

        context = PluginTransform::ExecutionContext(context.channel, plugin);

        if (configurationXml != "") {
            PluginXml(plugin).setParametersFromXml(configurationXml);
        }

        int sourceChannels = 1;
        if (dynamic_cast<DenseTimeValueModel *>(inputModel)) {
            sourceChannels = dynamic_cast<DenseTimeValueModel *>(inputModel)
                ->getChannelCount();
        }

        int minChannels = 1, maxChannels = sourceChannels;
        getChannelRange(name, plugin, minChannels, maxChannels);

        int targetChannels = sourceChannels;
        if (!effect) {
            if (sourceChannels < minChannels) targetChannels = minChannels;
            if (sourceChannels > maxChannels) targetChannels = maxChannels;
        }

        int defaultChannel = context.channel;

        PluginParameterDialog *dialog = new PluginParameterDialog(plugin,
                                                                  sourceChannels,
                                                                  targetChannels,
                                                                  defaultChannel,
                                                                  output,
                                                                  true,
                                                                  frequency);
        if (dialog->exec() == QDialog::Accepted) {
            ok = true;
        }

        configurationXml = PluginXml(plugin).toXmlString();
        context.channel = dialog->getChannel();

        dialog->getProcessingParameters(context.stepSize,
                                        context.blockSize,
                                        context.windowType);

        context.makeConsistentWithPlugin(plugin);

        delete dialog;

        if (effect && source) {
            source->setAuditioningPlugin(0); // will delete our plugin
        } else {
            delete plugin;
        }
    }

    if (ok) m_lastConfigurations[name] = configurationXml;

    return ok;
}

Transform *
TransformFactory::createTransform(TransformName name, Model *inputModel,
                                  const PluginTransform::ExecutionContext &context,
                                  QString configurationXml, bool start)
{
    Transform *transform = 0;

    QString id = name.section(':', 0, 2);
    QString output = name.section(':', 3);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {
        transform = new FeatureExtractionPluginTransform(inputModel,
                                                         id,
                                                         context,
                                                         configurationXml,
                                                         output);
    } else if (RealTimePluginFactory::instanceFor(id)) {
        transform = new RealTimePluginTransform(inputModel,
                                                id,
                                                context,
                                                configurationXml,
                                                getTransformUnits(name),
                                                output == "A" ? -1 :
                                                output.toInt());
    } else {
        std::cerr << "TransformFactory::createTransform: Unknown transform \""
                  << name.toStdString() << "\"" << std::endl;
        return transform;
    }

    if (start && transform) transform->start();
    transform->setObjectName(name);
    return transform;
}

Model *
TransformFactory::transform(TransformName name, Model *inputModel,
                            const PluginTransform::ExecutionContext &context,
                            QString configurationXml)
{
    Transform *t = createTransform(name, inputModel, context,
                                   configurationXml, false);

    if (!t) return 0;

    connect(t, SIGNAL(finished()), this, SLOT(transformFinished()));

    t->start();
    return t->detachOutputModel();
}

void
TransformFactory::transformFinished()
{
    QObject *s = sender();
    Transform *transform = dynamic_cast<Transform *>(s);
    
    if (!transform) {
	std::cerr << "WARNING: TransformFactory::transformFinished: sender is not a transform" << std::endl;
	return;
    }

    transform->wait(); // unnecessary but reassuring
    delete transform;
}

