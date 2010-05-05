/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
   This is a modified version of a source file from the 
   Rosegarden MIDI and audio sequencer and notation editor.
   This file copyright 2000-2009 Chris Cannam.
*/

#include "Surveyer.h"

#include <iostream>

#include <QHttp>

#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>

#include "version.h"

#include "transform/TransformFactory.h"
#include "plugin/PluginIdentifier.h"

Surveyer::Surveyer(QObject *parent) :
    QObject(parent),
    m_httpFailed(false)
{
    QSettings settings;
    settings.beginGroup("Survey");
    if (!settings.contains("countdown")) {
        settings.setValue("countdown", 15);
        settings.endGroup();
        return;
    }
    int countdown = settings.value("countdown").toInt();
    if (countdown == 0) {
        // The countdown value will now remain 0 until we have
        // successfully tested for a survey and offered it to the
        // user.  If the survey doesn't exist any more, then we'll
        // simply never present it to the user and the countdown will
        // remain 0 forever.  If the survey does exist, then we offer
        // the user the chance to respond to it and (regardless of
        // whether they want to or not) set the countdown to -1 so
        // that it is never offered again.
        QHttp *http = new QHttp();
        connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
                this, SLOT(httpResponseHeaderReceived(const QHttpResponseHeader &)));
        connect(http, SIGNAL(done(bool)),
                this, SLOT(httpDone(bool)));
        http->setHost("sonicvisualiser.org");
        http->get("/survey16-present.txt");
    } else if (countdown > 0) {
        settings.setValue("countdown", countdown-1);
    }
    settings.endGroup();
}

Surveyer::~Surveyer()
{
}

void
Surveyer::httpResponseHeaderReceived(const QHttpResponseHeader &h)
{
    if (h.statusCode() / 100 != 2) m_httpFailed = true;
}

void
Surveyer::httpDone(bool error)
{
    QHttp *http = const_cast<QHttp *>(dynamic_cast<const QHttp *>(sender()));
    if (!http) return;
    http->deleteLater();
//    if (error || m_httpFailed) return;

    QByteArray responseData = http->readAll();
    QString str = QString::fromUtf8(responseData.data());
    QStringList lines = str.split('\n', QString::SkipEmptyParts);
    if (lines.empty()) return;

    QString response = lines[0];
//    if (response != "yes") return;

    QString title = "Sonic Visualiser - User Survey";
    QString text = "<h3>Sonic Visualiser: Take part in our survey!</h3><p>We at Queen Mary, University of London are running a short survey for users of Sonic Visualiser.  We are trying to find out how useful Sonic Visualiser is to people, and what we can do to improve it.</p><p>We do not ask for any personal information, and it should only take five minutes.</p><p>Would you like to take part?</p>";

    QMessageBox mb(dynamic_cast<QWidget *>(parent()));
    mb.setWindowTitle(title);
    mb.setText(text);

    QPushButton *yes = mb.addButton(tr("Yes! Take me to the survey"), QMessageBox::ActionRole);
    mb.addButton(tr("No, thanks"), QMessageBox::RejectRole);

    mb.exec();

    QSettings settings;
    settings.beginGroup("Survey");
    settings.setValue("countdown", -1);
    settings.endGroup();

    if (mb.clickedButton() == yes) {
        QString svarg = SV_VERSION;
        QString platformarg = "unknown";
#ifdef _WIN32
        platformarg = "win32";
#else
#ifdef __APPLE__
        platformarg = "osx";
#else
        platformarg = "posix";
#endif
#endif
        QString plugsarg;
        TransformFactory *tf = TransformFactory::getInstance();
        if (tf) {
            TransformList tl = tf->getAllTransformDescriptions();
            std::set<QString> packages;
            for (size_t i = 0; i < tl.size(); ++i) {
                TransformId id = tl[i].identifier;
                Transform t;
                t.setIdentifier(id);
                QString plugid = t.getPluginIdentifier();
                QString type, soname, label;
                PluginIdentifier::parseIdentifier(plugid, type, soname, label);
                if (type == "vamp") packages.insert(soname);
            }
            for (std::set<QString>::const_iterator i = packages.begin();
                 i != packages.end(); ++i) {
                if (plugsarg != "") plugsarg = plugsarg + ",";
                plugsarg = plugsarg + *i;
            }
        }
        QDesktopServices::openUrl(QUrl(QString("http://sonicvisualiser.org/survey16.php?sv=%1&plugs=%2&platform=%3").arg(svarg).arg(plugsarg).arg(platformarg)));
    }
}


