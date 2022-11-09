#include "mainwindow.h"

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

#include "reading_thread.hpp"
#include "arinc_data_display.h"

#include "serialconfig.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
    setupUi(this);

    QCoreApplication::setApplicationName(APPNAME);
    QCoreApplication::setOrganizationName(ORGANIZATION);

    outputTabs = new QTabWidget{ OutputDocker };
    OutputDocker->setWidget(outputTabs);

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

    if (readerThr != nullptr && arincDisplay != nullptr) {
        if (readerThr->isRunning()) {
            readerThr->Pause(false);
            return;
        }
    }

    arincDisplay = new ArincDataDisplay(databridgeData, databridgeConfig, outputTabs, this);
    //connect(arincDisplay, &QThread::finished, arincDisplay, &QThread::deleteLater);

    readerThr = new ReadingThread(databridgeConfig, databridgeData, this);
    connect(readerThr, &QThread::finished, readerThr, &QThread::deleteLater);

    connect(readerThr, &ReadingThread::notificationDataArrived, arincDisplay, &ArincDataDisplay::ShowNewData);
    connect(saveSessionAs_menu, &QAction::triggered, arincDisplay, &ArincDataDisplay::SaveSession);

    //arincDisplay->start();
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
