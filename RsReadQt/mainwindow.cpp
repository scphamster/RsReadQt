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

    outputTabs = new QTabWidget(OutputDocker);

    rawOutputWidget = new MyWidget(outputTabs);
    rawOutputWidget->setObjectName(QString::fromUtf8("rawOutputWidget"));
    rawOutputWidget->setMouseTracking(true);

    rawOutputTxt = new TextEdit(rawOutputWidget);
    rawOutputTxt->setObjectName(QString::fromUtf8("rawOutputTxt"));

    asciiOutputTxt = new TextEdit(rawOutputWidget);
    asciiOutputTxt->setObjectName(QStringLiteral("asciiOutputTxt"));

    rawOutputWidget->SetTxtEditors(rawOutputTxt, asciiOutputTxt);
    outputTabs->addTab(rawOutputWidget, RAWOUTPUTTABNAME);

    decodedOutputTxt = new QPlainTextEdit();
    outputTabs->addTab(decodedOutputTxt, "Decoded data");

    OutputDocker->setWidget(outputTabs);

    resizeDocks({ OutputDocker }, { static_cast<int>(BOTdOCKdEFhEIGHT * size().height()) }, Qt::Orientation::Vertical);

    // Test

    diagram.chart  = new QChart;
    diagram.chview = new QChartView(diagram.chart);
    setCentralWidget(diagram.chview);

    // End Test
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
            readerThr = nullptr;
        }
    }

    if (writerThr != nullptr) {
        if (writerThr->isRunning()) {
            writerThr->quit();
            writerThr->wait();
            writerThr = nullptr;
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
MainWindow::OnThreadNotificaiton()
{
    if (databridgeData.empty() == true) {
        return;
    }

    auto data_item = std::make_shared<dataPacket>();
    databridgeData.pop_front_wait(data_item);

    QString rawOutput;
    QString asciiOutput;

    rawOutput.append("\n");
    rawOutput.append(" #");
    rawOutput.append(QString::number(data_item->msg_counter));
    rawOutput.append(QTime::currentTime().toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    // TODO: use vector range for loop
    for (int i = 0; (i < data_item->bytes_in_buffer) && (i < sizeof(data_item->data)); i++) {
        rawOutput.append(QString::number((data_item->data[i])));
        rawOutput.append(" ");
    }

    asciiOutput.append(QString("\nMsg number: "));
    asciiOutput.append(QString::number(data_item->msg_counter));
    asciiOutput.append(QString(":\n"));
    asciiOutput.append(QByteArray(reinterpret_cast<char *>(data_item->data._Unchecked_begin()), data_item->bytes_in_buffer));

    rawOutputTxt->appendPlainText(rawOutput);
    asciiOutputTxt->appendPlainText(asciiOutput);

    auto asciiLineCount = asciiOutputTxt->document()->lineCount();
    auto rawLineCount   = rawOutputTxt->document()->lineCount();
    auto lineDiff       = asciiLineCount - rawLineCount;
    if (lineDiff != 0) {
        auto equalizer = QString("");

        if (lineDiff > 1) {
            lineDiff--;   // insertion to TextEdit makes newline itself
            equalizer.append(QString(qAbs(lineDiff), '\n'));
        }

        lineDiff > 0 ? rawOutputTxt->appendPlainText(equalizer) : asciiOutputTxt->appendPlainText(equalizer);
    }
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

#ifndef NOWRITERTHREAD
    writerThr = new OutputThread(databridgeData, rawOutputTxt, asciiOutputTxt, decodedOutputTxt, databridgeConfig, &diagram);
    connect(writerThr, &QThread::finished, writerThr, &QThread::deleteLater, Qt::DirectConnection);
#endif

    readerThr = new ReadingThread(databridgeConfig, databridgeData, this);
    connect(readerThr, &QThread::finished, readerThr, &QThread::deleteLater, Qt::DirectConnection);

#ifndef NOWRITERTHREAD
    connect(readerThr, &ReadingThread::notificationDataArrived, writerThr, &OutputThread::ShowNewData);
    connect(saveSessionAs_menu, &QAction::triggered, writerThr, &OutputThread::SaveSession);
#else
    connect(readerThr, &SerialThread::notificationDataArrived, this, &MainWindow::OnThreadNotificaiton);
#endif

#ifndef NOWRITERTHREAD
    writerThr->start();
#endif

    readerThr->start();

    // TEST
}

void
MainWindow::OnStopSerialClicked()
{
    if (readerThr != nullptr && readerThr->isRunning()) {
        readerThr->quit();
        readerThr->wait();
        readerThr = nullptr;
    }

    if (writerThr != nullptr && writerThr->isRunning()) {
        writerThr->quit();
        writerThr->wait();
        writerThr = nullptr;
    }
}

void
MyWidget::mouseMoveEvent(QMouseEvent *ev)
{
    constexpr auto sizeGripWidth  = 50;
    auto           horSizerPos    = size().width() * lhRhSplitPos;
    auto           cursorPosX     = ev->pos().x();
    bool           readyForResize = false;

    if ((horSizerPos - cursorPosX > 0 && horSizerPos - cursorPosX < sizeGripWidth) ||
        (cursorPosX - horSizerPos > 0 && cursorPosX - horSizerPos < sizeGripWidth)) {
        if (cursor().shape() != Qt::SizeHorCursor) {
            setCursor(Qt::SizeHorCursor);
        }

        auto buttons = ev->buttons();
        if (buttons.testFlag(Qt::MouseButton::LeftButton)) {
            if (size().width() == 0)
                lhRhSplitPos = 0.5;
            else {
                lhRhSplitPos = static_cast<double>(ev->pos().x()) / static_cast<double>(size().width());
                resizeTxtEditors(size());
            }

            return;
        }

        /*if mouse is pressed -> set flag for next mouseMoveEvent to go to :
         ->check if mouse is still pressed
            ->change lhRhSplittPos to ev->pos().x() / size().width()
            ->resizeEvent
         ->ifnot -> unset flag
            */
    }
    else {
        if (cursor().shape() != Qt::ArrowCursor) {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void
MyWidget::ChangeProportions()
{ }
