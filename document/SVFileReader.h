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

#ifndef _SV_FILE_READER_H_
#define _SV_FILE_READER_H_

#include "layer/LayerFactory.h"
#include "transform/Transform.h"
#include "transform/PluginTransform.h"

#include <QXmlDefaultHandler>

#include <map>

class Pane;
class Model;
class Document;
class PlayParameters;

class SVFileReaderPaneCallback
{
public:
    virtual Pane *addPane() = 0;
    virtual void setWindowSize(int width, int height) = 0;
    virtual void addSelection(int start, int end) = 0;
};

class SVFileReader : public QXmlDefaultHandler
{
public:
    SVFileReader(Document *document,
		 SVFileReaderPaneCallback &callback,
                 QString location = ""); // for audio file locate mechanism
    virtual ~SVFileReader();

    void parse(const QString &xmlData);
    void parse(QXmlInputSource &source);

    bool isOK();
    QString getErrorString() const { return m_errorString; }

    // For loading a single layer onto an existing pane
    void setCurrentPane(Pane *pane) { m_currentPane = pane; }
    
    virtual bool startElement(const QString &namespaceURI,
			      const QString &localName,
			      const QString &qName,
			      const QXmlAttributes& atts);

    virtual bool characters(const QString &);

    virtual bool endElement(const QString &namespaceURI,
			    const QString &localName,
			    const QString &qName);

    bool error(const QXmlParseException &exception);
    bool fatalError(const QXmlParseException &exception);

protected:
    bool readWindow(const QXmlAttributes &);
    bool readModel(const QXmlAttributes &);
    bool readView(const QXmlAttributes &);
    bool readLayer(const QXmlAttributes &);
    bool readDatasetStart(const QXmlAttributes &);
    bool addBinToDataset(const QXmlAttributes &);
    bool addPointToDataset(const QXmlAttributes &);
    bool addRowToDataset(const QXmlAttributes &);
    bool readRowData(const QString &);
    bool readDerivation(const QXmlAttributes &);
    bool readPlayParameters(const QXmlAttributes &);
    bool readPlugin(const QXmlAttributes &);
    bool readSelection(const QXmlAttributes &);
    void addUnaddedModels();

    Document *m_document;
    SVFileReaderPaneCallback &m_paneCallback;
    QString m_location;
    Pane *m_currentPane;
    std::map<int, Layer *> m_layers;
    std::map<int, Model *> m_models;
    std::set<Model *> m_addedModels;
    std::map<int, int> m_awaitingDatasets; // map dataset id -> model id
    Model *m_currentDataset;
    Model *m_currentDerivedModel;
    int m_currentDerivedModelId;
    PlayParameters *m_currentPlayParameters;
    QString m_currentTransform;
    Model *m_currentTransformSource;
    PluginTransform::ExecutionContext m_currentTransformContext;
    QString m_currentTransformConfiguration;
    QString m_datasetSeparator;
    bool m_inRow;
    bool m_inView;
    bool m_inData;
    bool m_inSelections;
    int m_rowNumber;
    QString m_errorString;
    bool m_ok;
};

#endif
