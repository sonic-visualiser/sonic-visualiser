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

#include "NetworkPermissionTester.h"

#include "../version.h"

#include <QWidget>
#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>

bool
NetworkPermissionTester::havePermission()
{
    QSettings settings;
    settings.beginGroup("Preferences");
    
    QString tag = QString("network-permission-%1").arg(SV_VERSION);

    bool permish = false;

    if (settings.contains(tag)) {
	permish = settings.value(tag, false).toBool();
    } else {

	QDialog d;
	d.setWindowTitle(QCoreApplication::translate("NetworkPermissionTester", "Welcome to Sonic Visualiser"));

	QGridLayout *layout = new QGridLayout;
	d.setLayout(layout);

	QLabel *label = new QLabel;
	label->setWordWrap(true);
	label->setText
	    (QCoreApplication::translate
	     ("NetworkPermissionTester",
	      "<h2>Welcome to Sonic Visualiser!</h2>"
	      "<p><img src=\":icons/qm-logo-smaller.png\" style=\"float:right\">Sonic Visualiser is a program for viewing and exploring audio data for semantic music analysis and annotation.</p>"
	      "<p>Developed in the Centre for Digital Music at Queen Mary, University of London, Sonic Visualiser is provided free as open source software under the GNU General Public License.</p>"
              "<p><hr></p>"
	      "<p><b>Before we go on...</b></p>"
	      "<p>Sonic Visualiser needs to make occasional network requests to our servers.</p>"
	      "<p>This is to:</p>"
	      "<ul><li> look up information about available and installed plugins; and</li>"
	      "<li> tell you when updates are available.</li></ul>"
	      "<p>No personal information will be sent, no tracking is carried out, and all requests happen in the background without interrupting your work.</p>"
	      "<p>We recommend that you allow this, because it makes Sonic Visualiser more useful. But if you do not wish to do so, please un-check the box below.<br></p>"));
	layout->addWidget(label, 0, 0);

	QCheckBox *cb = new QCheckBox(QCoreApplication::translate("NetworkPermissionTester", "Allow this"));
	cb->setChecked(true);
	layout->addWidget(cb, 1, 0);
	
	QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
	QObject::connect(bb, SIGNAL(accepted()), &d, SLOT(accept()));
	layout->addWidget(bb, 2, 0);
	
	d.exec();

	bool permish = cb->isChecked();
	settings.setValue(tag, permish);
    }

    settings.endGroup();

    return permish;
}

   

