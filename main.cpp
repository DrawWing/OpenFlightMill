#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("DrawAhead");
    a.setApplicationName("OpenFlightMill");
    a.setApplicationVersion(VERSION_STRING);

    QFont appFont = QApplication::font();
    appFont.setPointSize(10);
    QApplication::setFont(appFont);


    MainWindow w;
    w.show();

    return a.exec();
}
