#include "OFMpreferences.h"
#include <QHBoxLayout>
#include <QSettings>

OFMpreferences::OFMpreferences(MainWindow *inWindow, QWidget *parent)
    : QDialog(parent)
{
    createWidget();
}

void OFMpreferences::createWidget()
{
    readSettings();

    okButton = new QPushButton(tr("OK"));
    cancelButton = new QPushButton(tr("Cancel"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch(1);

    // separator
    QGroupBox *separatorBox = new QGroupBox(tr("CSV separator"));
    QVBoxLayout *separatorLayout = new QVBoxLayout;
    commaRadio = new QRadioButton(tr("&Comma - ,"));
    semicolonRadio = new QRadioButton(tr("&Semicolon - ;"));
    if(separator == ',')
        commaRadio->setChecked(true);
    else
        semicolonRadio->setChecked(true);

    separatorLayout->addWidget(commaRadio);
    separatorLayout->addWidget(semicolonRadio);
    separatorLayout->addStretch(1);
    separatorBox->setLayout(separatorLayout);

    QLabel *radiusLabel = new QLabel(tr("Radius of the rotor"));
    radiusSpin = new QSpinBox;
    radiusSpin->setMinimum(20);
    radiusSpin->setMaximum(200);
    radiusSpin->setSingleStep(1);
    radiusSpin->setValue(radius);
    radiusSpin->setSuffix(" mm");

    QLabel *minTimeLabel = new QLabel(tr("Minimum time between two records"));
    minTimeSpin = new QDoubleSpinBox;
    minTimeSpin->setMinimum(0.0);
    minTimeSpin->setMaximum(1.0);
    minTimeSpin->setSingleStep(0.01);
    minTimeSpin->setValue(minTime);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(radiusLabel);
    mainLayout->addWidget(radiusSpin);
    mainLayout->addWidget(minTimeLabel);
    mainLayout->addWidget(minTimeSpin);
    mainLayout->addWidget(separatorBox);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    setWindowTitle(tr("Preferences"));
}

void OFMpreferences::accept()
{
    radius = radiusSpin->value();
    minTime = minTimeSpin->value();

    if(commaRadio->isChecked())
    {
        separator = ',';
    }
    else
    {
        separator = ';';
    }

    writeSettings();
    QDialog::accept();
}

void OFMpreferences::readSettings()
{
    QString companyName = "DrawAhead";
    QString windowName = "OpenFlightMill";
    QSettings settings(companyName, windowName);

    QString defaultPath("/home/OpenFlightMill-data");
    radius = settings.value("radius", 150).toInt();
    minTime = settings.value("minTime", 0.1).toDouble();
    separator = settings.value("separator", ',').toChar();
}

void OFMpreferences::writeSettings()
{
    QString companyName = "DrawAhead";
    QString windowName = "OpenFlightMill";
    QSettings settings(companyName, windowName);

    settings.setValue("radius", radius);
    settings.setValue("minTime", minTime);
    settings.setValue("separator", separator);
}

