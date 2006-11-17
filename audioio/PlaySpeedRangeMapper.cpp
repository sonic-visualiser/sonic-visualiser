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

#include "PlaySpeedRangeMapper.h"

#include <iostream>
#include <cmath>

PlaySpeedRangeMapper::PlaySpeedRangeMapper(int minpos, int maxpos) :
    m_minpos(minpos),
    m_maxpos(maxpos)
{
}

int
PlaySpeedRangeMapper::getPositionForValue(float value) const
{
    // value is percent
    float factor = getFactorForValue(value);
    int position = getPositionForFactor(factor);
    return position;
}

int
PlaySpeedRangeMapper::getPositionForFactor(float factor) const
{
    bool slow = (factor > 1.0);

    if (!slow) factor = 1.0 / factor;
    
    int half = (m_maxpos + m_minpos) / 2;

    factor = sqrtf((factor - 1.0) * 1000.f);
    int position = lrintf(((factor * (half - m_minpos)) / 100.0) + m_minpos);

    if (slow) {
        position = half - position;
    } else {
        position = position + half;
    }

//    std::cerr << "value = " << value << " slow = " << slow << " factor = " << factor << " position = " << position << std::endl;

    return position;
}

float
PlaySpeedRangeMapper::getValueForPosition(int position) const
{
    float factor = getFactorForPosition(position);
    float pc = getValueForFactor(factor);
    return pc;
}

float
PlaySpeedRangeMapper::getValueForFactor(float factor) const
{
    float pc;
    if (factor < 1.0) pc = ((1.0 / factor) - 1.0) * 100.0;
    else pc = (1.0 - factor) * 100.0;
//    std::cerr << "position = " << position << " percent = " << pc << std::endl;
    return pc;
}

float
PlaySpeedRangeMapper::getFactorForValue(float value) const
{
    // value is percent
    
    float factor;

    if (value <= 0) {
        factor = 1.0 - (value / 100.0);
    } else {
        factor = 1.0 / (1.0 + (value / 100.0));
    }

//    std::cerr << "value = " << value << " factor = " << factor << std::endl;
    return factor;
}

float
PlaySpeedRangeMapper::getFactorForPosition(int position) const
{
    bool slow = false;

    if (position < m_minpos) position = m_minpos;
    if (position > m_maxpos) position = m_maxpos;

    int half = (m_maxpos + m_minpos) / 2;

    if (position < half) {
        slow = true;
        position = half - position;
    } else {
        position = position - half;
    }

    // position is between min and half (inclusive)

    float factor;

    if (position == m_minpos) {
        factor = 1.0;
    } else {
        factor = ((position - m_minpos) * 100.0) / (half - m_minpos);
        factor = 1.0 + (factor * factor) / 1000.f;
    }

    if (!slow) factor = 1.0 / factor;

//    std::cerr << "position = " << position << " slow = " << slow << " factor = " << factor << std::endl;

    return factor;
}

QString
PlaySpeedRangeMapper::getUnit() const
{
    return "%";
}
