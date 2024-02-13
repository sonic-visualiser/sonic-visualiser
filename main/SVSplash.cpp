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

#include "SVSplash.h"

#include "../version.h"

#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QSvgRenderer>

#include "system/System.h"

#include <cmath>

#include <iostream>
using namespace std;

SVSplash::SVSplash()
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    bool darkTheme = OSReportsDarkThemeActive();
    
    QPixmap *p1 = new QPixmap(QString(":icons/scalable/sv-splash.png"));

    int w = p1->width(), h = p1->height();
    QScreen *screen = QApplication::primaryScreen();
    QRect desk = screen->availableGeometry();

    double dpratio = devicePixelRatio();
    double widthMultiple = double(desk.width()) / double(w);

    int sw = w, sh = h;

    if (widthMultiple > 2.5 || dpratio > 1.0 || darkTheme) {

        // Hi-dpi either via pixel doubling or simply via lots of
        // pixels - or dark theme splash, for which we only have an
        // svg version

        double factor = widthMultiple / 2.5;
        if (factor < 1.0) factor = 1.0;
        sw = int(floor(w * factor));
        sh = int(floor(h * factor));

        delete p1;
        m_pixmap = new QPixmap(int(floor(sw * dpratio)),
                               int(floor(sh * dpratio)));

//        cerr << "pixmap size = " << m_pixmap->width() << " * "
//             << m_pixmap->height() << endl;
        
        m_pixmap->fill(Qt::white);
        QString filename = "sv-splash";
        if (darkTheme) {
            filename = "sv-splash-dark";
        }
        QSvgRenderer renderer(QString(":icons/scalable/%1.svg").arg(filename));
        QPainter painter(m_pixmap);
        renderer.render(&painter);
        painter.end();

    } else {
        // The "low dpi" case
        m_pixmap = p1;
    }
    
    setFixedWidth(sw);
    setFixedHeight(sh);
    setGeometry(desk.x() + desk.width()/2 - sw/2,
                desk.y() + desk.height()/2 - sh/2,
                sw, sh);
}

SVSplash::~SVSplash()
{
    delete m_pixmap;
}

void
SVSplash::finishSplash(QWidget *w)
{
    finish(w);
}

void
SVSplash::drawContents(QPainter *painter)
{
    painter->drawPixmap(rect(), *m_pixmap, m_pixmap->rect());
    QString text = QString("v%1").arg(SV_VERSION);
    painter->setPen(OSReportsDarkThemeActive() ? Qt::white : Qt::black);
    painter->drawText
        (width() - painter->fontMetrics().horizontalAdvance(text) - (width()/50),
         (width()/70) + painter->fontMetrics().ascent(),
         text);
}


