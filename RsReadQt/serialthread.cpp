#include "serialthread.h"
#include <qtextedit.h>
#include <QTime>
#include <QTextBlock>

SerialThread::SerialThread(std::shared_ptr<void> databidgeData, deque_s<std::shared_ptr<::dataPacket>> &data, QObject *parent)
  : serDevice(std::static_pointer_cast<SerialConfigs>(databidgeData), data)
  , QThread(parent)
{ }

SerialThread::~SerialThread()
{
    serDevice.Stop();
}

void
SerialThread::run()
{
    assert(serDevice.Open() == 1);

    while (1) {
        if (serDevice.Read()) {
            emit notificationDataArrived();
        }
        // emit notificationDataArrived();
        // msleep(200);
    }
}

TxtOutputThread::TxtOutputThread(deque_s<std::shared_ptr<::dataPacket>> &data, QPlainTextEdit *lhTxt, QPlainTextEdit *rhTxt)
  : dataToOutput{ data }
  , rawOutputTxt{ lhTxt }
  , asciiOutputTxt{ rhTxt }
{ }

void
TxtOutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data_item = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data_item);

    QString rawOutput;
    QString asciiOutput;

    rawOutput.append("\n");
    rawOutput.append(" #");
    rawOutput.append(QString::number(data_item->msg_counter));
    rawOutput.append(QTime::currentTime().toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    //TODO: use vector range for loop
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
