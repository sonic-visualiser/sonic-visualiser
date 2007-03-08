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

#ifndef _TRANSFORM_FACTORY_H_
#define _TRANSFORM_FACTORY_H_

#include "Transform.h"
#include "PluginTransform.h"

#include <map>
#include <set>

namespace Vamp { class PluginBase; }

class AudioCallbackPlaySource;

class TransformFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformFactory();

    static TransformFactory *getInstance();

    // The identifier is intended to be computer-referenceable, and
    // unique within the application.  The name is intended to be
    // human readable.  In principle it doesn't have to be unique, but
    // the factory will add suffixes to ensure that it is, all the
    // same (just to avoid user confusion).  The friendly name is a
    // shorter version of the name.  The type is also intended to be
    // user-readable, for use in menus.

    struct TransformDesc {

        TransformDesc() { }
	TransformDesc(QString _type, QString _category,
                      TransformId _identifier, QString _name,
                      QString _friendlyName, QString _description,
                      QString _maker, QString _units, bool _configurable) :
	    type(_type), category(_category),
            identifier(_identifier), name(_name),
            friendlyName(_friendlyName), description(_description),
            maker(_maker), units(_units), configurable(_configurable) { }

        QString type; // e.g. feature extraction plugin
        QString category; // e.g. time > onsets
	TransformId identifier; // e.g. vamp:vamp-aubio:aubioonset
	QString name; // plugin's name if 1 output, else "name: output"
        QString friendlyName; // short text for layer name
        QString description; // sentence describing transform
        QString maker;
        QString units;
        bool configurable;

        bool operator<(const TransformDesc &od) const {
            return (name < od.name);
        };
    };
    typedef std::vector<TransformDesc> TransformList;

    TransformList getAllTransforms();

    std::vector<QString> getAllTransformTypes();

    std::vector<QString> getTransformCategories(QString transformType);
    std::vector<QString> getTransformMakers(QString transformType);

    /**
     * Get a configuration XML string for the given transform (by
     * asking the user, most likely).  Returns the selected input
     * model if the transform is acceptable, 0 if the operation should
     * be cancelled.  Audio callback play source may be used to
     * audition effects plugins, if provided.
     */
    Model *getConfigurationForTransform(TransformId identifier,
                                        const std::vector<Model *> &candidateInputModels,
                                        PluginTransform::ExecutionContext &context,
                                        QString &configurationXml,
                                        AudioCallbackPlaySource *source = 0);

    /**
     * Get the default execution context for the given transform
     * and input model (if known).
     */
    PluginTransform::ExecutionContext getDefaultContextForTransform(TransformId identifier,
                                                                    Model *inputModel = 0);

    /**
     * Return the output model resulting from applying the named
     * transform to the given input model.  The transform may still be
     * working in the background when the model is returned; check the
     * output model's isReady completion status for more details.
     *
     * If the transform is unknown or the input model is not an
     * appropriate type for the given transform, or if some other
     * problem occurs, return 0.
     * 
     * The returned model is owned by the caller and must be deleted
     * when no longer needed.
     */
    Model *transform(TransformId identifier, Model *inputModel,
                     const PluginTransform::ExecutionContext &context,
                     QString configurationXml = "");

    /**
     * Full name of a transform, suitable for putting on a menu.
     */
    QString getTransformName(TransformId identifier);

    /**
     * Brief but friendly name of a transform, suitable for use
     * as the name of the output layer.
     */
    QString getTransformFriendlyName(TransformId identifier);

    QString getTransformUnits(TransformId identifier);

    /**
     * Return true if the transform has any configurable parameters,
     * i.e. if getConfigurationForTransform can ever return a non-trivial
     * (not equivalent to empty) configuration string.
     */
    bool isTransformConfigurable(TransformId identifier);

    /**
     * If the transform has a prescribed number or range of channel
     * inputs, return true and set minChannels and maxChannels to the
     * minimum and maximum number of channel inputs the transform can
     * accept.  Return false if it doesn't care.
     */
    bool getTransformChannelRange(TransformId identifier,
                                  int &minChannels, int &maxChannels);
	
protected slots:
    void transformFinished();

    void modelAboutToBeDeleted(Model *);

protected:
    Transform *createTransform(TransformId identifier, Model *inputModel,
                               const PluginTransform::ExecutionContext &context,
                               QString configurationXml);

    struct TransformIdent
    {
        TransformId identifier;
        QString configurationXml;
    };

    typedef std::map<TransformId, QString> TransformConfigurationMap;
    TransformConfigurationMap m_lastConfigurations;

    typedef std::map<TransformId, TransformDesc> TransformDescriptionMap;
    TransformDescriptionMap m_transforms;

    typedef std::set<Transform *> TransformSet;
    TransformSet m_runningTransforms;

    void populateTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    bool getChannelRange(TransformId identifier,
                         Vamp::PluginBase *plugin, int &min, int &max);

    static TransformFactory *m_instance;
};


#endif
