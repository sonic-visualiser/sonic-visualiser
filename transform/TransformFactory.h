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

#ifndef _TRANSFORM_FACTORY_H_
#define _TRANSFORM_FACTORY_H_

#include "Transform.h"
#include "PluginTransform.h"

#include <map>

namespace Vamp { class PluginBase; }

class AudioCallbackPlaySource;

class TransformFactory : public QObject
{
    Q_OBJECT

public:
    virtual ~TransformFactory();

    static TransformFactory *getInstance();

    // The name is intended to be computer-referenceable, and unique
    // within the application.  The description is intended to be
    // human readable.  In principle it doesn't have to be unique, but
    // the factory will add suffixes to ensure that it is, all the
    // same (just to avoid user confusion).  The friendly name is a
    // shorter version of the description.  The type is also intended
    // to be user-readable, for use in menus.

    struct TransformDesc {

        TransformDesc() { }
	TransformDesc(QString _type, QString _category,
                      TransformName _name, QString _description,
                      QString _friendlyName, QString _maker,
                      QString _units, bool _configurable) :
	    type(_type), category(_category),
            name(_name), description(_description),
            friendlyName(_friendlyName),
            maker(_maker), units(_units), configurable(_configurable) { }

        QString type;
        QString category;
	TransformName name;
	QString description;
        QString friendlyName;
        QString maker;
        QString units;
        bool configurable;

        bool operator<(const TransformDesc &od) const {
            return (description < od.description);
        };
    };
    typedef std::vector<TransformDesc> TransformList;

    TransformList getAllTransforms();

    std::vector<QString> getAllTransformTypes();

    std::vector<QString> getTransformCategories(QString transformType);
    std::vector<QString> getTransformMakers(QString transformType);

    /**
     * Get a configuration XML string for the given transform (by
     * asking the user, most likely).  Returns true if the transform
     * is acceptable, false if the operation should be cancelled.
     * Audio callback play source may be used to audition effects
     * plugins, if provided.
     */
    bool getConfigurationForTransform(TransformName name,
                                      Model *inputModel,
                                      PluginTransform::ExecutionContext &context,
                                      QString &configurationXml,
                                      AudioCallbackPlaySource *source = 0);

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
    Model *transform(TransformName name, Model *inputModel,
                     const PluginTransform::ExecutionContext &context,
                     QString configurationXml = "");

    /**
     * Full description of a transform, suitable for putting on a menu.
     */
    QString getTransformDescription(TransformName name);

    /**
     * Brief but friendly description of a transform, suitable for use
     * as the name of the output layer.
     */
    QString getTransformFriendlyName(TransformName name);

    QString getTransformUnits(TransformName name);

    /**
     * Return true if the transform has any configurable parameters,
     * i.e. if getConfigurationForTransform can ever return a non-trivial
     * (not equivalent to empty) configuration string.
     */
    bool isTransformConfigurable(TransformName name);

    /**
     * If the transform has a prescribed number or range of channel
     * inputs, return true and set minChannels and maxChannels to the
     * minimum and maximum number of channel inputs the transform can
     * accept.  Return false if it doesn't care.
     */
    bool getTransformChannelRange(TransformName name,
                                  int &minChannels, int &maxChannels);

    //!!! Need some way to indicate that the input model has changed /
    //been deleted so as not to blow up backgrounded transform!  -- Or
    //indeed, if the output model has been deleted -- could equally
    //well happen!

    //!!! Need transform category!
	
protected slots:
    void transformFinished();

protected:
    Transform *createTransform(TransformName name, Model *inputModel,
                               const PluginTransform::ExecutionContext &context,
                               QString configurationXml, bool start);

    struct TransformIdent
    {
        TransformName name;
        QString configurationXml;
    };

    typedef std::map<TransformName, QString> TransformConfigurationMap;
    TransformConfigurationMap m_lastConfigurations;

    typedef std::map<TransformName, TransformDesc> TransformDescriptionMap;
    TransformDescriptionMap m_transforms;

    void populateTransforms();
    void populateFeatureExtractionPlugins(TransformDescriptionMap &);
    void populateRealTimePlugins(TransformDescriptionMap &);

    bool getChannelRange(TransformName name,
                         Vamp::PluginBase *plugin, int &min, int &max);

    static TransformFactory *m_instance;
};


#endif
