#ifndef OFMPREFERENCES_H
#define OFMPREFERENCES_H

#include <QDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QAction>

#include "mainwindow.h"

//QT_BEGIN_NAMESPACE
//class QPushButton;
//class QLineEdit;
//class QGroupBox;
//class QDoubleSpinBox;
//class QRadioButton;
//class QAction;
//QT_END_NAMESPACE

class OFMpreferences : public QDialog
{
    Q_OBJECT
public:
    OFMpreferences(QWidget *parent = 0);

protected:
    void accept();

private:
    void readSettings();
    void writeSettings();
    void createWidget();

    QRadioButton *commaRadio;
    QRadioButton *semicolonRadio;
    QSpinBox *radiusSpin;
    QSpinBox *inactiveSpin;
    QDoubleSpinBox *minTimeSpin;

    QPushButton *okButton;
    QPushButton *cancelButton;

    MainWindow *theWindow; // ro reomve

    // preferences
    int radius;
    int inactiveMin;
    double minTime;
    QChar separator;
};

#endif // OFMPREFERENCES_H
