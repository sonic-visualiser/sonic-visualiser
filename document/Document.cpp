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

#include "Document.h"

#include "data/model/WaveFileModel.h"
#include "layer/Layer.h"
#include "base/CommandHistory.h"
#include "base/Command.h"
#include "view/View.h"
#include "base/PlayParameterRepository.h"
#include "base/PlayParameters.h"
#include "transform/TransformFactory.h"
#include <iostream>

//!!! still need to handle command history, documentRestored/documentModified

Document::Document() :
    m_mainModel(0)
{
}

Document::~Document()
{
    //!!! Document should really own the command history.  atm we
    //still refer to it in various places that don't have access to
    //the document, be nice to fix that

//    std::cerr << "\n\nDocument::~Document: about to clear command history" << std::endl;
    CommandHistory::getInstance()->clear();
    
//    std::cerr << "Document::~Document: about to delete layers" << std::endl;
    while (!m_layers.empty()) {
	deleteLayer(*m_layers.begin(), true);
    }

    if (!m_models.empty()) {
	std::cerr << "Document::~Document: WARNING: " 
		  << m_models.size() << " model(s) still remain -- "
		  << "should have been garbage collected when deleting layers"
		  << std::endl;
	while (!m_models.empty()) {
	    if (m_models.begin()->first == m_mainModel) {
		// just in case!
		std::cerr << "Document::~Document: WARNING: Main model is also"
			  << " in models list!" << std::endl;
	    } else {
		emit modelAboutToBeDeleted(m_models.begin()->first);
		delete m_models.begin()->first;
	    }
	    m_models.erase(m_models.begin());
	}
    }

//    std::cerr << "Document::~Document: About to get rid of main model"
//	      << std::endl;
    emit modelAboutToBeDeleted(m_mainModel);
    emit mainModelChanged(0);
    delete m_mainModel;

}

Layer *
Document::createLayer(LayerFactory::LayerType type)
{
    Layer *newLayer = LayerFactory::getInstance()->createLayer(type);
    if (!newLayer) return 0;

    newLayer->setObjectName(getUniqueLayerName(newLayer->objectName()));

    m_layers.insert(newLayer);
    emit layerAdded(newLayer);

    return newLayer;
}

Layer *
Document::createMainModelLayer(LayerFactory::LayerType type)
{
    Layer *newLayer = createLayer(type);
    if (!newLayer) return 0;
    setModel(newLayer, m_mainModel);
    return newLayer;
}

Layer *
Document::createImportedLayer(Model *model)
{
    LayerFactory::LayerTypeSet types =
	LayerFactory::getInstance()->getValidLayerTypes(model);

    if (types.empty()) {
	std::cerr << "WARNING: Document::importLayer: no valid display layer for model" << std::endl;
	return 0;
    }

    //!!! for now, just use the first suitable layer type
    LayerFactory::LayerType type = *types.begin();

    Layer *newLayer = LayerFactory::getInstance()->createLayer(type);
    if (!newLayer) return 0;

    newLayer->setObjectName(getUniqueLayerName(newLayer->objectName()));

    addImportedModel(model);
    setModel(newLayer, model);

    //!!! and all channels
    setChannel(newLayer, -1);

    m_layers.insert(newLayer);
    emit layerAdded(newLayer);
    return newLayer;
}

Layer *
Document::createEmptyLayer(LayerFactory::LayerType type)
{
    Model *newModel =
	LayerFactory::getInstance()->createEmptyModel(type, m_mainModel);
    if (!newModel) return 0;

    Layer *newLayer = createLayer(type);
    if (!newLayer) {
	delete newModel;
	return 0;
    }

    addImportedModel(newModel);
    setModel(newLayer, newModel);

    return newLayer;
}

Layer *
Document::createDerivedLayer(LayerFactory::LayerType type,
			     TransformName transform)
{
    Layer *newLayer = createLayer(type);
    if (!newLayer) return 0;

    newLayer->setObjectName(getUniqueLayerName
                            (TransformFactory::getInstance()->
                             getTransformFriendlyName(transform)));

    return newLayer;
}

