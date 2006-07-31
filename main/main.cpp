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

#include "MainWindow.h"

#include "base/System.h"
#include "base/TempDirectory.h"
#include "base/PropertyContainer.h"
#include "base/Preferences.h"
#include "fileio/ConfigFile.h"

#include <QMetaType>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>

#include <iostream>
#include <signal.h>

//!!! catch trappable signals, cleanup temporary directory etc
//!!! check for crap left over from previous run

static QMutex cleanupMutex;

static void
signalHandler(int /* signal */)
{
    // Avoid this happening more than once across threads

    cleanupMutex.lock();
    std::cerr << "signalHandler: cleaning up and exiting" << std::endl;
    TempDirectory::getInstance()->cleanup();
    exit(0); // without releasing mutex
}

extern void svSystemSpecificInitialisation();

int
main(int argc, char **argv)
{
    QApplication application(argc, argv);

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

#ifndef Q_WS_WIN32
    signal(SIGHUP,  signalHandler);
    signal(SIGQUIT, signalHandler);
#endif

    svSystemSpecificInitialisation();

    QString language = QLocale::system().name();

    QTranslator qtTranslator;
    QString qtTrName = QString("qt_%1").arg(language);
    std::cerr << "Loading " << qtTrName.toStdString() << "..." << std::endl;
    qtTranslator.load(qtTrName);
    application.installTranslator(&qtTranslator);

    QTranslator svTranslator;
    QString svTrName = QString("sonic-visualiser_%1").arg(language);
    std::cerr << "Loading " << svTrName.toStdString() << "..." << std::endl;
    svTranslator.load(svTrName, ":i18n");
    application.installTranslator(&svTranslator);

    // Permit size_t and PropertyName to be used as args in queued signal calls
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<PropertyContainer::PropertyName>("PropertyContainer::PropertyName");

    MainWindow gui;

    QDesktopWidget *desktop = QApplication::desktop();
    QRect available = desktop->availableGeometry();

    int width = available.width() * 2 / 3;
    int height = available.height() / 2;
    if (height < 450) height = available.height() * 2 / 3;
    if (width > height * 2) width = height * 2;

    gui.resize(width, height);
    gui.show();

    if (argc > 1) {
	QString path = argv[1];
        bool success = false;
        if (path.endsWith(".sv")) {
            success = gui.openSessionFile(path);
        }
        if (!success) {
            success = gui.openSomeFile(path);
        }
        if (!success) {
	    QMessageBox::critical(&gui, QMessageBox::tr("Failed to open file"),
				  QMessageBox::tr("File \"%1\" could not be opened").arg(path));
	}
    }

    int rv = application.exec();
    std::cerr << "application.exec() returned " << rv << std::endl;

    cleanupMutex.lock();
    TempDirectory::getInstance()->cleanup();
    Preferences::getInstance()->getConfigFile()->commit();
    return rv;
}
