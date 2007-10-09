/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 QMUL.
   
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginTransform.h"

#include "vamp-sdk/PluginHostAdapter.h"
#include "vamp-sdk/hostext/PluginWrapper.h"

PluginTransform::PluginTransform(Model *inputModel,
				 const ExecutionContext &context) :
    Transform(inputModel),
    m_context(context)
{
}

PluginTransform::ExecutionContext::ExecutionContext(int _c, size_t _bs) :
    channel(_c),
    domain(Vamp::Plugin::TimeDomain),
    stepSize(_bs ? _bs : 1024),
    blockSize(_bs ? _bs : 1024),
    windowType(HanningWindow),
    startFrame(0),
    duration(0),
    sampleRate(0)
{
}

PluginTransform::ExecutionContext::ExecutionContext(int _c, size_t _ss,
                                                    size_t _bs, WindowType _wt) :
    channel(_c),
    domain(Vamp::Plugin::FrequencyDomain),
    stepSize(_ss ? _ss : (_bs ? _bs / 2 : 512)),
    blockSize(_bs ? _bs : 1024),
    windowType(_wt),
    startFrame(0),
    duration(0),
    sampleRate(0)
{
}

PluginTransform::ExecutionContext::ExecutionContext(int _c,
                                                    const Vamp::PluginBase *_plugin) :
    channel(_c),
    domain(Vamp::Plugin::TimeDomain),
    stepSize(0),
    blockSize(0),
    windowType(HanningWindow),
    startFrame(0),
    duration(0),
    sampleRate(0)
{
    makeConsistentWithPlugin(_plugin);
}

bool
PluginTransform::ExecutionContext::operator==(const ExecutionContext &c)
{
    return (c.channel == channel &&
            c.domain == domain &&
            c.stepSize == stepSize &&
            c.blockSize == blockSize &&
            c.windowType == windowType &&
            c.startFrame == startFrame &&
            c.duration == duration &&
            c.sampleRate == sampleRate);
}

void
PluginTransform::ExecutionContext::makeConsistentWithPlugin(const Vamp::PluginBase *_plugin)
{
    const Vamp::Plugin *vp = dynamic_cast<const Vamp::Plugin *>(_plugin);
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::Plugin" << std::endl;
        vp = dynamic_cast<const Vamp::PluginHostAdapter *>(_plugin); //!!! why?
}
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::PluginHostAdapter" << std::endl;
        vp = dynamic_cast<const Vamp::HostExt::PluginWrapper *>(_plugin); //!!! no, I mean really why?
    }
    if (!vp) {
//        std::cerr << "makeConsistentWithPlugin: not a Vamp::HostExt::PluginWrapper" << std::endl;
    }

    if (!vp) {
        domain = Vamp::Plugin::TimeDomain;
        if (!stepSize) {
            if (!blockSize) blockSize = 1024;
            stepSize = blockSize;
        } else {
            if (!blockSize) blockSize = stepSize;
        }
    } else {
        domain = vp->getInputDomain();
        if (!stepSize) stepSize = vp->getPreferredStepSize();
        if (!blockSize) blockSize = vp->getPreferredBlockSize();
        if (!blockSize) blockSize = 1024;
        if (!stepSize) {
            if (domain == Vamp::Plugin::FrequencyDomain) {
//                std::cerr << "frequency domain, step = " << blockSize/2 << std::endl;
                stepSize = blockSize/2;
            } else {
//                std::cerr << "time domain, step = " << blockSize/2 << std::endl;
                stepSize = blockSize;
            }
        }
    }
}
    

