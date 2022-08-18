#include <algorithm>


#include <QMainWindow>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>
#include <QTime>
#include <qtextedit.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <QTextBlock>
#include <QMessageBox>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QXYSeries>
#include <QLineSeries>
#include <QScatterSeries>
#include <QSplineSeries>
#include <QAbstractAxis>
#include <QValueAxis>
#include <QPixMap>
#include <QPainter>
#include <QList>

#include "serialthread.h"
#include "arinc.hpp"

ReadingThread::ReadingThread(std::shared_ptr<void>                   databidgeData,
                             deque_s<std::shared_ptr<::dataPacket>> &data,
                             QObject                                *parent)
  : serDevice(std::static_pointer_cast<SerialConfigs>(databidgeData), data)
  , QThread(parent)
{ }

void
ReadingThread::quit()
{
    serDevice.Stop();
    QThread::quit();
}

void
ReadingThread::Pause(bool makePause)
{
    isPaused = makePause;

    if (!makePause)
        serDevice.Flush();
}

void
ReadingThread::run()
{
    // TODO: add error checking to serial
    serDevice.Open();

    while (serDevice.IsOpen()) {
        if (isPaused)
            continue;

        if (serDevice.Read()) {
            emit notificationDataArrived();
        }
    }
}

OutputThread::OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                           std::shared_ptr<void>                   dataBridgeConfigs,
                           QTabWidget                             *tabs,
                           QMainWindow                            *parent)
  : dataToOutput{ data }
  , tabWgt{ tabs }
  , myParent(parent)
{
    arinc = std::make_unique<Arinc>(std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName());
    arincChart = new ArincLabelsChart{ parent };

    rawMessages = new QListWidget{ tabWgt };
    rawMessages->setUniformItemSizes(true); //for better performance
    tabWgt->addTab(rawMessages, "raw");
    
    connect(arincChart, &ArincLabelsChart::MsgOnChartBeenSelected, this, &OutputThread::ScrollAndSelectMsg);
}

OutputThread::~OutputThread()
{
}


void
OutputThread::ShowDiagram()
{
    const auto &data             = arinc->messages.back();
    arincChart->Append(data);
    // Beep(data.labelRaw * 4, 50);
}

void
OutputThread::NormalizeRawData(const auto &data, QString &appendHere)
{
    arinc->NormalizeAndStoreMsg(data);

    const auto &decoded = arinc->messages.back();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded.channel)
                        .arg(decoded.labelRaw)
                        .arg(decoded.SDI)
                        .arg(decoded.valueRaw)
                        .arg(decoded.SSM)
                        .arg(decoded.parity)
                        .arg(decoded.DTtimeRaw));

    arinc->lastMsgReadedNum = decoded.msgNumber;
}

void
OutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data);

    QString rawOutput;

    rawOutput.append(QString{ "#%1  " }.arg(data->msg_counter));
    rawOutput.append(data->msg_arrival_time.toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    
    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);
    rawMessages->addItem(rawOutput);

    if (!arincChart->IsSomeLabelSelected()) {
        rawMessages->scrollToBottom();
    }

    ShowDiagram();
}

void
OutputThread::ScrollAndSelectMsg(uint64_t item)
{
    rawMessages->scrollToItem(rawMessages->item(arincChart->GetIdxOfSelectedMessage()));
    rawMessages->setCurrentRow((arincChart->GetIdxOfSelectedMessage()));
}

void
OutputThread::testSlot(int)
{ }

bool
OutputThread::SaveSession()
{
    auto saveto_fileAddress = QFileDialog::getSaveFileName(nullptr, tr("Save session to file"), "LastSession.txt");

    QString messagesText;

    // test
    // messagesText.append(textOutput->toPlainText());
    QFile lastSessionFile(saveto_fileAddress);

    lastSessionFile.open(QIODeviceBase::OpenModeFlag::Append);
    lastSessionFile.write(messagesText.toLocal8Bit(), messagesText.length());
    lastSessionFile.close();

    return true;
}