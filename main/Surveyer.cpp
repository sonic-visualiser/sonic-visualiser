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

#include "Surveyer.h"

#include <iostream>

#include <QNetworkAccessManager>

#include <QSettings>
#include <QMessageBox>
#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>

#include "../version.h"

#include "transform/TransformFactory.h"
#include "plugin/PluginIdentifier.h"

using namespace sv;

Surveyer::Surveyer(Config config) :
    m_config(config),
    m_httpFailed(false),
    m_reply(nullptr),
    m_nm(new QNetworkAccessManager)
{
    QSettings settings;
    settings.beginGroup("Survey");
    if (!settings.contains(m_config.countdownKey)) {
        settings.setValue(m_config.countdownKey, m_config.countdownFrom);
        settings.endGroup();
        return;
    }
    int countdown = settings.value(m_config.countdownKey).toInt();
    if (countdown == 0) {
        // The countdown value will now remain 0 until we have
        // successfully tested for a survey and offered it to the
        // user.  If the survey doesn't exist any more, then we'll
        // simply never present it to the user and the countdown will
        // remain 0 forever.  If the survey does exist, then we offer
        // the user the chance to respond to it and (regardless of
        // whether they want to or not) set the countdown to -1 so
        // that it is never offered again.
        QUrl url(QString("http://%1/%2")
                 .arg(m_config.hostname).arg(m_config.testPath));
        SVDEBUG << "Surveyer: Test URL is " << url << endl;
        m_reply = m_nm->get(QNetworkRequest(url));
        connect(m_reply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)),
                this, SLOT(error(QNetworkReply::NetworkError)));
        connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
    } else if (countdown > 0) {
        settings.setValue(m_config.countdownKey, countdown-1);
    }
    settings.endGroup();
}

Surveyer::~Surveyer()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
    delete m_nm;
}

void
Surveyer::error(QNetworkReply::NetworkError)
{
    SVDEBUG << "Surveyer: error: " << m_reply->errorString() << endl;
    m_httpFailed = true;
}

void
Surveyer::finished()
{
    if (m_httpFailed) return;

    QMessageBox mb(dynamic_cast<QWidget *>(parent()));
    mb.setWindowTitle(m_config.title);
    mb.setText(m_config.text);

    QPushButton *yes =
        mb.addButton(m_config.acceptLabel, QMessageBox::ActionRole);
    mb.addButton(m_config.rejectLabel, QMessageBox::RejectRole);

    mb.exec();

    QSettings settings;
    settings.beginGroup("Survey");
    settings.setValue(m_config.countdownKey, -1);
    settings.endGroup();

    if (mb.clickedButton() == yes) {
        QString svarg = SV_VERSION;
        QString platformarg = "unknown";
#ifdef Q_OS_WIN32
        platformarg = "win32";
#else
#ifdef Q_OS_MAC
        platformarg = "osx";
#else
        platformarg = "posix";
#endif
#endif
        QString plugsarg;
        TransformFactory *tf = TransformFactory::getInstance();
        if (tf) {
            TransformList tl = tf->getInstalledTransformDescriptions();
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

        if (m_config.includeSystemInfo) {
            QDesktopServices::openUrl(QUrl(QString("http://%1/%2?sv=%3&plugs=%4&platform=%5").arg(m_config.hostname).arg(m_config.surveyPath).arg(svarg).arg(plugsarg).arg(platformarg)));
        } else {
            QDesktopServices::openUrl(QUrl(QString("http://%1/%2?sv=%3").arg(m_config.hostname).arg(m_config.surveyPath).arg(svarg)));
        }
    }
}


