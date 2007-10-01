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

#include "SVFileReader.h"

#include "layer/Layer.h"
#include "view/View.h"
#include "base/PlayParameters.h"
#include "base/PlayParameterRepository.h"

#include "data/fileio/AudioFileReaderFactory.h"
#include "data/fileio/FileFinder.h"
#include "data/fileio/RemoteFile.h"

#include "data/model/WaveFileModel.h"
#include "data/model/EditableDenseThreeDimensionalModel.h"
#include "data/model/SparseOneDimensionalModel.h"
#include "data/model/SparseTimeValueModel.h"
#include "data/model/NoteModel.h"
#include "data/model/TextModel.h"

#include "view/Pane.h"

#include "Document.h"

#include <QString>
#include <QMessageBox>
#include <QFileDialog>

#include <iostream>

SVFileReader::SVFileReader(Document *document,
			   SVFileReaderPaneCallback &callback,
                           QString location) :
    m_document(document),
    m_paneCallback(callback),
    m_location(location),
    m_currentPane(0),
    m_currentDataset(0),
    m_currentDerivedModel(0),
    m_currentDerivedModelId(-1),
    m_currentPlayParameters(0),
    m_datasetSeparator(" "),
    m_inRow(false),
    m_inLayer(false),
    m_inView(false),
    m_rowNumber(0),
    m_ok(false)
{
}

void
SVFileReader::parse(const QString &xmlData)
{
    QXmlInputSource inputSource;
    inputSource.setData(xmlData);
    parse(inputSource);
}

void
SVFileReader::parse(QXmlInputSource &inputSource)
{
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    m_ok = reader.parse(inputSource);
}    

bool
SVFileReader::isOK()
{
    return m_ok;
}
	
SVFileReader::~SVFileReader()
{
    if (!m_awaitingDatasets.empty()) {
	std::cerr << "WARNING: SV-XML: File ended with "
		  << m_awaitingDatasets.size() << " unfilled model dataset(s)"
		  << std::endl;
    }

    std::set<Model *> unaddedModels;

    for (std::map<int, Model *>::iterator i = m_models.begin();
	 i != m_models.end(); ++i) {
	if (m_addedModels.find(i->second) == m_addedModels.end()) {
	    unaddedModels.insert(i->second);
	}
    }

    if (!unaddedModels.empty()) {
	std::cerr << "WARNING: SV-XML: File contained "
		  << unaddedModels.size() << " unused models"
		  << std::endl;
	while (!unaddedModels.empty()) {
	    delete *unaddedModels.begin();
	    unaddedModels.erase(unaddedModels.begin());
	}
    }	
}

bool
SVFileReader::startElement(const QString &, const QString &,
			   const QString &qName,
			   const QXmlAttributes &attributes)
{
    QString name = qName.toLower();

    bool ok = false;

    // Valid element names:
    //
    // sv
    // data
    // dataset
    // display
    // derivation
    // playparameters
    // layer
    // model
    // point
    // row
    // view
    // window

    if (name == "sv") {

	// nothing needed
	ok = true;

    } else if (name == "data") {

	// nothing needed
	m_inData = true;
	ok = true;

    } else if (name == "display") {

	// nothing needed
	ok = true;

    } else if (name == "window") {

	ok = readWindow(attributes);

    } else if (name == "model") {

	ok = readModel(attributes);
    
    } else if (name == "dataset") {
	
	ok = readDatasetStart(attributes);

    } else if (name == "bin") {
	
	ok = addBinToDataset(attributes);
    
    } else if (name == "point") {
	
	ok = addPointToDataset(attributes);

    } else if (name == "row") {

	ok = addRowToDataset(attributes);

    } else if (name == "layer") {

        addUnaddedModels(); // all models must be specified before first layer
	ok = readLayer(attributes);

    } else if (name == "view") {

	m_inView = true;
	ok = readView(attributes);

    } else if (name == "derivation") {

	ok = readDerivation(attributes);

    } else if (name == "playparameters") {
        
        ok = readPlayParameters(attributes);

    } else if (name == "plugin") {

	ok = readPlugin(attributes);

    } else if (name == "selections") {

	m_inSelections = true;
	ok = true;

    } else if (name == "selection") {

	ok = readSelection(attributes);

    } else if (name == "measurement") {

        ok = readMeasurement(attributes);

    } else {
        std::cerr << "WARNING: SV-XML: Unexpected element \""
                  << name.toLocal8Bit().data() << "\"" << std::endl;
    }

    if (!ok) {
	std::cerr << "WARNING: SV-XML: Failed to completely process element \""
		  << name.toLocal8Bit().data() << "\"" << std::endl;
    }

    return true;
}

