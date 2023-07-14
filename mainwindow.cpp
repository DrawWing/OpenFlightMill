#include "mainwindow.h"
#include "OFMpreferences.h"

#include <QCoreApplication>
#include <QApplication>
//#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/OFMill.png"));
//    qDebug() << tr("QT version: %1").arg(qVersion());
    companyName = QCoreApplication::organizationName();
    appName = QCoreApplication::applicationName();
    appVersion = QCoreApplication::applicationVersion();
    millCount = 10; // number of flightmills
    timeFormat = "yyyy.MM.dd hh:mm:ss.zzz";

    readSettings();

    dataPath = QCoreApplication::applicationDirPath() + "/OFM-data";
    QDir dataDir(dataPath);
    if(!dataDir.exists())
    {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir appDir(appPath);
        if (!appDir.mkdir("OFM-data"))
        {
            dataPath =  QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                          "/home",
                                                          QFileDialog::ShowDirsOnly
                                                          | QFileDialog::DontResolveSymlinks);
        }
    }
    dataDir.setPath(dataPath);

    logFileName = dataDir.absoluteFilePath("OpenFlightMill-log.txt");
    QFile logFile(logFileName);
    logFile.open(QFile::WriteOnly | QFile::Text);
    logFile.close();

    createMenu();
    createTable();

    int w = table->verticalHeader()->width() + 2; // +4 seems to be needed
    for (int i = 0; i < table->columnCount(); i++)
        w += table->columnWidth(i); // seems to include gridline (on my machine)
    int h = table->horizontalHeader()->height();
    for (int i = 0; i < table->rowCount(); i++)
        h += table->rowHeight(i);
    h += menuBar()->height();
    setMaximumSize(QSize(w, h));
    resize(w, h);

    m_process = new QProcess(this);
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));

    QTimer *inactiveTimer = new QTimer(this);
    connect(inactiveTimer, SIGNAL(timeout()), this, SLOT(inactiveTest()));
    inactiveTimer->start(60000); // 60 000 milliseconds = 1 minute

    // ititialize all rows with invalid data
    for(unsigned i = 0; i < millCount; ++i)
    {
        startSec.push_back(0.0);
        startTime.push_back(QDateTime());
        lastTime.push_back(QDateTime());
        std::vector<double> timeVec;
        times.push_back(timeVec);
    }

    QString fullCommand("stdbuf -oL -eL gpiomon --format=\"%o %e %s.%n\" gpiochip0 1 2 3 4 5 6 7 8 9 10");
    m_process->start(fullCommand);
}

MainWindow::~MainWindow()
{
    m_process->close();
}

