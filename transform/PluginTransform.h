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

#ifndef _PLUGIN_TRANSFORM_H_
#define _PLUGIN_TRANSFORM_H_

#include "Transform.h"

#include "base/Window.h"

#include "vamp-sdk/Plugin.h"

//!!! should this just move back up to Transform? It is after all used
//directly in all sorts of generic places, like Document

class PluginTransform : public Transform
{
public:
    class ExecutionContext {
    public:
        // Time domain:
        ExecutionContext(int _c = -1, size_t _bs = 0);
        
        // Frequency domain:
        ExecutionContext(int _c, size_t _ss, size_t _bs, WindowType _wt);

        // From plugin defaults:
        ExecutionContext(int _c, const Vamp::PluginBase *_plugin);

        bool operator==(const ExecutionContext &);

        void makeConsistentWithPlugin(const Vamp::PluginBase *_plugin);

        int channel;
        Vamp::Plugin::InputDomain domain;
        size_t stepSize;
        size_t blockSize;
        WindowType windowType;
    };

protected:
    PluginTransform(Model *inputModel,
                    const ExecutionContext &context);

    ExecutionContext m_context;
};

#endif