bool
SVFileReader::characters(const QString &text)
{
    bool ok = false;

    if (m_inRow) {
	ok = readRowData(text);
	if (!ok) {
	    std::cerr << "WARNING: SV-XML: Failed to read row data content for row " << m_rowNumber << std::endl;
	}
    }

    return true;
}

bool
SVFileReader::endElement(const QString &, const QString &,
			 const QString &qName)
{
    QString name = qName.toLower();

    if (name == "dataset") {

	if (m_currentDataset) {
	    
	    bool foundInAwaiting = false;

	    for (std::map<int, int>::iterator i = m_awaitingDatasets.begin();
		 i != m_awaitingDatasets.end(); ++i) {
		if (haveModel(i->second) &&
                    m_models[i->second] == m_currentDataset) {
		    m_awaitingDatasets.erase(i);
		    foundInAwaiting = true;
		    break;
		}
	    }

	    if (!foundInAwaiting) {
		std::cerr << "WARNING: SV-XML: Dataset precedes model, or no model uses dataset" << std::endl;
	    }
	}

	m_currentDataset = 0;

    } else if (name == "data") {

        addUnaddedModels();
	m_inData = false;

    } else if (name == "derivation") {

        if (!m_currentDerivedModel) {
            if (m_currentDerivedModel < 0) {
                std::cerr << "WARNING: SV-XML: Bad derivation output model id "
                          << m_currentDerivedModelId << std::endl;
            } else if (haveModel(m_currentDerivedModelId)) {
                std::cerr << "WARNING: SV-XML: Derivation has existing model "
                          << m_currentDerivedModelId
                          << " as target, not regenerating" << std::endl;
            } else {
                m_currentDerivedModel = m_models[m_currentDerivedModelId] =
                    m_document->addDerivedModel(m_currentTransform,
                                                m_currentTransformSource,
                                                m_currentTransformContext,
                                                m_currentTransformConfiguration);
            }
        } else {
            m_document->addDerivedModel(m_currentTransform,
                                        m_currentTransformSource,
                                        m_currentTransformContext,
                                        m_currentDerivedModel,
                                        m_currentTransformConfiguration);
        }

        m_addedModels.insert(m_currentDerivedModel);
        m_currentDerivedModel = 0;
        m_currentDerivedModelId = -1;
        m_currentTransform = "";
        m_currentTransformConfiguration = "";

    } else if (name == "row") {
	m_inRow = false;
    } else if (name == "layer") {
        m_inLayer = false;
    } else if (name == "view") {
	m_inView = false;
    } else if (name == "selections") {
	m_inSelections = false;
    } else if (name == "playparameters") {
        m_currentPlayParameters = 0;
    }

    return true;
}

