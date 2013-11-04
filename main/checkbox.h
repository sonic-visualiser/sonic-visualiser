#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QSpinBox>
#include "MainWindow.h"

class CheckBox : public QWidget
{
    Q_OBJECT

public:
    CheckBox(QWidget *parent = 0);
    QLineEdit *line = new QLineEdit("", this);
    QLineEdit *line2 = new QLineEdit("", this);
    QLabel *sel_rule1, *sel_rule2;
    QLabel *mix_rule1, *mix_rule2, *mix_rule3, *mix_rule4;
    QLabel *group_label, *grp_vol, *grp_name;
    QLineEdit *group_name_line;

private slots:
    void defineImafTextFile();
    void defineImafImageFile();
    void saveImafFile();
    void insertLyrics(int state);
    void insertImage(int state);
    void set_selruleType(QAbstractButton *button);
    void set_selrulePAR1(int value);
    void set_selrulePAR2(int value);
    void set_mixruleType(QAbstractButton *button);
    void set_mixrulePAR1(int value);
    void set_mixrulePAR2(int value);
    void set_mixrulePAR3(int value);
    void set_mixrulePAR4(int value);
    void set_TrackInGroup(QAbstractButton *button);
    void set_GroupName(QString name);
    void set_GroupVolume(int value);
    void set_presetType(QAbstractButton *button);
    void set_fade(QAbstractButton *button);
};

#endif // CHECKBOX_H
