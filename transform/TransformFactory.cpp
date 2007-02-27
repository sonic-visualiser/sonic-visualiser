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

    // disambiguate plugins with similar names

    std::map<QString, int> names;
    std::map<QString, QString> pluginSources;
    std::map<QString, QString> pluginMakers;

    for (TransformDescriptionMap::iterator i = transforms.begin();
         i != transforms.end(); ++i) {

        TransformDesc desc = i->second;

        QString td = desc.name;
        QString tn = td.section(": ", 0, 0);
        QString pn = desc.identifier.section(":", 1, 1);

        if (pluginSources.find(tn) != pluginSources.end()) {
            if (pluginSources[tn] != pn && pluginMakers[tn] != desc.maker) {
                ++names[tn];
            }
        } else {
            ++names[tn];
            pluginSources[tn] = pn;
            pluginMakers[tn] = desc.maker;
        }
    }

    std::map<QString, int> counts;
    m_transforms.clear();

    for (TransformDescriptionMap::iterator i = transforms.begin();
         i != transforms.end(); ++i) {

        TransformDesc desc = i->second;
	QString identifier = desc.identifier;
        QString maker = desc.maker;

        QString td = desc.name;
        QString tn = td.section(": ", 0, 0);
        QString to = td.section(": ", 1);

	if (names[tn] > 1) {
            maker.replace(QRegExp(tr(" [\\(<].*$")), "");
	    tn = QString("%1 [%2]").arg(tn).arg(maker);
	}

        if (to != "") {
            desc.name = QString("%1: %2").arg(tn).arg(to);
        } else {
            desc.name = tn;
        }

	m_transforms[identifier] = desc;
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
		
	QString pluginName = plugin->getName().c_str();
        QString category = factory->getPluginCategory(pluginId);

	Vamp::Plugin::OutputList outputs =
	    plugin->getOutputDescriptors();

	for (size_t j = 0; j < outputs.size(); ++j) {

	    QString transformId = QString("%1:%2")
		    .arg(pluginId).arg(outputs[j].identifier.c_str());

	    QString userName;
            QString friendlyName;
            QString units = outputs[j].unit.c_str();
            QString description = plugin->getDescription().c_str();
            QString maker = plugin->getMaker().c_str();
            if (maker == "") maker = tr("<unknown maker>");

            if (description == "") {
                if (outputs.size() == 1) {
                    description = tr("Extract features using \"%1\" plugin (from %2)")
                        .arg(pluginName).arg(maker);
                } else {
                    description = tr("Extract features using \"%1\" output of \"%2\" plugin (from %3)")
                        .arg(outputs[j].name.c_str()).arg(pluginName).arg(maker);
                }
            } else {
                if (outputs.size() == 1) {
                    description = tr("%1 using \"%2\" plugin (from %3)")
                        .arg(description).arg(pluginName).arg(maker);
                } else {
                    description = tr("%1 using \"%2\" output of \"%3\" plugin (from %4)")
                        .arg(description).arg(outputs[j].name.c_str()).arg(pluginName).arg(maker);
                }
            }                    

	    if (outputs.size() == 1) {
		userName = pluginName;
                friendlyName = pluginName;
	    } else {
		userName = QString("%1: %2")
		    .arg(pluginName)
		    .arg(outputs[j].name.c_str());
                friendlyName = outputs[j].name.c_str();
	    }

            bool configurable = (!plugin->getPrograms().empty() ||
                                 !plugin->getParameterDescriptors().empty());

            std::cerr << "Adding feature extraction plugin transform: id = " << transformId.toStdString() << std::endl;

	    transforms[transformId] = 
                TransformDesc(tr("Analysis"),
                              category,
                              transformId,
                              userName,
                              friendlyName,
                              description,
                              maker,
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
	
	QString pluginName = descriptor->name.c_str();
        QString category = factory->getPluginCategory(pluginId);
        bool configurable = (descriptor->parameterCount > 0);
        QString maker = descriptor->maker.c_str();
        if (maker == "") maker = tr("<unknown maker>");

        if (descriptor->audioInputPortCount > 0) {

            for (size_t j = 0; j < descriptor->controlOutputPortCount; ++j) {

                QString transformId = QString("%1:%2").arg(pluginId).arg(j);
                QString userName;
                QString units;
                QString portName;

                if (j < descriptor->controlOutputPortNames.size() &&
                    descriptor->controlOutputPortNames[j] != "") {

                    portName = descriptor->controlOutputPortNames[j].c_str();

                    userName = tr("%1: %2")
                        .arg(pluginName)
                        .arg(portName);

                    if (unitRE.indexIn(portName) >= 0) {
                        units = unitRE.cap(1);
                    }

                } else if (descriptor->controlOutputPortCount > 1) {

                    userName = tr("%1: Output %2")
                        .arg(pluginName)
                        .arg(j + 1);

                } else {

                    userName = pluginName;
                }

                QString description;

                if (portName != "") {
                    description = tr("Extract \"%1\" data output from \"%2\" effect plugin (from %3)")
                        .arg(portName)
                        .arg(pluginName)
                        .arg(maker);
                } else {
                    description = tr("Extract data output %1 from \"%2\" effect plugin (from %3)")
                        .arg(j + 1)
                        .arg(pluginName)
                        .arg(maker);
                }

                transforms[transformId] = 
                    TransformDesc(tr("Effects Data"),
                                  category,
                                  transformId,
                                  userName,
                                  userName,
                                  description,
                                  maker,
                                  units,
                                  configurable);
            }
        }

        if (!descriptor->isSynth || descriptor->audioInputPortCount > 0) {

            if (descriptor->audioOutputPortCount > 0) {

                QString transformId = QString("%1:A").arg(pluginId);
                QString type = tr("Effects");

                QString description = tr("Transform audio signal with \"%1\" effect plugin (from %2)")
                    .arg(pluginName)
                    .arg(maker);

                if (descriptor->audioInputPortCount == 0) {
                    type = tr("Generators");
                    QString description = tr("Generate audio signal using \"%1\" plugin (from %2)")
                        .arg(pluginName)
                        .arg(maker);
                }

                transforms[transformId] =
                    TransformDesc(type,
                                  category,
                                  transformId,
                                  pluginName,
                                  pluginName,
                                  description,
                                  maker,
                                  "",
                                  configurable);
            }
        }
    }
}

QString
TransformFactory::getTransformName(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].name;
    } else return "";
}

