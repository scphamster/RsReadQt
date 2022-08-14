#include "mainwindow.h"
#include "serialconfig.h"
#include <qmessagebox.h>
#include <qtextedit.h>
#include <qtimeline.h>
#include <QTime>
#include <qevent.h>
#include <qstyle.h>
#include <qcoreapplication.h>
#include <qjsondocument.h>
#include <qlayout.h>
#include <QtWidgets/qhboxlayout>

//#define NOWRITERTHREAD

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
    setupUi(this);

    QCoreApplication::setApplicationName(APPNAME);
    QCoreApplication::setOrganizationName(ORGANIZATION);

    outputTabs = new QTabWidget{ OutputDocker };
    OutputDocker->setWidget(outputTabs);

    chart  = new QChart{};
    chview = new QChartView{ chart, this };
    chart->setParent(chview);
    chart->setTitle("Labels");

    setCentralWidget(chview);

    resizeDocks({ OutputDocker }, { static_cast<int>(BOTdOCKdEFhEIGHT * size().height()) }, Qt::Orientation::Vertical);

    connect(actionConfigure_serial, &QAction::triggered, this, &MainWindow::OnSerialConfigureClicked);
    connect(actionStart_receiver, &QAction::triggered, this, &MainWindow::OnStartSerialClicked);
    connect(actionStop_receiver, &QAction::triggered, this, &MainWindow::OnStopSerialClicked);
}

MainWindow::~MainWindow()
{
    if (readerThr != nullptr) {
        if (readerThr->isRunning()) {
            readerThr->quit();
            readerThr->wait();
            // readerThr = nullptr;
        }
    }

    if (writerThr != nullptr) {
        if (writerThr->isRunning()) {
            writerThr->quit();
            writerThr->wait();
            // writerThr = nullptr;
        }
    }

    databridgeData.clear();
}

void
MainWindow::OnSerialConfigureClicked()
{
    auto dialog = SerialConfig{ databridgeConfig, this };
    dialog.exec();
}

void
MainWindow::OnStartSerialClicked()
{
    if (databridgeConfig.get() == nullptr) {
        QMessageBox noconfigsMsg{ QMessageBox::Warning,
                                  _T("Warning"),
                                  _T("No configurations provided for serial port. Configure now?") };
        noconfigsMsg.addButton(QMessageBox::StandardButton::Yes);
        noconfigsMsg.addButton(QMessageBox::StandardButton::No);

        auto decision = noconfigsMsg.exec();

        if (decision == QMessageBox::StandardButton::Yes) {
            OnSerialConfigureClicked();
            return;
        }
        else {
            return;
        }
    }

    if (readerThr != nullptr && writerThr != nullptr) {
        if (readerThr->isRunning() && writerThr->isRunning()) {
            readerThr->Pause(false);
            return;
        }
    }

    writerThr = new OutputThread(databridgeData, databridgeConfig, outputTabs, chart, chview, this);
    connect(writerThr, &QThread::finished, writerThr, &QThread::deleteLater, Qt::DirectConnection);

    readerThr = new ReadingThread(databridgeConfig, databridgeData, this);
    connect(readerThr, &QThread::finished, readerThr, &QThread::deleteLater, Qt::DirectConnection);

    connect(readerThr, &ReadingThread::notificationDataArrived, writerThr, &OutputThread::ShowNewData);
    connect(saveSessionAs_menu, &QAction::triggered, writerThr, &OutputThread::SaveSession);

    writerThr->start();
    readerThr->start();
}

void
MainWindow::OnStopSerialClicked()
{
    if (readerThr != nullptr) {
        if (readerThr->isRunning()) {
            readerThr->Pause(true);
        }
    }
}
