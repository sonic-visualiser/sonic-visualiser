/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam and QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MainWindow.h"

#include "system/System.h"
#include "system/Init.h"
#include "base/TempDirectory.h"
#include "base/PropertyContainer.h"
#include "base/Preferences.h"

#include <QMetaType>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QIcon>
#include <QSessionManager>

#include <iostream>
#include <signal.h>

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

class SVApplication : public QApplication
{
public:
    SVApplication(int argc, char **argv) :
        QApplication(argc, argv),
        m_mainWindow(0) { }
    virtual ~SVApplication() { }

    void setMainWindow(MainWindow *mw) { m_mainWindow = mw; }
    void releaseMainWindow() { m_mainWindow = 0; }

    virtual void commitData(QSessionManager &manager) {
        if (!m_mainWindow) return;
        bool mayAskUser = manager.allowsInteraction();
        bool success = m_mainWindow->commitData(mayAskUser);
        manager.release();
        if (!success) manager.cancel();
    }

protected:
    MainWindow *m_mainWindow;
};

int
main(int argc, char **argv)
{
    SVApplication application(argc, argv);

    QStringList args = application.arguments();

    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

#ifndef Q_WS_WIN32
    signal(SIGHUP,  signalHandler);
    signal(SIGQUIT, signalHandler);
#endif

    svSystemSpecificInitialisation();

    bool audioOutput = true;
    bool oscSupport = true;

    if (args.contains("--help")) {
        std::cerr << QApplication::tr(
            "\nSonic Visualiser is a program for viewing and exploring audio data\nfor semantic music analysis and annotation.\n\nUsage:\n\n  %1 [--no-audio] [--no-osc] [<file> ...]\n\n  --no-audio: Do not attempt to open an audio output device\n  --no-osc: Do not provide an Open Sound Control port for remote control\n  <file>: One or more Sonic Visualiser (.sv) and audio files may be provided.\n").arg(argv[0]).toStdString() << std::endl;
        exit(2);
    }

    if (args.contains("--no-audio")) audioOutput = false;
    if (args.contains("--no-osc")) oscSupport = false;

    QApplication::setOrganizationName("sonic-visualiser");
    QApplication::setOrganizationDomain("sonicvisualiser.org");
    QApplication::setApplicationName("sonic-visualiser");

    QApplication::setWindowIcon(QIcon(":icons/waveform.png"));

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

    MainWindow gui(audioOutput, oscSupport);
    application.setMainWindow(&gui);

    QDesktopWidget *desktop = QApplication::desktop();
    QRect available = desktop->availableGeometry();

    int width = available.width() * 2 / 3;
    int height = available.height() / 2;
    if (height < 450) height = available.height() * 2 / 3;
    if (width > height * 2) width = height * 2;

    QSettings settings;
    settings.beginGroup("MainWindow");
    QSize size = settings.value("size", QSize(width, height)).toSize();
    gui.resize(size);
    if (settings.contains("position")) {
        gui.move(settings.value("position").toPoint());
    }
    settings.endGroup();
    
    gui.show();

    bool haveSession = false;
    bool haveMainModel = false;

    for (QStringList::iterator i = args.begin(); i != args.end(); ++i) {

        MainWindow::FileOpenStatus status = MainWindow::FileOpenFailed;

        if (i == args.begin()) continue;
        if (i->startsWith('-')) continue;

        QString path = *i;

        if (path.endsWith("sv")) {
            if (!haveSession) {
                status = gui.openSessionFile(path);
                if (status == MainWindow::FileOpenSucceeded) {
                    haveSession = true;
                    haveMainModel = true;
                }
            } else {
                std::cerr << "WARNING: Ignoring additional session file argument \"" << path.toStdString() << "\"" << std::endl;
                status = MainWindow::FileOpenSucceeded;
            }
        }
        if (status != MainWindow::FileOpenSucceeded) {
            if (!haveMainModel) {
                status = gui.openSomeFile(path, MainWindow::ReplaceMainModel);
                if (status == MainWindow::FileOpenSucceeded) haveMainModel = true;
            } else {
                status = gui.openSomeFile(path, MainWindow::CreateAdditionalModel);
            }
        }
        if (status == MainWindow::FileOpenFailed) {
	    QMessageBox::critical
                (&gui, QMessageBox::tr("Failed to open file"),
                 QMessageBox::tr("File \"%1\" could not be opened").arg(path));
        }
    }            

    int rv = application.exec();
    std::cerr << "application.exec() returned " << rv << std::endl;

    cleanupMutex.lock();
    TempDirectory::getInstance()->cleanup();
    application.releaseMainWindow();

    return rv;
}
