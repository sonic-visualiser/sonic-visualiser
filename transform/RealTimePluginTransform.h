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

#ifndef _REAL_TIME_PLUGIN_TRANSFORM_H_
#define _REAL_TIME_PLUGIN_TRANSFORM_H_

#include "Transform.h"
#include "plugin/RealTimePluginInstance.h"

class DenseTimeValueModel;

class RealTimePluginTransform : public Transform
{
public:
    RealTimePluginTransform(Model *inputModel,
			    QString plugin,
                            int channel,
			    QString configurationXml = "",
                            QString units = "",
			    int output = 0,
                            size_t blockSize = 0);
    virtual ~RealTimePluginTransform();

protected:
    virtual void run();

    RealTimePluginInstance *m_plugin;
    int m_channel;
    int m_outputNo;
    size_t m_blockSize;

    // just casts
    DenseTimeValueModel *getInput();
};

#endif

