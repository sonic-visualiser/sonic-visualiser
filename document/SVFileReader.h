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

/**
    SVFileReader loads Sonic Visualiser XML files.  (The SV file
    format is bzipped XML.)

    Some notes about the SV XML format follow.  We're very lazy with
    our XML: there's no schema or DTD, and we depend heavily on
    elements being in a particular order.
 
\verbatim

    <sv>

    <data>

      <!-- The data section contains definitions of both models and
           visual layers.  Layers are considered data in the document;
           the structure of views that displays the layers is not. -->

      <!-- id numbers are unique within the data type (i.e. no two
           models can have the same id, but a model can have the same
           id as a layer, etc).  SV generates its id numbers just for
           the purpose of cross-referencing within the current file;
           they don't necessarily have any meaning once the file has
           been loaded. -->

      <model id="0" name="..." type="..." ... />
      <model id="1" name="..." type="..." ... />

      <!-- Models that have data associated with them store it
           in a neighbouring dataset element.  The dataset must follow
           the model and precede any derivation or layer elements that
           refer to the model. -->

      <model id="2" name="..." type="..." dataset="0" ... />

      <dataset id="0" type="..."> 
        <point frame="..." value="..." ... />
      </dataset>

      <!-- Where one model is derived from another via a transform,
           it has an associated derivation element.  This must follow
           both the source and target model elements.  The source and
           model attributes give the source model id and target model
           id respectively.  A model can have both dataset and
           derivation elements; if it does, dataset must appear first. 
           If the model's data are not stored, but instead the model
           is to be regenerated completely from the transform when 
           the session is reloaded, then the model should have _only_
           a derivation element, and no model element should appear
           for it at all. -->

      <derivation source="0" model="2" transform="..." ...>
        <plugin id="..." ... />
      </derivation>

      <!-- The playparameters element lists playback settings for
           a model. -->

      <playparameters mute="false" pan="0" gain="1" model="1" ... />

      <!-- Layer elements.  The models must have already been defined.
           The same model may appear in more than one layer (of more
           than one type). -->

      <layer id="1" type="..." name="..." model="0" ... />
      <layer id="2" type="..." name="..." model="1" ... />

    </data>


    <display>

      <!-- The display element contains visual structure for the
           layers.  It's simpler than the data section. -->

      <!-- Overall preferred window size for this session. -->

      <window width="..." height="..."/>

      <!-- List of view elements to stack up.  Each one contains
           a list of layers in stacking order, back to front. -->

      <view type="pane" ...>
        <layer id="1"/>
        <layer id="2"/>
      </view>

      <!-- The layer elements just refer to layers defined in the
           data section, so they don't have to have any attributes
           other than the id.  For sort-of-historical reasons SV
           actually does repeat the other attributes here, but
           it doesn't need to. -->

      <view type="pane" ...>
        <layer id="2"/>
      <view>

    </display>


    <!-- List of selected regions by audio frame extents. -->

    <selections>
      <selection start="..." end="..."/>
    </selections>


    </sv>
 
\endverbatim
 */


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