bool
SVFileReader::error(const QXmlParseException &exception)
{
    m_errorString =
	QString("ERROR: SV-XML: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << m_errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::error(exception);
}

bool
SVFileReader::fatalError(const QXmlParseException &exception)
{
    m_errorString =
	QString("FATAL ERROR: SV-XML: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << m_errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::fatalError(exception);
}


#define READ_MANDATORY(TYPE, NAME, CONVERSION)		      \
    TYPE NAME = attributes.value(#NAME).trimmed().CONVERSION(&ok); \
    if (!ok) { \
	std::cerr << "WARNING: SV-XML: Missing or invalid mandatory " #TYPE " attribute \"" #NAME "\"" << std::endl; \
	return false; \
    }

bool
SVFileReader::readWindow(const QXmlAttributes &attributes)
{
    bool ok = false;

    READ_MANDATORY(int, width, toInt);
    READ_MANDATORY(int, height, toInt);

    m_paneCallback.setWindowSize(width, height);
    return true;
}

void
SVFileReader::addUnaddedModels()
{
    std::set<Model *> unaddedModels;
    
    for (std::map<int, Model *>::iterator i = m_models.begin();
         i != m_models.end(); ++i) {
        if (m_addedModels.find(i->second) == m_addedModels.end()) {
            unaddedModels.insert(i->second);
        }
    }
    
    for (std::set<Model *>::iterator i = unaddedModels.begin();
         i != unaddedModels.end(); ++i) {
        m_document->addImportedModel(*i);
        m_addedModels.insert(*i);
    }
}

bool
SVFileReader::readModel(const QXmlAttributes &attributes)
{
    bool ok = false;

    READ_MANDATORY(int, id, toInt);

    if (haveModel(id)) {
	std::cerr << "WARNING: SV-XML: Ignoring duplicate model id " << id
		  << std::endl;
	return false;
    }

    QString name = attributes.value("name");

    std::cerr << "SVFileReader::readModel: model name \"" << name.toStdString() << "\"" << std::endl;

    READ_MANDATORY(int, sampleRate, toInt);

    QString type = attributes.value("type").trimmed();
    bool mainModel = (attributes.value("mainModel").trimmed() == "true");

    if (type == "wavefile") {
	
        WaveFileModel *model = 0;
        FileFinder *ff = FileFinder::getInstance();
        QString originalPath = attributes.value("file");
        QString path = ff->find(FileFinder::AudioFile,
                                originalPath, m_location);
        QUrl url(path);

        if (RemoteFile::canHandleScheme(url)) {

            RemoteFile rf(url);
            rf.wait();

            if (rf.isOK()) {

                model = new WaveFileModel(rf.getLocalFilename(), path);
                if (!model->isOK()) {
                    delete model;
                    model = 0;
                    //!!! and delete local file?
                }
            }
        } else {

            model = new WaveFileModel(path);
            if (!model->isOK()) {
                delete model;
                model = 0;
            }
        }

        if (!model) return false;

        model->setObjectName(name);
	m_models[id] = model;
	if (mainModel) {
	    m_document->setMainModel(model);
	    m_addedModels.insert(model);
	}
	// Derived models will be added when their derivation
	// is found.

	return true;

    } else if (type == "dense") {
	
	READ_MANDATORY(int, dimensions, toInt);
		    
	// Currently the only dense model we support here is the dense
	// 3d model.  Dense time-value models are always file-backed
	// waveform data, at this point, and they come in as wavefile
	// models.
	
	if (dimensions == 3) {
	    
	    READ_MANDATORY(int, windowSize, toInt);
	    READ_MANDATORY(int, yBinCount, toInt);
	    
            EditableDenseThreeDimensionalModel *model =
		new EditableDenseThreeDimensionalModel
                (sampleRate, windowSize, yBinCount);
	    
	    float minimum = attributes.value("minimum").trimmed().toFloat(&ok);
	    if (ok) model->setMinimumLevel(minimum);
	    
	    float maximum = attributes.value("maximum").trimmed().toFloat(&ok);
	    if (ok) model->setMaximumLevel(maximum);

	    int dataset = attributes.value("dataset").trimmed().toInt(&ok);
	    if (ok) m_awaitingDatasets[dataset] = id;

            model->setObjectName(name);
	    m_models[id] = model;
	    return true;

	} else {

	    std::cerr << "WARNING: SV-XML: Unexpected dense model dimension ("
		      << dimensions << ")" << std::endl;
	}
    } else if (type == "sparse") {

	READ_MANDATORY(int, dimensions, toInt);
		  
	if (dimensions == 1) {
	    
	    READ_MANDATORY(int, resolution, toInt);

	    SparseOneDimensionalModel *model = new SparseOneDimensionalModel
		(sampleRate, resolution);
            model->setObjectName(name);
	    m_models[id] = model;

	    int dataset = attributes.value("dataset").trimmed().toInt(&ok);
	    if (ok) m_awaitingDatasets[dataset] = id;

	    return true;

	} else if (dimensions == 2 || dimensions == 3) {
	    
	    READ_MANDATORY(int, resolution, toInt);

            bool haveMinMax = true;
	    float minimum = attributes.value("minimum").trimmed().toFloat(&ok);
            if (!ok) haveMinMax = false;
	    float maximum = attributes.value("maximum").trimmed().toFloat(&ok);
            if (!ok) haveMinMax = false;

	    float valueQuantization =
		attributes.value("valueQuantization").trimmed().toFloat(&ok);

	    bool notifyOnAdd = (attributes.value("notifyOnAdd") == "true");

            QString units = attributes.value("units");

	    if (dimensions == 2) {
		if (attributes.value("subtype") == "text") {
		    TextModel *model = new TextModel
			(sampleRate, resolution, notifyOnAdd);
                    model->setObjectName(name);
		    m_models[id] = model;
		} else {
		    SparseTimeValueModel *model;
                    if (haveMinMax) {
                        model = new SparseTimeValueModel
                            (sampleRate, resolution, minimum, maximum, notifyOnAdd);
                    } else {
                        model = new SparseTimeValueModel
                            (sampleRate, resolution, notifyOnAdd);
                    }
                    model->setScaleUnits(units);
                    model->setObjectName(name);
		    m_models[id] = model;
		}
	    } else {
		NoteModel *model;
                if (haveMinMax) {
                    model = new NoteModel
                        (sampleRate, resolution, minimum, maximum, notifyOnAdd);
                } else {
                    model = new NoteModel
                        (sampleRate, resolution, notifyOnAdd);
                }
		model->setValueQuantization(valueQuantization);
                model->setScaleUnits(units);
                model->setObjectName(name);
		m_models[id] = model;
	    }

	    int dataset = attributes.value("dataset").trimmed().toInt(&ok);
	    if (ok) m_awaitingDatasets[dataset] = id;

	    return true;

	} else {

	    std::cerr << "WARNING: SV-XML: Unexpected sparse model dimension ("
		      << dimensions << ")" << std::endl;
	}
    } else {

	std::cerr << "WARNING: SV-XML: Unexpected model type \""
		  << type.toLocal8Bit().data() << "\" for model id " << id << std::endl;
    }

    return false;
}

bool
SVFileReader::readView(const QXmlAttributes &attributes)
{
    QString type = attributes.value("type");
    m_currentPane = 0;
    
    if (type != "pane") {
	std::cerr << "WARNING: SV-XML: Unexpected view type \""
		  << type.toLocal8Bit().data() << "\"" << std::endl;
	return false;
    }

    m_currentPane = m_paneCallback.addPane();

    if (!m_currentPane) {
	std::cerr << "WARNING: SV-XML: Internal error: Failed to add pane!"
		  << std::endl;
	return false;
    }

    bool ok = false;

    View *view = m_currentPane;

    // The view properties first

    READ_MANDATORY(size_t, centre, toUInt);
    READ_MANDATORY(size_t, zoom, toUInt);
    READ_MANDATORY(int, followPan, toInt);
    READ_MANDATORY(int, followZoom, toInt);
    QString tracking = attributes.value("tracking");

    // Specify the follow modes before we set the actual values
    view->setFollowGlobalPan(followPan);
    view->setFollowGlobalZoom(followZoom);
    view->setPlaybackFollow(tracking == "scroll" ? PlaybackScrollContinuous :
			    tracking == "page" ? PlaybackScrollPage
			    : PlaybackIgnore);

    // Then set these values
    view->setCentreFrame(centre);
    view->setZoomLevel(zoom);

    // And pane properties
    READ_MANDATORY(int, centreLineVisible, toInt);
    m_currentPane->setCentreLineVisible(centreLineVisible);

    int height = attributes.value("height").toInt(&ok);
    if (ok) {
	m_currentPane->resize(m_currentPane->width(), height);
    }

    return true;
}

bool
SVFileReader::readLayer(const QXmlAttributes &attributes)
{
    QString type = attributes.value("type");

    int id;
    bool ok = false;
    id = attributes.value("id").trimmed().toInt(&ok);

    if (!ok) {
	std::cerr << "WARNING: SV-XML: No layer id for layer of type \""
		  << type.toLocal8Bit().data()
		  << "\"" << std::endl;
	return false;
    }

    Layer *layer = 0;
    bool isNewLayer = false;

    // Layers are expected to be defined in layer elements in the data
    // section, and referred to in layer elements in the view
    // sections.  So if we're in the data section, we expect this
    // layer not to exist already; if we're in the view section, we
    // expect it to exist.

    if (m_inData) {

	if (m_layers.find(id) != m_layers.end()) {
	    std::cerr << "WARNING: SV-XML: Ignoring duplicate layer id " << id
		      << " in data section" << std::endl;
	    return false;
	}

	layer = m_layers[id] = m_document->createLayer
	    (LayerFactory::getInstance()->getLayerTypeForName(type));

	if (layer) {
	    m_layers[id] = layer;
	    isNewLayer = true;
	}

    } else {

	if (!m_currentPane) {
	    std::cerr << "WARNING: SV-XML: No current pane for layer " << id
		      << " in view section" << std::endl;
	    return false;
	}

	if (m_layers.find(id) != m_layers.end()) {
	    
	    layer = m_layers[id];
	
	} else {
	    std::cerr << "WARNING: SV-XML: Layer id " << id 
		      << " in view section has not been defined -- defining it here"
		      << std::endl;

	    layer = m_document->createLayer
		(LayerFactory::getInstance()->getLayerTypeForName(type));

	    if (layer) {
		m_layers[id] = layer;
		isNewLayer = true;
	    }
	}
    }
	    
    if (!layer) {
	std::cerr << "WARNING: SV-XML: Failed to add layer of type \""
		  << type.toLocal8Bit().data()
		  << "\"" << std::endl;
	return false;
    }

    if (isNewLayer) {

	QString name = attributes.value("name");
	layer->setObjectName(name);

	int modelId;
	bool modelOk = false;
	modelId = attributes.value("model").trimmed().toInt(&modelOk);

	if (modelOk) {
	    if (haveModel(modelId)) {
		Model *model = m_models[modelId];
		m_document->setModel(layer, model);
	    } else {
		std::cerr << "WARNING: SV-XML: Unknown model id " << modelId
			  << " in layer definition" << std::endl;
	    }
	}

	layer->setProperties(attributes);
    }

    if (!m_inData && m_currentPane) {

        QString visible = attributes.value("visible");
        bool dormant = (visible == "false");

        // We need to do this both before and after adding the layer
        // to the view -- we need it to be dormant if appropriate
        // before it's actually added to the view so that any property
        // box gets the right state when it's added, but the add layer
        // command sets dormant to false because it assumes it may be
        // restoring a previously dormant layer, so we need to set it
        // again afterwards too.  Hm
        layer->setLayerDormant(m_currentPane, dormant);

	m_document->addLayerToView(m_currentPane, layer);

        layer->setLayerDormant(m_currentPane, dormant);
    }

    m_currentLayer = layer;
    m_inLayer = true;

    return true;
}

bool
SVFileReader::readDatasetStart(const QXmlAttributes &attributes)
{
    bool ok = false;

    READ_MANDATORY(int, id, toInt);
    READ_MANDATORY(int, dimensions, toInt);
    
    if (m_awaitingDatasets.find(id) == m_awaitingDatasets.end()) {
	std::cerr << "WARNING: SV-XML: Unwanted dataset " << id << std::endl;
	return false;
    }
    
    int modelId = m_awaitingDatasets[id];
    
    Model *model = 0;
    if (haveModel(modelId)) {
	model = m_models[modelId];
    } else {
	std::cerr << "WARNING: SV-XML: Internal error: Unknown model " << modelId
		  << " expecting dataset " << id << std::endl;
	return false;
    }

    bool good = false;

    switch (dimensions) {
    case 1:
	if (dynamic_cast<SparseOneDimensionalModel *>(model)) good = true;
	break;

    case 2:
	if (dynamic_cast<SparseTimeValueModel *>(model)) good = true;
	else if (dynamic_cast<TextModel *>(model)) good = true;
	break;

    case 3:
	if (dynamic_cast<NoteModel *>(model)) good = true;
	else if (dynamic_cast<EditableDenseThreeDimensionalModel *>(model)) {
	    m_datasetSeparator = attributes.value("separator");
	    good = true;
	}
	break;
    }

    if (!good) {
	std::cerr << "WARNING: SV-XML: Model id " << modelId << " has wrong number of dimensions for " << dimensions << "-D dataset " << id << std::endl;
	m_currentDataset = 0;
	return false;
    }

    m_currentDataset = model;
    return true;
}

bool
SVFileReader::addPointToDataset(const QXmlAttributes &attributes)
{
    bool ok = false;

    READ_MANDATORY(int, frame, toInt);

//    std::cerr << "SVFileReader::addPointToDataset: frame = " << frame << std::endl;

    SparseOneDimensionalModel *sodm = dynamic_cast<SparseOneDimensionalModel *>
	(m_currentDataset);

    if (sodm) {
//        std::cerr << "Current dataset is a sparse one dimensional model" << std::endl;
	QString label = attributes.value("label");
	sodm->addPoint(SparseOneDimensionalModel::Point(frame, label));
	return true;
    }

    SparseTimeValueModel *stvm = dynamic_cast<SparseTimeValueModel *>
	(m_currentDataset);

    if (stvm) {
//        std::cerr << "Current dataset is a sparse time-value model" << std::endl;
	float value = 0.0;
	value = attributes.value("value").trimmed().toFloat(&ok);
	QString label = attributes.value("label");
	stvm->addPoint(SparseTimeValueModel::Point(frame, value, label));
	return ok;
    }
	
    NoteModel *nm = dynamic_cast<NoteModel *>(m_currentDataset);

    if (nm) {
//        std::cerr << "Current dataset is a note model" << std::endl;
	float value = 0.0;
	value = attributes.value("value").trimmed().toFloat(&ok);
	size_t duration = 0;
	duration = attributes.value("duration").trimmed().toUInt(&ok);
	QString label = attributes.value("label");
	nm->addPoint(NoteModel::Point(frame, value, duration, label));
	return ok;
    }

    TextModel *tm = dynamic_cast<TextModel *>(m_currentDataset);

    if (tm) {
//        std::cerr << "Current dataset is a text model" << std::endl;
	float height = 0.0;
	height = attributes.value("height").trimmed().toFloat(&ok);
	QString label = attributes.value("label");
//        std::cerr << "SVFileReader::addPointToDataset: TextModel: frame = " << frame << ", height = " << height << ", label = " << label.toStdString() << ", ok = " << ok << std::endl;
	tm->addPoint(TextModel::Point(frame, height, label));
	return ok;
    }

    std::cerr << "WARNING: SV-XML: Point element found in non-point dataset" << std::endl;

    return false;
}

bool
SVFileReader::addBinToDataset(const QXmlAttributes &attributes)
{
    EditableDenseThreeDimensionalModel *dtdm = 
        dynamic_cast<EditableDenseThreeDimensionalModel *>
	(m_currentDataset);

    if (dtdm) {

	bool ok = false;
	int n = attributes.value("number").trimmed().toInt(&ok);
	if (!ok) {
	    std::cerr << "WARNING: SV-XML: Missing or invalid bin number"
		      << std::endl;
	    return false;
	}

	QString name = attributes.value("name");

	dtdm->setBinName(n, name);
	return true;
    }

    std::cerr << "WARNING: SV-XML: Bin definition found in incompatible dataset" << std::endl;

    return false;
}


bool
SVFileReader::addRowToDataset(const QXmlAttributes &attributes)
{
    m_inRow = false;

    bool ok = false;
    m_rowNumber = attributes.value("n").trimmed().toInt(&ok);
    if (!ok) {
	std::cerr << "WARNING: SV-XML: Missing or invalid row number"
		  << std::endl;
	return false;
    }
    
    m_inRow = true;

//    std::cerr << "SV-XML: In row " << m_rowNumber << std::endl;
    
    return true;
}

bool
SVFileReader::readRowData(const QString &text)
{
    EditableDenseThreeDimensionalModel *dtdm =
        dynamic_cast<EditableDenseThreeDimensionalModel *>
	(m_currentDataset);

    bool warned = false;

    if (dtdm) {
	QStringList data = text.split(m_datasetSeparator);

	DenseThreeDimensionalModel::Column values;

	for (QStringList::iterator i = data.begin(); i != data.end(); ++i) {

	    if (values.size() == dtdm->getHeight()) {
		if (!warned) {
		    std::cerr << "WARNING: SV-XML: Too many y-bins in 3-D dataset row "
			      << m_rowNumber << std::endl;
		    warned = true;
		}
	    }

	    bool ok;
	    float value = i->toFloat(&ok);
	    if (!ok) {
		std::cerr << "WARNING: SV-XML: Bad floating-point value "
			  << i->toLocal8Bit().data()
			  << " in row data" << std::endl;
	    } else {
		values.push_back(value);
	    }
	}

	dtdm->setColumn(m_rowNumber, values);
	return true;
    }

    std::cerr << "WARNING: SV-XML: Row data found in non-row dataset" << std::endl;

    return false;
}

bool
SVFileReader::readDerivation(const QXmlAttributes &attributes)
{
    int modelId = 0;
    bool modelOk = false;
    modelId = attributes.value("model").trimmed().toInt(&modelOk);

    if (!modelOk) {
	std::cerr << "WARNING: SV-XML: No model id specified for derivation" << std::endl;
	return false;
    }

    QString transform = attributes.value("transform");

    if (haveModel(modelId)) {
        m_currentDerivedModel = m_models[modelId];
    } else {
        // we'll regenerate the model when the derivation element ends
        m_currentDerivedModel = 0;
    }
    
    m_currentDerivedModelId = modelId;
    
    int sourceId = 0;
    bool sourceOk = false;
    sourceId = attributes.value("source").trimmed().toInt(&sourceOk);

    if (sourceOk && haveModel(sourceId)) {
        m_currentTransformSource = m_models[sourceId];
    } else {
        m_currentTransformSource = m_document->getMainModel();
    }

    m_currentTransform = transform;
    m_currentTransformConfiguration = "";

    m_currentTransformContext = PluginTransform::ExecutionContext();

    bool ok = false;
    int channel = attributes.value("channel").trimmed().toInt(&ok);
    if (ok) m_currentTransformContext.channel = channel;

    int domain = attributes.value("domain").trimmed().toInt(&ok);
    if (ok) m_currentTransformContext.domain = Vamp::Plugin::InputDomain(domain);

    int stepSize = attributes.value("stepSize").trimmed().toInt(&ok);
    if (ok) m_currentTransformContext.stepSize = stepSize;

    int blockSize = attributes.value("blockSize").trimmed().toInt(&ok);
    if (ok) m_currentTransformContext.blockSize = blockSize;

    int windowType = attributes.value("windowType").trimmed().toInt(&ok);
    if (ok) m_currentTransformContext.windowType = WindowType(windowType);

    QString startFrameStr = attributes.value("startFrame");
    QString durationStr = attributes.value("duration");

    size_t startFrame = 0;
    size_t duration = 0;

    if (startFrameStr != "") {
        startFrame = startFrameStr.trimmed().toInt(&ok);
        if (!ok) startFrame = 0;
    }
    if (durationStr != "") {
        duration = durationStr.trimmed().toInt(&ok);
        if (!ok) duration = 0;
    }

    m_currentTransformContext.startFrame = startFrame;
    m_currentTransformContext.duration = duration;

    return true;
}

bool
SVFileReader::readPlayParameters(const QXmlAttributes &attributes)
{
    m_currentPlayParameters = 0;

    int modelId = 0;
    bool modelOk = false;
    modelId = attributes.value("model").trimmed().toInt(&modelOk);

    if (!modelOk) {
	std::cerr << "WARNING: SV-XML: No model id specified for play parameters" << std::endl;
	return false;
    }

    if (haveModel(modelId)) {

        bool ok = false;

        PlayParameters *parameters = PlayParameterRepository::getInstance()->
            getPlayParameters(m_models[modelId]);

        if (!parameters) {
            std::cerr << "WARNING: SV-XML: Play parameters for model "
                      << modelId
                      << " not found - has model been added to document?"
                      << std::endl;
            return false;
        }
        
        bool muted = (attributes.value("mute").trimmed() == "true");
        parameters->setPlayMuted(muted);
        
        float pan = attributes.value("pan").toFloat(&ok);
        if (ok) parameters->setPlayPan(pan);
        
        float gain = attributes.value("gain").toFloat(&ok);
        if (ok) parameters->setPlayGain(gain);
        
        QString pluginId = attributes.value("pluginId");
        if (pluginId != "") parameters->setPlayPluginId(pluginId);
        
        m_currentPlayParameters = parameters;

//        std::cerr << "Current play parameters for model: " << m_models[modelId] << ": " << m_currentPlayParameters << std::endl;

    } else {

	std::cerr << "WARNING: SV-XML: Unknown model " << modelId
		  << " for play parameters" << std::endl;
        return false;
    }

    return true;
}

bool
SVFileReader::readPlugin(const QXmlAttributes &attributes)
{
    if (m_currentDerivedModelId < 0 && !m_currentPlayParameters) {
        std::cerr << "WARNING: SV-XML: Plugin found outside derivation or play parameters" << std::endl;
        return false;
    }

    QString configurationXml = "<plugin";
    
    for (int i = 0; i < attributes.length(); ++i) {
        configurationXml += QString(" %1=\"%2\"")
            .arg(attributes.qName(i))
            .arg(XmlExportable::encodeEntities(attributes.value(i)));
    }

    configurationXml += "/>";

    if (m_currentPlayParameters) {
        m_currentPlayParameters->setPlayPluginConfiguration(configurationXml);
    } else {
        m_currentTransformConfiguration += configurationXml;
    }

    return true;
}

bool
SVFileReader::readSelection(const QXmlAttributes &attributes)
{
    bool ok;

    READ_MANDATORY(int, start, toInt);
    READ_MANDATORY(int, end, toInt);

    m_paneCallback.addSelection(start, end);

    return true;
}

bool
SVFileReader::readMeasurement(const QXmlAttributes &attributes)
{
    std::cerr << "SVFileReader::readMeasurement: inLayer "
              << m_inLayer << ", layer " << m_currentLayer << std::endl;

    if (!m_inLayer) {
        std::cerr << "WARNING: SV-XML: Measurement found outside layer" << std::endl;
        return false;
    }

    m_currentLayer->addMeasurementRect(attributes);
    return true;
}

SVFileReaderPaneCallback::~SVFileReaderPaneCallback()
{
}

