/*
    iTunes connection for
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2010 Dan Stowell and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _SVITUNES_H_
#define _SVITUNES_H_

#include <QString>
#include <QStringList>

//LATER: bool iTunesRunning();

// Returns a list containing [posixpath, genre]
QStringList iTunesNowPlaying();

//LATER: QStringList iTunesSelectedPaths();

#endif
