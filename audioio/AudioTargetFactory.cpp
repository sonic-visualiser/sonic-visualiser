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

#include "AudioTargetFactory.h"

#include "AudioJACKTarget.h"
#include "AudioCoreAudioTarget.h"
#include "AudioPortAudioTarget.h"

#include <iostream>

AudioCallbackPlayTarget *
AudioTargetFactory::createCallbackTarget(AudioCallbackPlaySource *source)
{
    AudioCallbackPlayTarget *target = 0;

#ifdef HAVE_JACK
    target = new AudioJACKTarget(source);
    if (target->isOK()) return target;
    else {
	std::cerr << "WARNING: AudioTargetFactory::createCallbackTarget: Failed to open JACK target" << std::endl;
	delete target;
    }
#endif

#ifdef HAVE_COREAUDIO
    target = new AudioCoreAudioTarget(source);
    if (target->isOK()) return target;
    else {
	std::cerr << "WARNING: AudioTargetFactory::createCallbackTarget: Failed to open CoreAudio target" << std::endl;
	delete target;
    }
#endif

#ifdef HAVE_DIRECTSOUND
    target = new AudioDirectSoundTarget(source);
    if (target->isOK()) return target;
    else {
	std::cerr << "WARNING: AudioTargetFactory::createCallbackTarget: Failed to open DirectSound target" << std::endl;
	delete target;
    }
#endif

#ifdef HAVE_PORTAUDIO
    target = new AudioPortAudioTarget(source);
    if (target->isOK()) return target;
    else {
	std::cerr << "WARNING: AudioTargetFactory::createCallbackTarget: Failed to open PortAudio target" << std::endl;
	delete target;
    }
#endif

    std::cerr << "WARNING: AudioTargetFactory::createCallbackTarget: No suitable targets available" << std::endl;
    return 0;
}