Layer *
Document::createDerivedLayer(TransformName transform,
                             Model *inputModel, 
                             const PluginTransform::ExecutionContext &context,
                             QString configurationXml)
{
    Model *newModel = createModelForTransform(transform, inputModel,
                                              context, configurationXml);
    if (!newModel) {
        // error already printed to stderr by createModelForTransform
        emit modelGenerationFailed(transform);
        return 0;
    }

    LayerFactory::LayerTypeSet types =
	LayerFactory::getInstance()->getValidLayerTypes(newModel);

    if (types.empty()) {
	std::cerr << "WARNING: Document::createLayerForTransform: no valid display layer for output of transform " << transform.toStdString() << std::endl;
	delete newModel;
	return 0;
    }

    //!!! for now, just use the first suitable layer type

    Layer *newLayer = createLayer(*types.begin());
    setModel(newLayer, newModel);

    //!!! We need to clone the model when adding the layer, so that it
    //can be edited without affecting other layers that are based on
    //the same model.  Unfortunately we can't just clone it now,
    //because it probably hasn't been completed yet -- the transform
    //runs in the background.  Maybe the transform has to handle
    //cloning and cacheing models itself.
    //
    // Once we do clone models here, of course, we'll have to avoid
    // leaking them too.
    //
    // We want the user to be able to add a model to a second layer
    // _while it's still being calculated in the first_ and have it
    // work quickly.  That means we need to put the same physical
    // model pointer in both layers, so they can't actually be cloned.
    
    if (newLayer) {
	newLayer->setObjectName(getUniqueLayerName
                                (TransformFactory::getInstance()->
                                 getTransformFriendlyName(transform)));
    }

    emit layerAdded(newLayer);
    return newLayer;
}

