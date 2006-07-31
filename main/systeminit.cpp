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

#include <QApplication>
#include <QFont>

#include <iostream>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>

static int handle_x11_error(Display *dpy, XErrorEvent *err)
{
    char errstr[256];
    XGetErrorText(dpy, err->error_code, errstr, 256);
    if (err->error_code != BadWindow) {
	std::cerr << "waveform: X Error: "
		  << errstr << " " << int(err->error_code)
		  << "\nin major opcode:  "
		  << int(err->request_code) << std::endl;
    }
    return 0;
}
#endif

#ifdef Q_WS_WIN32

#include <fcntl.h>

// Set default file open mode to binary
#undef _fmode
int _fmode = _O_BINARY;

void redirectStderr()
{
    HANDLE stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    if (!stderrHandle) return;

    AllocConsole();

    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(stderrHandle, &info);
    info.dwSize.Y = 1000;
    SetConsoleScreenBufferSize(stderrHandle, info.dwSize);

    int h = _open_osfhandle((long)stderrHandle, _O_TEXT);
    if (h) {
        FILE *fd = _fdopen(h, "w");
        if (fd) {
            *stderr = *fd;
            setvbuf(stderr, NULL, _IONBF, 0);
        }
    }
}

#endif

extern void svSystemSpecificInitialisation()
{
#ifdef Q_WS_X11
    XSetErrorHandler(handle_x11_error);
#endif

#ifdef Q_WS_WIN32
    redirectStderr();
    QFont fn = qApp->font();
    fn.setFamily("Tahoma");
    qApp->setFont(fn);
#else
#ifdef Q_WS_X11
    QFont fn = qApp->font();
    fn.setPointSize(fn.pointSize() + 2);
    qApp->setFont(fn);
#endif
#endif
}

