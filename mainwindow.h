#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QHeaderView>
#include <QTableWidget>
#include <QProcess>
#include <QDebug>

#include <vector>
#include "qdatetime.h"
#include <QFile>
#include <QCloseEvent>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void readyReadStandardError();
    void readyReadStandardOutput();
    void editClicked(int row, int col=1);
    void txt2data();
    void preferences();
    bool saveAll();
    void clearAll();
    void about();

private:
    void readSettings();
    void writeSettings();
    void createMenu();
    void createTable();
    bool saveRow(unsigned row);
    void clearRow(unsigned row);
    bool okToContinue();
    bool isModified();

    QMenu *menuFile;
    QMenu *menuHelp;
    QAction *actTxt2data;
    QAction *actPreferences;
    QAction *actSaveAll;
    QAction *actClearAll;
    QAction *actExit;
    QAction *actAbout;

    QTableWidget *table;
    unsigned millCount; // maximim number of flightmils
    QProcess *m_process;

    QString companyName;
    QString appName;
    QString appVersion;
    QString logFileName;


    // columns
    enum {
        first = 1,
        last = 2,
        time = 3,
        count = 4,
        distance = 5,
        clear = 6,
        save = 7
    };

    QString timeFormat;

    std::vector<double> startSec; // first record in miliseconds
    std::vector<QDateTime> startTime; // first record
    std::vector<QDateTime> lastTime; // last record
    std::vector<std::vector<double>> times; // reading for all half lap

    // preferences
    int radius;
    double halfLapM; // half lap distance in meters
    double minTime;
    QString dataPath;
    QChar separator;

};

#endif // MAINWINDOW_H
