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

//#import <Foundation/Foundation.h>

/**
* Class to handle communication with a running iTunes program on the system.
* Only implemented for Mac at present, since using applescript communication.
*/
class ITunesSVRemote : QObject
{
    Q_OBJECT
    
    public:
        
        ITunesSVRemote() :
            m_playerState(STATE_UNKNOWN),
            m_playerPos(0)
            { }
        virtual ~ITunesSVRemote() { }
    
        // Returns a list containing [posixpath, genre]
        QStringList getNowPlaying();
        
        // When importing a fresh track we don't want the old cached playback position.
        // We can't simply update player position every time, since it only gets reported if playing.
        void resetPlayerState();
        
        // Queries iTunes about player state and stores results locally
        void updatePlayerState();
    
        // Whether the app is running. Only correct if updatePlayerState() recently invoked.
        bool isRunning();
        
        // Whether the app is playing back. Only correct if updatePlayerState() recently invoked.
        bool isPlaying();
        
        // Playback position in seconds. Only correct if updatePlayerState() recently invoked.
        unsigned int playerPos();
    
    protected:
        
        enum {
            STATE_UNKNOWN, // before ever querying
            STATE_CLOSED,  // application not running
            // The rest correspond to states reported by iTunes itself:
            STATE_STOPPED, 
            STATE_PLAYING, 
            STATE_PAUSED,
            STATE_FASTFORWARDING, 
            STATE_REWINDING
            };
        
        // itunes has a set of states: {playing, stopped, ...}.
        // we also use "unknown" before checking, 
        // and "closed" if iTunes isn't running.
        int m_playerState;
        unsigned int m_playerPos; // itunes only tells us seconds
    
};

#endif
