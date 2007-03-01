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

#ifndef _REAL_TIME_PLUGIN_TRANSFORM_H_
#define _REAL_TIME_PLUGIN_TRANSFORM_H_

#include "PluginTransform.h"
#include "plugin/RealTimePluginInstance.h"

class DenseTimeValueModel;

class RealTimePluginTransform : public PluginTransform
{
public:
    RealTimePluginTransform(Model *inputModel,
			    QString plugin,
                            const ExecutionContext &context,
			    QString configurationXml = "",
                            QString units = "",
			    int output = -1); // -1 -> audio, 0+ -> data
    virtual ~RealTimePluginTransform();

protected:
    virtual void run();

    QString m_pluginId;
    QString m_configurationXml;
    QString m_units;

    RealTimePluginInstance *m_plugin;
    int m_outputNo;

    // just casts
    DenseTimeValueModel *getInput();
};

#endif