void MainWindow::createMenu()
{
    menuFile = menuBar()->addMenu(tr("&File"));
    actSaveAll = menuFile->addAction(tr("&Save all"));
    actSaveAll->setShortcut(tr("Ctrl+S"));
    connect(actSaveAll, SIGNAL(triggered()), this, SLOT(saveAll()));
    actClearAll = menuFile->addAction(tr("&Clear all"));
    actClearAll->setShortcut(tr("Del"));
    connect(actClearAll, SIGNAL(triggered()), this, SLOT(clearAll()));
    actTxt2data = menuFile->addAction(tr("&Data from text"));
    actTxt2data->setShortcut(tr("Ctrl+D"));
    connect(actTxt2data, SIGNAL(triggered()), this, SLOT(txt2data()));
    actPreferences = menuFile->addAction(tr("&Preferences"));
    actPreferences->setShortcut(tr("Ctrl+P"));
    connect(actPreferences, SIGNAL(triggered()), this, SLOT(preferences()));
    actExit = menuFile->addAction(tr("E&xit"));
    actExit->setShortcut(tr("Ctrl+Q"));
    connect(actExit, SIGNAL(triggered()), this, SLOT(close()));

    menuHelp = menuBar()->addMenu(tr("&Help"));
    actAbout = menuHelp->addAction(tr("&About"));
    actAbout->setShortcut(tr("Ctrl+H"));
    connect(actAbout, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createTable()
{
    table = new QTableWidget(this);
    table->setColumnCount(8);
    table->setWordWrap(true);
    table->setStyleSheet("QHeaderView::section { background-color:lightGray }");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headerStringList;
    headerStringList<<tr("No.")<<tr("First record")
                   <<tr("Last record")
                  <<tr("Elapsed time")
                 <<tr("Half lap count")
                <<tr("Distance")
               <<tr("Clear")
              <<tr("Save");
    table->setHorizontalHeaderLabels(headerStringList);

    QHeaderView *verticalHeader = table->verticalHeader();
    verticalHeader->hide();

    table->setSelectionMode(QAbstractItemView::NoSelection);
    connect(table, SIGNAL( cellClicked(int, int) ),
            this, SLOT( editClicked(int, int) ));

    for(int i = 0; i < int(millCount); ++i)
    {
        table->insertRow(i);
        QString theNumberStr = QString::number(i + 1) ; // row numbering from 1
        table->setItem(i, 0, new QTableWidgetItem(theNumberStr));
        table->setItem(i, clear, new QTableWidgetItem("Clear"));
        table->setItem(i, save, new QTableWidgetItem("Save"));
    }
    table->resizeColumnToContents(0);
    setCentralWidget(table);
}

void MainWindow::readyReadStandardError()
{
    qDebug() << tr("readyReadStandardError()...");
    QByteArray buffer = m_process->readAllStandardError();
    qDebug() << buffer;

    QMessageBox::critical(this, appName,
                         buffer);
}

void MainWindow::readyReadStandardOutput()
{
    qDebug() << tr("readyReadStandardOutput()...");
    QByteArray buffer = m_process->readAllStandardOutput();
    qDebug() << buffer;

    // process line
    QString bufferStr(buffer);

    QFile logFile(logFileName);
    logFile.open(QFile::Append | QFile::Text);
    QTextStream logStream(&logFile);
    logStream << bufferStr;

    QStringList lines = bufferStr.split("\n");
    for(int i = 0; i < lines.size(); ++i)
    {
        QString theLine = lines[i];
        QStringList theLineList = theLine.split(" ");
        if(theLineList.size() < 3)
            continue;
        unsigned theRow = theLineList[0].toInt() - 1; // rows are counted from 0
        //        int bw = theLineList[1].toInt();
        double seconds = theLineList[2].toDouble();

        if(times[theRow].size() > 0)
        {
            double timeDif = seconds - times[theRow].back();
            if(timeDif < minTime)
                return;
        }

        times[theRow].push_back(seconds);
        QString theNumberStr = QString::number(times[theRow].size());
        table->setItem(theRow, count, new QTableWidgetItem(theNumberStr));

        theNumberStr = QString::number(times[theRow].size() * halfLapM, 'f', 2);
        table->setItem(theRow, distance, new QTableWidgetItem(theNumberStr));


        QDateTime theTime = QDateTime::currentDateTime();
        QString theTimeStr = theTime.toString("hh:mm:ss.zzz");

        // makr the row as modified
        table->item(theRow,save)->setBackground(Qt::yellow);

        lastTime[unsigned(theRow)] = theTime;
        if(startTime[unsigned(theRow)].isValid())
        {
            table->setItem(theRow, last, new QTableWidgetItem(theTimeStr));
            int msDiff = startTime[unsigned(theRow)].msecsTo(theTime);
            QTime elapsedTime(0, 0);
            elapsedTime = elapsedTime.addMSecs(msDiff);
            QString elapsedTimeStr = elapsedTime.toString("hh:mm:ss.zzz");
            table->setItem(theRow, time, new QTableWidgetItem(elapsedTimeStr));
        }
        else
        {
            table->setItem(theRow, first, new QTableWidgetItem(theTimeStr));
            startSec[unsigned(theRow)] = seconds;
            startTime[unsigned(theRow)] = theTime;
        }
    }
}

// mark rows inactive for inactiveThd ms red
void MainWindow::inactiveTest()
{
    QDateTime theTime = QDateTime::currentDateTime();
    for(unsigned i = 0; i < millCount; ++i)
    {
        if(!startTime[i].isValid())
            continue;
        int msDiff = lastTime[i].msecsTo(theTime);
        if(msDiff > inactiveThd)
            table->item(i,save)->setBackground(Qt::red);
    }
}

// col is only for compatibility with cellClicked(row, col)
void MainWindow::editClicked(int row, int col)
{
    if(col == clear)
    {
        if(!startTime[row].isValid())
            return;
        int ret = QMessageBox::warning(this, appName,
                                     tr("Do you want to discard data from this row?"),
                                     QMessageBox::Yes,
                                     QMessageBox::Cancel | QMessageBox::Default);

        if(ret == QMessageBox::Cancel)
            return;

        clearRow(row);
    }
    else if(col == save)
    {
        saveRow(row);
    }
}

// convert text file to data in csv file
void MainWindow::txt2data()
{
    QDir outputDir(dataPath);
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open file"),
                                                    dataPath, tr("Text files (*.txt)"));
    if( fileName.isEmpty() )
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::warning(this, appName,
                             tr("Cannot open %1.")
                             .arg(file.fileName()));
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    int inRadius;
    if(line.startsWith("radius: "))
    {
        line = line.remove("radius: ");
        inRadius = line.toInt();
    }
    else
    {
        QMessageBox::warning(this, appName,
                             tr("No radius information in file %1.")
                             .arg(file.fileName()));
        return;
    }

    line = in.readLine();
    QDateTime inTime;
    if(line.startsWith("time: ")){
        line = line.remove("time: ");
        inTime = QDateTime::fromString(line, timeFormat);
    }
    else
    {
        QMessageBox::warning(this, appName,
                             tr("No time information in file %1.")
                             .arg(file.fileName()));
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::vector< double > inData;
    while(!in.atEnd())
    {
        QString line = in.readLine();
        bool ok;
        double theData = line.toDouble(&ok);
        if(ok)
            inData.push_back(theData);
    }
    file.close();

    const double PI = 3.141592653589793238463;
    double halfLapDist = PI * inRadius * 0.001;
    double distance = 0.0;
    double prevTime = 0.0;
    QString outTxt = "count" + QString(separator);
    outTxt += "time" + QString(separator);
    outTxt += "elapsed-time" + QString(separator);
    outTxt += "distance" + QString(separator);
    outTxt += "half-lap-time" + QString(separator);
    outTxt += "speed\n";
    for(unsigned i = 0; i < inData.size(); ++i)
    {
        QDateTime theTime = inTime.addMSecs(inData[i]*1000);
        QString theTimeStr = theTime.toString(timeFormat);
        distance += halfLapDist;
        double halfLapTime = inData[i] - prevTime;
        double speed = halfLapDist/halfLapTime;

        outTxt += QString::number(i+1) + separator;
        outTxt += "\"" + theTimeStr + "\"" + separator;
        outTxt += QString::number(inData[i]) + separator;
        outTxt += QString::number(distance) + separator;
        outTxt += QString::number(halfLapTime) + separator;
        outTxt += QString::number(speed) + separator;
        outTxt += "\n";

        prevTime = inData[i];
    }
    QApplication::restoreOverrideCursor();

    QFileInfo fileInfo(fileName);
    QString csvFileName = fileInfo.absolutePath()+"/"+fileInfo.baseName()+".csv";
    fileName = QFileDialog::getSaveFileName(this,
                                            tr("Save as file"),
                                            csvFileName, tr("Text files (*.txt)"));
    if( fileName.isEmpty() )
        return;

    QFile outFile(csvFileName);
    if (!outFile.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, appName,
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QTextStream out(&outFile);
    out.setCodec("UTF-8");
    out << outTxt;
    QApplication::restoreOverrideCursor();
}

bool MainWindow::saveRow(unsigned row)
{
    if(!startTime[row].isValid())
        return true;

    QDir outputDir(dataPath);
    QString fileName = startTime[unsigned(row)].toString("yyyy-MM-dd_hh-mm-ss");
    fileName.append(QString("_FM%1.txt").arg(row + 1)); // row numbering from 1
    QString filePath = outputDir.filePath(fileName);
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, appName,
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
                    return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    QString outTxt = QString("radius: %1\n").arg(radius);

    outTxt += "time: ";
    outTxt += startTime[unsigned(row)].toString(timeFormat);
    outTxt += "\n";
    // the first value is startSec - start with 1
    for(unsigned i = 1; i < times[row].size(); ++i)
    {
        double elapsed = times[row][i] - startSec[row];
        QString doubleStr = QString::number(elapsed, 'f', 4);
        outTxt += doubleStr + "\n";
    }

    out << outTxt;

    if(out.status() == QTextStream::WriteFailed)
    {
        QMessageBox::warning(this, appName,
                             tr("Writing to file %1 failed:\nThis can be caused by lack of disk space.")
                             .arg(fileName));
        return false;
    }
    else
    {
        table->item(row, save)->setBackground(Qt::green);
        return true;
    }
}

bool MainWindow::saveAll()
{
    for(unsigned i = 0; i < millCount; ++i)
        if(!saveRow(i))
            return false;
    return true;
}

void MainWindow::clearRow(unsigned row)
{
    table->setItem(row, first, new QTableWidgetItem(QString()));
    table->setItem(row, last, new QTableWidgetItem(QString()));
    table->setItem(row, time, new QTableWidgetItem(QString()));
    table->setItem(row, count, new QTableWidgetItem(QString()));
    table->setItem(row, distance, new QTableWidgetItem(QString()));
    startTime[row] = QDateTime();
    lastTime[row] = QDateTime();
    times[row].clear();
    table->item(row, save)->setBackground(Qt::white);
}

void MainWindow::clearAll()
{
    if(!isModified())
        return;
    int ret = QMessageBox::warning(this, appName,
                                 tr("Do you want to discard all data?"),
                                 QMessageBox::Yes,
                                 QMessageBox::Cancel | QMessageBox::Default);

    if(ret == QMessageBox::Cancel)
        return;

    for(unsigned row = 0; row < millCount; ++row)
        clearRow(row);
}

void MainWindow::about()
{
    QString aboutTxt(tr(
                         "<p><b>%1 version %2</b></p>"
                         "<p>Author: Adam Tofilski</p>"
                         "<p>For full functionality, you need specific hardware connected to the Raspberry Pi.</p>"
                         //                         "<p>Home page: <a href=\"http://drawwing.org/dkey\">drawwing.org/dkey</a></p>"
                         //                         "<p>If you find this software useful please cite it:<br />Tofilski A (2018) DKey software for editing and browsing dichotomous keys. ZooKeys 735: 131-140. <a href=\"https://doi.org/10.3897/zookeys.735.21412\">https://doi.org/10.3897/zookeys.735.21412</a></p>"
                         "<p>This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.</p>"
                         "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. </p>"
                         "<p>You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.</p>"
                         ));
    QMessageBox::about(this, appName, aboutTxt.arg(appName).arg(appVersion));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (okToContinue()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::okToContinue()
{
    if (isModified()) {
        int r = QMessageBox::warning(this, appName,
                                     tr("There are unsaved data.\n""Do you want to save changes?"),
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No,
                                     QMessageBox::Cancel | QMessageBox::Escape);
        if (r == QMessageBox::Yes)
        {
            return saveAll();
        } else if (r == QMessageBox::No) {
            return true;
        } else if (r == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

bool MainWindow::isModified()
{
    for(unsigned row = 0; row < millCount; ++row)
    {
        QBrush bgd = table->item(row, save)->background(); //Qt::green
        if(bgd.color() == Qt::yellow)
            return true;
    }
    return false;
}

void MainWindow::preferences()
{
    OFMpreferences * prefDialog = new OFMpreferences(this);
    prefDialog->exec();
    readSettings();
}

void MainWindow::readSettings()
{
    QSettings settings(companyName, appName);
    radius = settings.value("radius", 150).toInt();
    const double PI = 3.141592653589793238463;
    halfLapM = PI * radius * 0.001;
    minTime = settings.value("minTime", 0.1).toDouble();
    inactiveThd = settings.value("inactiveThd", 3600000).toInt(); // 3600000 ms = 1 hour
    separator = settings.value("separator", ',').toChar();
}

void MainWindow::writeSettings()
{
    QSettings settings(companyName, appName);
    settings.setValue("radius", radius);
    settings.setValue("minTime", minTime);
    settings.setValue("inactiveThd", inactiveThd);
    settings.setValue("separator", separator);
}