QString
TransformFactory::getTransformFriendlyName(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].friendlyName;
    } else return "";
}

QString
TransformFactory::getTransformUnits(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].units;
    } else return "";
}

bool
TransformFactory::isTransformConfigurable(TransformId identifier)
{
    if (m_transforms.find(identifier) != m_transforms.end()) {
	return m_transforms[identifier].configurable;
    } else return false;
}

bool
TransformFactory::getTransformChannelRange(TransformId identifier,
                                           int &min, int &max)
{
    QString id = identifier.section(':', 0, 2);

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
TransformFactory::getChannelRange(TransformId identifier, Vamp::PluginBase *plugin,
                                  int &minChannels, int &maxChannels)
{
    Vamp::Plugin *vp = 0;
    if ((vp = dynamic_cast<Vamp::Plugin *>(plugin)) ||
        (vp = dynamic_cast<Vamp::PluginHostAdapter *>(plugin))) {
        minChannels = vp->getMinChannelCount();
        maxChannels = vp->getMaxChannelCount();
        return true;
    } else {
        return getTransformChannelRange(identifier, minChannels, maxChannels);
    }
}

Model *
TransformFactory::getConfigurationForTransform(TransformId identifier,
                                               const std::vector<Model *> &candidateInputModels,
                                               PluginTransform::ExecutionContext &context,
                                               QString &configurationXml,
                                               AudioCallbackPlaySource *source)
{
    if (candidateInputModels.empty()) return 0;

    //!!! This will need revision -- we'll have to have a callback
    //from the dialog for when the candidate input model is changed,
    //as we'll need to reinitialise the channel settings in the dialog
    Model *inputModel = candidateInputModels[0]; //!!! for now
    QStringList candidateModelNames;
    std::map<QString, Model *> modelMap;
    for (size_t i = 0; i < candidateInputModels.size(); ++i) {
        QString modelName = candidateInputModels[i]->objectName();
        QString origModelName = modelName;
        int dupcount = 1;
        while (modelMap.find(modelName) != modelMap.end()) {
            modelName = tr("%1 <%2>").arg(origModelName).arg(++dupcount);
        }
        modelMap[modelName] = candidateInputModels[i];
        candidateModelNames.push_back(modelName);
    }

    QString id = identifier.section(':', 0, 2);
    QString output = identifier.section(':', 3);
    QString outputLabel = "";
    QString outputDescription = "";
    
    bool ok = false;
    configurationXml = m_lastConfigurations[identifier];

//    std::cerr << "last configuration: " << configurationXml.toStdString() << std::endl;

    Vamp::PluginBase *plugin = 0;

    bool frequency = false;
    bool effect = false;
    bool generator = false;

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel->getSampleRate());

        if (vp) {

            plugin = vp;
            frequency = (vp->getInputDomain() == Vamp::Plugin::FrequencyDomain);

            std::vector<Vamp::Plugin::OutputDescriptor> od =
                vp->getOutputDescriptors();
            if (od.size() > 1) {
                for (size_t i = 0; i < od.size(); ++i) {
                    if (od[i].identifier == output.toStdString()) {
                        outputLabel = od[i].name.c_str();
                        outputDescription = od[i].description.c_str();
                        break;
                    }
                }
            }
        }

    } else if (RealTimePluginFactory::instanceFor(id)) {

        RealTimePluginFactory *factory = RealTimePluginFactory::instanceFor(id);
        const RealTimePluginDescriptor *desc = factory->getPluginDescriptor(id);

        if (desc->audioInputPortCount > 0 && 
            desc->audioOutputPortCount > 0 &&
            !desc->isSynth) {
            effect = true;
        }

        if (desc->audioInputPortCount == 0) {
            generator = true;
        }

        if (output != "A") {
            int outputNo = output.toInt();
            if (outputNo >= 0 && outputNo < desc->controlOutputPortCount) {
                outputLabel = desc->controlOutputPortNames[outputNo].c_str();
            }
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
        getChannelRange(identifier, plugin, minChannels, maxChannels);

        int targetChannels = sourceChannels;
        if (!effect) {
            if (sourceChannels < minChannels) targetChannels = minChannels;
            if (sourceChannels > maxChannels) targetChannels = maxChannels;
        }

        int defaultChannel = context.channel;

        PluginParameterDialog *dialog = new PluginParameterDialog(plugin);

        if (candidateModelNames.size() > 1 && !generator) {
            dialog->setCandidateInputModels(candidateModelNames);
        }

        if (targetChannels > 0) {
            dialog->setChannelArrangement(sourceChannels, targetChannels,
                                          defaultChannel);
        }
        
        dialog->setOutputLabel(outputLabel, outputDescription);
        
        dialog->setShowProcessingOptions(true, frequency);

        if (dialog->exec() == QDialog::Accepted) {
            ok = true;
        }

        QString selectedInput = dialog->getInputModel();
        if (selectedInput != "") {
            if (modelMap.find(selectedInput) != modelMap.end()) {
                inputModel = modelMap[selectedInput];
                std::cerr << "Found selected input \"" << selectedInput.toStdString() << "\" in model map, result is " << inputModel << std::endl;
            } else {
                std::cerr << "Failed to find selected input \"" << selectedInput.toStdString() << "\" in model map" << std::endl;
            }
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

    if (ok) m_lastConfigurations[identifier] = configurationXml;

    return ok ? inputModel : 0;
}

PluginTransform::ExecutionContext
TransformFactory::getDefaultContextForTransform(TransformId identifier,
                                                Model *inputModel)
{
    PluginTransform::ExecutionContext context(-1);

    QString id = identifier.section(':', 0, 2);

    if (FeatureExtractionPluginFactory::instanceFor(id)) {

        Vamp::Plugin *vp =
            FeatureExtractionPluginFactory::instanceFor(id)->instantiatePlugin
            (id, inputModel ? inputModel->getSampleRate() : 48000);

        if (vp) context = PluginTransform::ExecutionContext(-1, vp);

    }

    return context;
}

Transform *
TransformFactory::createTransform(TransformId identifier, Model *inputModel,
                                  const PluginTransform::ExecutionContext &context,
                                  QString configurationXml, bool start)
{
    Transform *transform = 0;

    QString id = identifier.section(':', 0, 2);
    QString output = identifier.section(':', 3);

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
                                                getTransformUnits(identifier),
                                                output == "A" ? -1 :
                                                output.toInt());
    } else {
        std::cerr << "TransformFactory::createTransform: Unknown transform \""
                  << identifier.toStdString() << "\"" << std::endl;
        return transform;
    }

    if (start && transform) transform->start();
    transform->setObjectName(identifier);
    return transform;
}

Model *
TransformFactory::transform(TransformId identifier, Model *inputModel,
                            const PluginTransform::ExecutionContext &context,
                            QString configurationXml)
{
    Transform *t = createTransform(identifier, inputModel, context,
                                   configurationXml, false);

    if (!t) return 0;

    connect(t, SIGNAL(finished()), this, SLOT(transformFinished()));

    t->start();
    Model *model = t->detachOutputModel();

    if (model) {
        QString imn = inputModel->objectName();
        QString trn = getTransformFriendlyName(identifier);
        if (imn != "") {
            if (trn != "") {
                model->setObjectName(tr("%1: %2").arg(imn).arg(trn));
            } else {
                model->setObjectName(imn);
            }
        } else if (trn != "") {
            model->setObjectName(trn);
        }
    }

    return model;
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

