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

#ifndef _SURVEYER_H_
#define _SURVEYER_H_

#include <QStringList>
#include <QString>
#include <QObject>
#include <QNetworkReply>

class QNetworkAccessManager;

class Surveyer : public QObject
{
    Q_OBJECT

public:
    Surveyer(QString hostname, QString testPath, QString surveyPath);
    virtual ~Surveyer();

protected slots:
    void finished();
    void error(QNetworkReply::NetworkError);

private:
    bool m_httpFailed;
    QString m_hostname;
    QString m_testPath;
    QString m_surveyPath;
    QNetworkReply *m_reply;
    QNetworkAccessManager *m_nm;
};

#endif

