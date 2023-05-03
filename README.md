
Sonic Visualiser
================

### A program for viewing and analysing the contents of music audio files

![Sonic Visualiser small screenshot](https://sonicvisualiser.org/images/sv-3.0-win-thumb.png)

* Home page and downloads: https://sonicvisualiser.org/
* Code project: https://github.com/sonic-visualiser/sonic-visualiser

Sonic Visualiser is a free, open source, cross-platform desktop
application for music audio visualisation, annotation, and analysis.

With Sonic Visualiser you can:

 * Load audio files in various formats (WAV/AIFF, Ogg, Opus, MP3 etc)
   and view their waveforms
 * Look at audio visualisations such as spectrogram views, with
   interactive adjustment of display parameters
 * Annotate audio data by adding labelled time points and defining
   segments, point values and curves
 * Run feature-extraction plugins to calculate annotations
   automatically, using algorithms such as beat trackers, pitch detectors
   and so on (see https://vamp-plugins.org/)
 * Import annotation data from various text formats and MIDI files
 * Play back the original audio with synthesised annotations, taking
   care to synchronise playback with the display position
 * Slow down and speed up playback and loop segments of interest,
   including seamless looping of complex non-contiguous areas
 * Export annotations and audio selections to external files.

Sonic Visualiser can also be controlled remotely using the Open Sound
Control (OSC) protocol (if support is compiled in).


Credits
-------

If you are using Sonic Visualiser in research work for publication,
please see the file CITATION for a reference to cite.

Sonic Visualiser was devised and developed in the [Centre for Digital
Music](https://c4dm.eecs.qmul.ac.uk/) at Queen Mary University of
London, primarily by Chris Cannam, with contributions from Christian
Landone, Mathieu Barthet, Dan Stowell, Jesús Corral García, Matthias
Mauch, and Craig Sapp. Special thanks to Professor Mark Sandler for
initiating and supporting the project.

Sonic Visualiser is currently lightly maintained by Chris Cannam at
Particular Programs Ltd. There are no major features in development as
of this release, only bug fixes. If you need something specific and
have funding available for it, please contact us.

The Sonic Visualiser code is in general

 * Copyright (c) 2005-2007 Chris Cannam
 * Copyright (c) 2006-2020 Queen Mary University of London
 * Copyright (c) 2020-2023 Particular Programs Ltd

with a few exceptions as indicated in the individual source files.

Russian translation provided by Alexandre Prokoudine, copyright
2006-2019 Alexandre Prokoudine.

Czech translation provided by Pavel Fric, copyright 2010-2019 Pavel
Fric.

This work was partially funded by the European Commission through the
SIMAC project IST-FP6-507142 and the EASAIER project IST-FP6-033902.

This work was partially funded by the Arts and Humanities Research
Council through its Research Centre for the History and Analysis of
Recorded Music (CHARM).

This work was partially funded by the Engineering and Physical
Sciences Research Council through the OMRAS2 project EP/E017614/1, the
Musicology for the Masses project EP/I001832/1, and the Sound Software
project EP/H043101/1.

Sonic Visualiser is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.  See the file COPYING included with
this distribution for more information.

Sonic Visualiser may also make use of the following libraries:

 * Qt5 - Copyright The Qt Company, distributed under the LGPL
 * JACK - Copyright Paul Davis, Jack O'Quin et al, under the LGPL
 * PortAudio - Copyright Ross Bencina, Phil Burk et al, BSD license
 * Ogg decoder - Copyright CSIRO Australia, BSD license
 * MAD mp3 decoder - Copyright Underbit Technologies Inc, GPL
 * Opus decoder - Copyright Xiph.org and others, BSD license
 * libsamplerate - Copyright Erik de Castro Lopo, BSD license
 * libsndfile - Copyright Erik de Castro Lopo, LGPL
 * FFTW3 - Copyright Matteo Frigo and MIT, GPL
 * Rubber Band Library - Copyright Particular Programs Ltd, GPL
 * Vamp plugin SDK - Copyright Chris Cannam and QMUL, BSD license
 * LADSPA plugin SDK - Copyright Richard Furse et al, LGPL
 * RtMIDI - Copyright Gary P. Scavone, BSD license
 * Dataquay - Copyright Particular Programs Ltd, BSD license
 * Sord and Serd - Copyright David Robillard, BSD license
 * Redland - Copyright Dave Beckett and the University of Bristol, LGPL/Apache license
 * liblo OSC library - Copyright Steve Harris, GPL
 * Cap'n Proto - Copyright Sandstorm Development Group, Inc, BSD license

(Some distributions of Sonic Visualiser may have one or more of these
libraries statically linked.)  Many thanks to their authors.


Compiling Sonic Visualiser
--------------------------

If you are planning to compile Sonic Visualiser from source code,
please read the relevant instructions:

 * Linux and similar systems: [COMPILE_linux.md](https://github.com/sonic-visualiser/sonic-visualiser/blob/default/COMPILE_linux.md)
 * macOS: [COMPILE_macos.md](https://github.com/sonic-visualiser/sonic-visualiser/blob/default/COMPILE_macos.md)
 * Windows: [COMPILE_windows.md](https://github.com/sonic-visualiser/sonic-visualiser/blob/default/COMPILE_windows.md)

These three platform builds are checked via continuous integration:

 * Linux CI build: [![Build Status](https://github.com/sonic-visualiser/sonic-visualiser/workflows/Linux%20CI/badge.svg)](https://github.com/sonic-visualiser/sonic-visualiser/actions?query=workflow%3A%22Linux+CI%22)
 * macOS CI build: [![Build Status](https://github.com/sonic-visualiser/sonic-visualiser/workflows/macOS%20CI/badge.svg)](https://github.com/sonic-visualiser/sonic-visualiser/actions?query=workflow%3A%22macOS+CI%22)
 * Windows CI build: [![Build status](https://ci.appveyor.com/api/projects/status/26pygienkigw39p7?svg=true)](https://ci.appveyor.com/project/cannam/sonic-visualiser)

For notes on how to update and edit the UI translation strings, see [TRANSLATION.md](https://github.com/sonic-visualiser/sonic-visualiser/blob/default/TRANSLATION.md)


More information
----------------

For more information about Sonic Visualiser, please go to

  https://www.sonicvisualiser.org/

