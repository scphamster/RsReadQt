#include "mainwindow.h"
#include "serialconfig.h"
#include <qmessagebox.h>
#include <qtextedit.h>
#include <qtimeline.h>
#include <QTime>
#include <qevent.h>
#include <qstyle.h>
#include <qcoreapplication.h>
//#define NOWRITERTHREAD

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
    setupUi(this);

    QCoreApplication::setApplicationName(APPNAME);
    QCoreApplication::setOrganizationName(ORGANIZATION);

    outputWidget = new MyWidget(OutputDocker);
    outputWidget->setObjectName(QString::fromUtf8("outputWidget"));
    outputWidget->setMouseTracking(true);

    rawOutputTxt = new TextEdit(outputWidget);
    rawOutputTxt->setObjectName(QString::fromUtf8("rawOutputTxt"));

    asciiOutputTxt = new TextEdit(outputWidget);
    asciiOutputTxt->setObjectName(QStringLiteral("asciiOutputTxt"));

    outputWidget->SetTxtEditors(rawOutputTxt, asciiOutputTxt);
    OutputDocker->setWidget(outputWidget);
    
    resizeDocks({ OutputDocker }, { static_cast<int>(BOTdOCKdEFhEIGHT * size().height()) }, Qt::Orientation::Vertical);

    connect(actionConfigure_serial, &QAction::triggered, this, &MainWindow::OnSerialConfigureClicked);
    connect(actionStart_receiver, &QAction::triggered, this, &MainWindow::OnStartSerialClicked);
    connect(actionStop_receiver, &QAction::triggered, this, &MainWindow::OnStopSerialClicked);
}

MainWindow::~MainWindow()
{
    if (writerThr != nullptr) {
        writerThr->quit();
        writerThr->wait();
    }
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
    asciiOutput.append(QByteArray(data_item->data._Unchecked_begin(), data_item->bytes_in_buffer));

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
    writerThr = new TxtOutputThread(databridgeData, rawOutputTxt, asciiOutputTxt);
    connect(writerThr, &QThread::finished, writerThr, &QThread::deleteLater);
#endif

    readerThr = new SerialThread(databridgeConfig, databridgeData, nullptr);
    connect(readerThr, &SerialThread::finished, readerThr, &SerialThread::deleteLater);

#ifndef NOWRITERTHREAD
    connect(readerThr, &SerialThread::notificationDataArrived, writerThr, &TxtOutputThread::ShowNewData);
#else
    connect(readerThr, &SerialThread::notificationDataArrived, this, &MainWindow::OnThreadNotificaiton);
#endif

#ifndef NOWRITERTHREAD
    writerThr->start();
#endif

    readerThr->start();

    OutputDocker->setGeometry(0,
                              static_cast<int>(static_cast<double>(size().height()) * 0.65),
                              size().width(),
                              static_cast<int>(static_cast<double>(size().height()) * 0.5));
    OutputDocker->updateGeometry();
}

void
MainWindow::OnStopSerialClicked()
{
    assert(readerThr != nullptr);
    if (readerThr->isRunning()) {
        readerThr->quit();
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