void
Document::setMainModel(WaveFileModel *model)
{
    Model *oldMainModel = m_mainModel;
    m_mainModel = model;

    emit modelAdded(m_mainModel);

    std::vector<Layer *> obsoleteLayers;
    std::set<QString> failedTransforms;

    // We need to ensure that no layer is left using oldMainModel or
    // any of the old derived models as its model.  Either replace the
    // model, or delete the layer for each layer that is currently
    // using one of these.  Carry out this replacement before we
    // delete any of the models.

    for (LayerSet::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {

	Layer *layer = *i;
	Model *model = layer->getModel();

        std::cerr << "Document::setMainModel: inspecting model "
                  << (model ? model->objectName().toStdString() : "(null)") << " in layer "
                  << layer->objectName().toStdString() << std::endl;

	if (model == oldMainModel) {
            std::cerr << "... it uses the old main model, replacing" << std::endl;
	    LayerFactory::getInstance()->setModel(layer, m_mainModel);
	    continue;
	}

	if (m_models.find(model) == m_models.end()) {
	    std::cerr << "WARNING: Document::setMainModel: Unknown model "
		      << model << " in layer " << layer << std::endl;
	    // get rid of this hideous degenerate
	    obsoleteLayers.push_back(layer);
	    continue;
	}
	    
	if (m_models[model].source == oldMainModel) {

            std::cerr << "... it uses a model derived from the old main model, regenerating" << std::endl;

	    // This model was derived from the previous main
	    // model: regenerate it.
	    
	    TransformName transform = m_models[model].transform;
            PluginTransform::ExecutionContext context = m_models[model].context;
	    
	    Model *replacementModel =
                createModelForTransform(transform,
                                        m_mainModel,
                                        context,
                                        m_models[model].configurationXml);
	    
	    if (!replacementModel) {
		std::cerr << "WARNING: Document::setMainModel: Failed to regenerate model for transform \""
			  << transform.toStdString() << "\"" << " in layer " << layer << std::endl;
                if (failedTransforms.find(transform) == failedTransforms.end()) {
                    emit modelRegenerationFailed(layer->objectName(),
                                                 transform);
                    failedTransforms.insert(transform);
                }
		obsoleteLayers.push_back(layer);
	    } else {
                std::cerr << "Replacing model " << model << " (type "
                          << typeid(*model).name() << ") with model "
                          << replacementModel << " (type "
                          << typeid(*replacementModel).name() << ") in layer "
                          << layer << " (name " << layer->objectName().toStdString() << ")"
                          << std::endl;
                RangeSummarisableTimeValueModel *rm =
                    dynamic_cast<RangeSummarisableTimeValueModel *>(replacementModel);
                if (rm) {
                    std::cerr << "new model has " << rm->getChannelCount() << " channels " << std::endl;
                } else {
                    std::cerr << "new model is not a RangeSummarisableTimeValueModel!" << std::endl;
                }
		setModel(layer, replacementModel);
	    }
	}	    
    }

    for (size_t k = 0; k < obsoleteLayers.size(); ++k) {
	deleteLayer(obsoleteLayers[k], true);
    }

    emit mainModelChanged(m_mainModel);

    // we already emitted modelAboutToBeDeleted for this
    delete oldMainModel;
}

void
Document::addDerivedModel(TransformName transform,
                          Model *inputModel,
                          const PluginTransform::ExecutionContext &context,
                          Model *outputModelToAdd,
                          QString configurationXml)
{
    if (m_models.find(outputModelToAdd) != m_models.end()) {
	std::cerr << "WARNING: Document::addDerivedModel: Model already added"
		  << std::endl;
	return;
    }

    std::cerr << "Document::addDerivedModel: source is " << inputModel << " \"" << inputModel->objectName().toStdString() << std::endl;

    ModelRecord rec;
    rec.source = inputModel;
    rec.transform = transform;
    rec.context = context;
    rec.configurationXml = configurationXml;
    rec.refcount = 0;

    m_models[outputModelToAdd] = rec;

    emit modelAdded(outputModelToAdd);
}


void
Document::addImportedModel(Model *model)
{
    if (m_models.find(model) != m_models.end()) {
	std::cerr << "WARNING: Document::addImportedModel: Model already added"
		  << std::endl;
	return;
    }

    ModelRecord rec;
    rec.source = 0;
    rec.transform = "";
    rec.refcount = 0;

    m_models[model] = rec;

    emit modelAdded(model);
}

Model *
Document::createModelForTransform(TransformName transform,
                                  Model *inputModel,
                                  const PluginTransform::ExecutionContext &context,
                                  QString configurationXml)
{
    Model *model = 0;

    for (ModelMap::iterator i = m_models.begin(); i != m_models.end(); ++i) {
	if (i->second.transform == transform &&
	    i->second.source == inputModel && 
            i->second.context == context &&
            i->second.configurationXml == configurationXml) {
	    return i->first;
	}
    }

    model = TransformFactory::getInstance()->transform
	(transform, inputModel, context, configurationXml);

    if (!model) {
	std::cerr << "WARNING: Document::createModelForTransform: no output model for transform " << transform.toStdString() << std::endl;
    } else {
	addDerivedModel(transform, inputModel, context, model, configurationXml);
    }

    return model;
}

void
Document::releaseModel(Model *model) // Will _not_ release main model!
{
    if (model == 0) {
	return;
    }

    if (model == m_mainModel) {
	return;
    }

    bool toDelete = false;

    if (m_models.find(model) != m_models.end()) {
	
	if (m_models[model].refcount == 0) {
	    std::cerr << "WARNING: Document::releaseModel: model " << model
		      << " reference count is zero already!" << std::endl;
	} else {
	    if (--m_models[model].refcount == 0) {
		toDelete = true;
	    }
	}
    } else { 
	std::cerr << "WARNING: Document::releaseModel: Unfound model "
		  << model << std::endl;
	toDelete = true;
    }

    if (toDelete) {

	int sourceCount = 0;

	for (ModelMap::iterator i = m_models.begin(); i != m_models.end(); ++i) {
	    if (i->second.source == model) {
		++sourceCount;
		i->second.source = 0;
	    }
	}

	if (sourceCount > 0) {
	    std::cerr << "Document::releaseModel: Deleting model "
		      << model << " even though it is source for "
		      << sourceCount << " other derived model(s) -- resetting "
		      << "their source fields appropriately" << std::endl;
	}

	emit modelAboutToBeDeleted(model);
	m_models.erase(model);
	delete model;
    }
}

void
Document::deleteLayer(Layer *layer, bool force)
{
    if (m_layerViewMap.find(layer) != m_layerViewMap.end() &&
	m_layerViewMap[layer].size() > 0) {

	std::cerr << "WARNING: Document::deleteLayer: Layer "
		  << layer << " [" << layer->objectName().toStdString() << "]"
		  << " is still used in " << m_layerViewMap[layer].size()
		  << " views!" << std::endl;

	if (force) {

	    std::cerr << "(force flag set -- deleting from all views)" << std::endl;

	    for (std::set<View *>::iterator j = m_layerViewMap[layer].begin();
		 j != m_layerViewMap[layer].end(); ++j) {
		// don't use removeLayerFromView, as it issues a command
		layer->setLayerDormant(*j, true);
		(*j)->removeLayer(layer);
	    }
	    
	    m_layerViewMap.erase(layer);

	} else {
	    return;
	}
    }

    if (m_layers.find(layer) == m_layers.end()) {
	std::cerr << "Document::deleteLayer: Layer "
		  << layer << " does not exist, or has already been deleted "
		  << "(this may not be as serious as it sounds)" << std::endl;
	return;
    }

    m_layers.erase(layer);

    releaseModel(layer->getModel());
    emit layerRemoved(layer);
    emit layerAboutToBeDeleted(layer);
    delete layer;
}

void
Document::setModel(Layer *layer, Model *model)
{
    if (model && 
	model != m_mainModel &&
	m_models.find(model) == m_models.end()) {
	std::cerr << "ERROR: Document::setModel: Layer " << layer
		  << " is using unregistered model " << model
		  << ": register the layer's model before setting it!"
		  << std::endl;
	return;
    }

    Model *previousModel = layer->getModel();

    if (previousModel == model) {
        std::cerr << "WARNING: Document::setModel: Layer is already set to this model" << std::endl;
        return;
    }

    if (model && model != m_mainModel) {
	m_models[model].refcount ++;
    }

    LayerFactory::getInstance()->setModel(layer, model);

    if (previousModel) {
        releaseModel(previousModel);
    }
}

void
Document::setChannel(Layer *layer, int channel)
{
    LayerFactory::getInstance()->setChannel(layer, channel);
}

void
Document::addLayerToView(View *view, Layer *layer)
{
    Model *model = layer->getModel();
    if (!model) {
	std::cerr << "Document::addLayerToView: Layer with no model being added to view: normally you want to set the model first" << std::endl;
    } else {
	if (model != m_mainModel &&
	    m_models.find(model) == m_models.end()) {
	    std::cerr << "ERROR: Document::addLayerToView: Layer " << layer
		      << " has unregistered model " << model
		      << " -- register the layer's model before adding the layer!" << std::endl;
	    return;
	}
    }

    CommandHistory::getInstance()->addCommand
	(new Document::AddLayerCommand(this, view, layer));
}

void
Document::removeLayerFromView(View *view, Layer *layer)
{
    CommandHistory::getInstance()->addCommand
	(new Document::RemoveLayerCommand(this, view, layer));
}

void
Document::addToLayerViewMap(Layer *layer, View *view)
{
    bool firstView = (m_layerViewMap.find(layer) == m_layerViewMap.end() ||
                      m_layerViewMap[layer].empty());

    if (m_layerViewMap[layer].find(view) !=
	m_layerViewMap[layer].end()) {
	std::cerr << "WARNING: Document::addToLayerViewMap:"
		  << " Layer " << layer << " -> view " << view << " already in"
		  << " layer view map -- internal inconsistency" << std::endl;
    }

    m_layerViewMap[layer].insert(view);

    if (firstView) emit layerInAView(layer, true);
}
    
void
Document::removeFromLayerViewMap(Layer *layer, View *view)
{
    if (m_layerViewMap[layer].find(view) ==
	m_layerViewMap[layer].end()) {
	std::cerr << "WARNING: Document::removeFromLayerViewMap:"
		  << " Layer " << layer << " -> view " << view << " not in"
		  << " layer view map -- internal inconsistency" << std::endl;
    }

    m_layerViewMap[layer].erase(view);

    if (m_layerViewMap[layer].empty()) {
        m_layerViewMap.erase(layer);
        emit layerInAView(layer, false);
    }
}

QString
Document::getUniqueLayerName(QString candidate)
{
    for (int count = 1; ; ++count) {

        QString adjusted =
            (count > 1 ? QString("%1 <%2>").arg(candidate).arg(count) :
             candidate);
        
        bool duplicate = false;

        for (LayerSet::iterator i = m_layers.begin(); i != m_layers.end(); ++i) {
            if ((*i)->objectName() == adjusted) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) return adjusted;
    }
}

Document::AddLayerCommand::AddLayerCommand(Document *d,
					   View *view,
					   Layer *layer) :
    m_d(d),
    m_view(view),
    m_layer(layer),
    m_name(d->tr("Add %1 Layer").arg(layer->objectName())),
    m_added(false)
{
}

Document::AddLayerCommand::~AddLayerCommand()
{
//    std::cerr << "Document::AddLayerCommand::~AddLayerCommand" << std::endl;
    if (!m_added) {
	m_d->deleteLayer(m_layer);
    }
}

void
Document::AddLayerCommand::execute()
{
    for (int i = 0; i < m_view->getLayerCount(); ++i) {
	if (m_view->getLayer(i) == m_layer) {
	    // already there
	    m_layer->setLayerDormant(m_view, false);
	    m_added = true;
	    return;
	}
    }

    m_view->addLayer(m_layer);
    m_layer->setLayerDormant(m_view, false);

    m_d->addToLayerViewMap(m_layer, m_view);
    m_added = true;
}

void
Document::AddLayerCommand::unexecute()
{
    m_view->removeLayer(m_layer);
    m_layer->setLayerDormant(m_view, true);

    m_d->removeFromLayerViewMap(m_layer, m_view);
    m_added = false;
}

Document::RemoveLayerCommand::RemoveLayerCommand(Document *d,
						 View *view,
						 Layer *layer) :
    m_d(d),
    m_view(view),
    m_layer(layer),
    m_name(d->tr("Delete %1 Layer").arg(layer->objectName())),
    m_added(true)
{
}

Document::RemoveLayerCommand::~RemoveLayerCommand()
{
//    std::cerr << "Document::RemoveLayerCommand::~RemoveLayerCommand" << std::endl;
    if (!m_added) {
	m_d->deleteLayer(m_layer);
    }
}

void
Document::RemoveLayerCommand::execute()
{
    bool have = false;
    for (int i = 0; i < m_view->getLayerCount(); ++i) {
	if (m_view->getLayer(i) == m_layer) {
	    have = true;
	    break;
	}
    }

    if (!have) { // not there!
	m_layer->setLayerDormant(m_view, true);
	m_added = false;
	return;
    }

    m_view->removeLayer(m_layer);
    m_layer->setLayerDormant(m_view, true);

    m_d->removeFromLayerViewMap(m_layer, m_view);
    m_added = false;
}

void
Document::RemoveLayerCommand::unexecute()
{
    m_view->addLayer(m_layer);
    m_layer->setLayerDormant(m_view, false);

    m_d->addToLayerViewMap(m_layer, m_view);
    m_added = true;
}

void
Document::toXml(QTextStream &out, QString indent, QString extraAttributes) const
{
    out << indent + QString("<data%1%2>\n")
        .arg(extraAttributes == "" ? "" : " ").arg(extraAttributes);

    if (m_mainModel) {
	m_mainModel->toXml(out, indent + "  ", "mainModel=\"true\"");
    }

    for (ModelMap::const_iterator i = m_models.begin();
	 i != m_models.end(); ++i) {

	i->first->toXml(out, indent + "  ");
	
	const ModelRecord &rec = i->second;

	if (rec.source && rec.transform != "") {
	    
            //!!! stream the rest of the execution context in both directions (i.e. not just channel)

	    out << indent;
	    out << QString("  <derivation source=\"%1\" model=\"%2\" channel=\"%3\" domain=\"%4\" stepSize=\"%5\" blockSize=\"%6\" windowType=\"%7\" transform=\"%8\"")
		.arg(XmlExportable::getObjectExportId(rec.source))
		.arg(XmlExportable::getObjectExportId(i->first))
                .arg(rec.context.channel)
                .arg(rec.context.domain)
                .arg(rec.context.stepSize)
                .arg(rec.context.blockSize)
                .arg(int(rec.context.windowType))
		.arg(XmlExportable::encodeEntities(rec.transform));

            if (rec.configurationXml != "") {
                out << ">\n    " + indent + rec.configurationXml
                    + "\n" + indent + "  </derivation>\n";
            } else {
                out << "/>\n";
            }
	}

        //!!! We should probably own the PlayParameterRepository
        PlayParameters *playParameters =
            PlayParameterRepository::getInstance()->getPlayParameters(i->first);
        if (playParameters) {
            playParameters->toXml
                (out, indent + "  ",
                 QString("model=\"%1\"")
                 .arg(XmlExportable::getObjectExportId(i->first)));
        }
    }
	    
    for (LayerSet::const_iterator i = m_layers.begin();
	 i != m_layers.end(); ++i) {

	(*i)->toXml(out, indent + "  ");
    }

    out << indent + "</data>\n";
}

QString
Document::toXmlString(QString indent, QString extraAttributes) const
{
    QString s;

    {
        QTextStream out(&s);
        toXml(out, indent, extraAttributes);
    }

    return s;
}

