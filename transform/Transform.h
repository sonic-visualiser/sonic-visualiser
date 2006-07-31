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

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "base/Thread.h"

#include "data/model/Model.h"

typedef QString TransformName;

/**
 * A Transform turns one data model into another.
 *
 * Typically in this application, a Transform might have a
 * DenseTimeValueModel as its input (e.g. an audio waveform) and a
 * SparseOneDimensionalModel (e.g. detected beats) as its output.
 *
 * The Transform typically runs in the background, as a separate
 * thread populating the output model.  The model is available to the
 * user of the Transform immediately, but may be initially empty until
 * the background thread has populated it.
 */

class Transform : public Thread
{
public:
    virtual ~Transform();

    Model *getInputModel()  { return m_input; }
    Model *getOutputModel() { return m_output; }
    Model *detachOutputModel() { m_detached = true; return m_output; }

protected:
    Transform(Model *m);

    Model *m_input; // I don't own this
    Model *m_output; // I own this, unless...
    bool m_detached; // ... this is true.
    bool m_deleting;
};

#endif
