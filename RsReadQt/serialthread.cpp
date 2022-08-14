#include "serialthread.h"

#include <algorithm>

#include <QMainWindow>

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
#include <qvalueaxis.h>
#include <QLineSeries>
#include <QPixMap>
#include <QPainter>
#include <QList>

#include <QAbstractAxis>
#include <QValueAxis>
#include <QXYSeries>
#include <QScatterSeries>
#include <QSplineSeries>

//#include <windows.h>

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
                           QChart                                 *ch,
                           QChartView                             *chvw,
                           QMainWindow                            *parent)
  : dataToOutput{ data }
  , tabWgt{ tabs }
  , myParent(parent)
{
    diagram.chart     = ch;
    diagram.chview    = chvw;
    decodeConfigsFile = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName();

    assert(decodeConfigsFile != QString{});

    GetDecodeConfigsFromFile();

    diagram.startOfOperation = QDateTime::currentDateTime();

    // textOutput = new QPlainTextEdit{ tabWgt };
    // tabWgt->addTab(textOutput, "Raw");

    outputList = new QListWidget{ tabWgt };
    outputList->setUniformItemSizes(true);
    tabWgt->addTab(outputList, "raw");

    diagram.yaxis = new QValueAxis{ diagram.chart };
    diagram.xaxis = new QValueAxis{ diagram.chart };

    diagram.chart->addAxis(diagram.yaxis, Qt::AlignLeft);
    diagram.chart->addAxis(diagram.xaxis, Qt::AlignBottom);

    auto valuesRange = 400;

    diagram.xaxis->setRange(0, 10);
    diagram.yaxis->setRange(0, valuesRange);
    diagram.yaxis->setTickInterval(10);

    connect(&diagram, &ArincLabelsChart::MsgOnChartBeenSelected, this, &OutputThread::ScrollAndSelectMsg);
}

OutputThread::~OutputThread()
{
    diagram.labelsSeries.clear();
}

void
OutputThread::AddLabelToDiagram(int labelIdx)
{
    if (diagram.labelsSeries.contains(labelIdx))
        return;

    diagram.labelsSeries[labelIdx].first = new QLineSeries{ diagram.chart };

    auto &series = *diagram.labelsSeries[labelIdx].first;
    series.setParent(this);
    auto labelMarker = new QImage(80, 80, QImage::Format::Format_ARGB32);
    labelMarker->fill(QColor(0, 0, 0, 0));

    QPainter painter(labelMarker);

    painter.setBrush(QBrush(QColor(rand() % 255, rand() % 255, rand() % 255, 255)));
    painter.setBackground(QBrush(QColor(255, 255, 255, 0)));
    painter.drawRect(QRect(0, 0, 20, 40));

    painter.rotate(-90);
    painter.setBrush(QBrush(QColor(255, 255, 255, 255)));
    painter.drawText(QPoint(-40, 10), QString::number(labelIdx));
    // painter.rotate(80);

    series.setMarkerSize(70);
    series.setLightMarker(*labelMarker);

    series.setPen(QPen(QColor(0, 0, 0, 0)));

    diagram.chart->addSeries(diagram.labelsSeries[labelIdx].first);

    series.attachAxis(diagram.yaxis);
    series.attachAxis(diagram.xaxis);

    QObject::connect(&series, &QXYSeries::released, &diagram, &ArincLabelsChart::OnLabelOnChartSelected);
}

void
OutputThread::ShowDiagram()
{
    const auto &data             = messages.back();
    qreal       secondsFromStart = (-1) * static_cast<qreal>(data.timeArrivalPC.msecsTo(diagram.startOfOperation)) / 1000;

    if (!diagram.labelsSeries.contains(data.labelRaw))
        AddLabelToDiagram(data.labelRaw);

    diagram.labelsSeries[data.labelRaw].first->append(QPointF(secondsFromStart, data.labelRaw));
    diagram.labelsSeries[data.labelRaw].second.push_back(std::pair<qreal, const ArincMsg &>(secondsFromStart, data));

    if (diagram.yaxis->max() < data.labelRaw) {
        diagram.yaxis->setMax(data.labelRaw + 50);
    }

    if (diagram.xaxis->max() < secondsFromStart) {
        diagram.xaxis->setMax(secondsFromStart + 5);
        diagram.xaxis->setMin(secondsFromStart - 10);
    }

    // Beep(data.labelRaw * 4, 50);
}

void
OutputThread::NormalizeRawData(const auto &data, QString &appendHere)
{
    NormalizeAndStoreMsg(data);

    const auto &decoded = messages.back();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded.channel)
                        .arg(decoded.labelRaw)
                        .arg(decoded.SDI)
                        .arg(decoded.valueRaw)
                        .arg(decoded.SSM)
                        .arg(decoded.parity)
                        .arg(decoded.DTtimeRaw));

    lastMsgReadedNum = decoded.msgNumber;
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
    // TODO: use vector range for loop
    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);
    outputList->addItem(rawOutput);

    if (diagram.selectedItem == ArincLabelsChart::ItemSelection::NOTSELECTED) {
        outputList->scrollToBottom();
    }
    else {
        
    }

    ShowDiagram();
}

void
OutputThread::ScrollAndSelectMsg(uint64_t item)
{
    outputList->scrollToItem(outputList->item(diagram.selectedItem));
    outputList->setCurrentRow((diagram.selectedItem));
}

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

void
Arinc::GetDecodeConfigsFromFile()
{
    auto jsonfile = QFile{ decodeConfigsFile };
    jsonfile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonfile.readAll();
    jsonfile.close();

    DTMsgAnatomy.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    if (err.error != QJsonParseError::NoError) {
        QMessageBox errorMsg{ QMessageBox::Warning,
                              "Json file Error",
                              "Error occured during json file \" " + decodeConfigsFile +
                                "\" opening, Error: " + err.errorString() };

        assert(0);
        exit(-1);
    }

    auto mainObj   = jsondoc.object();
    auto configObj = mainObj["Configurations"].toObject();

    auto msgChunkNames         = configObj["Message parts names"].toArray();
    auto msgChunkNumber        = configObj["Message parts number"].toInt();
    auto msgLenBytes           = configObj["Message length bytes"].toInt();
    auto msgLenBits_calculated = msgLenBytes * 8 - 1;

    auto chunks = configObj["Parts"].toObject();

    DTMsgAnatomy.clear();

    for (auto chunkName : msgChunkNames) {
        auto configs = chunks[chunkName.toString()].toObject();

        DTWordField chunk;
        chunk.activeBits.first  = configs["Bit start"].toInt();
        chunk.activeBits.second = configs["Bit end"].toInt();

        if (chunk.activeBits.first > chunk.activeBits.second) {
            QMessageBox errorMsg{
                QMessageBox::Critical,
                "Json file Error",
                "Message part with name " + chunkName.toString() + "has starting bit number higher than ending bit number"

            };

            anatomyIsConfigured = false;
            return;
        }

        if (chunk.activeBits.second > msgLenBits_calculated || chunk.activeBits.first > msgLenBits_calculated) {
            QMessageBox errorMsg{
                QMessageBox::Critical,
                "Json file Error",
                "Message part with name: " + chunkName.toString() +
                  " has active bits number exceeding number of bits calculated from \"Mesaage length bytes\""
            };

            anatomyIsConfigured = false;
            return;
        }

        chunk.bitOrder =
          configs["Reverse bits?"].toBool() ? DTWordField::BitOrder::REVERSE : DTWordField::BitOrder::NORMAL;
        chunk.byteOrder =
          configs["Reverse bytes?"].toBool() ? DTWordField::ByteOrder::REVERSE : DTWordField::ByteOrder::NORMAL;

        auto format = configs["Encoding"].toString();

        if (format == "BIN")
            chunk.dataFormat = DTWordField::DataFormat::BIN;
        else if (format == "DEC")
            chunk.dataFormat = DTWordField::DataFormat::DEC;
        else if (format == "OCT")
            chunk.dataFormat = DTWordField::DataFormat::OCT;
        else if (format == "HEX")
            chunk.dataFormat = DTWordField::DataFormat::HEX;
        else if (format == "BCD")
            chunk.dataFormat = DTWordField::DataFormat::BCD;
        else {
            chunk.dataFormat = DTWordField::DataFormat::UNDEFINED;

            QMessageBox error{ QMessageBox::Critical,
                               "Json file Error",
                               "Message part with name: " + chunkName.toString() +
                                 " has undefined encoding. Valid types are: BIN, DEC, OCT, HEX, BCD" };
        }

        DTMsgAnatomy[chunkName.toString()] = std::move(chunk);
    }

    anatomyIsConfigured = true;
}

void
Arinc::NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTWordField &configs, auto &container)
{
    auto firstByteNum = configs.activeBits.first / 8;
    auto lastByteNum  = configs.activeBits.second / 8;
    int  incrementor;
    int  byteNumber = 0;

    if (configs.byteOrder == DTWordField::ByteOrder::NORMAL) {
        incrementor = 1;
    }
    else {
        incrementor = -1;

        auto temp    = firstByteNum;
        firstByteNum = lastByteNum;
        lastByteNum  = temp;
    }

    container = 0;
    for (int i = firstByteNum; true; i += incrementor) {
        auto byte    = static_cast<uint8_t>(data->data.at(i));
        auto byteNum = i - firstByteNum;

        if (configs.bitOrder == DTWordField::BitOrder::REVERSE) {
            byte = DTWordField::reverseBitsInByte(byte);
        }

        if (byteNumber > 7)
            while (1) { }

        container |= byte << (byteNumber * 8);

        byteNumber++;

        if (i == lastByteNum)
            break;
    }

    auto     activeBitsNum = configs.activeBits.second - configs.activeBits.first + 1;
    uint64_t mask          = (static_cast<uint64_t>(1 << activeBitsNum)) - 1;

    if (configs.byteOrder == DTWordField::ByteOrder::NORMAL) {
        auto firstByteFirstBitNum = configs.activeBits.first % 8;

        container >>= firstByteFirstBitNum;
        container &= mask;
    }
    else {
        uint64_t allButFirst = container & 0xFFffFFff00UL;

        auto lastByteUnusedBitsNum = configs.activeBits.first % 8;
        auto bytesNum              = firstByteNum - lastByteNum;

        uint64_t lastByte = container & (0xFF << (bytesNum * 8));
        lastByte >>= lastByteUnusedBitsNum;

        container &= ~(0xFF << (bytesNum * 8));
        container |= lastByte;

        auto firstByteUnusedBitsNum = 7 - configs.activeBits.second % 8;
        container >>= firstByteUnusedBitsNum;

        container &= mask;
    }
}

void
Arinc::NormalizeAndStoreMsg(std::shared_ptr<dataPacket> rawData)
{
    ArincMsg msg;
    if (decodeConfigsFile != QString{}) {
        msg.timeArrivalPC = rawData->msg_arrival_time;
        msg.msgNumber     = rawData->msg_counter;

        for (auto &msgChunk : DTMsgAnatomy) {
            if (msgChunk.second.activeBits.first / 8 >= rawData->bytes_in_buffer ||
                msgChunk.second.activeBits.second / 8 >= rawData->bytes_in_buffer) {
                QMessageBox err{ QMessageBox::Critical, "Data read error", "Not enough bytes in buffer" };
            }
        }

        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Channel"], msg.channel);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Label"], msg.labelRaw);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["SDI"], msg.SDI);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Data"], msg.valueRaw);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["SSM"], msg.SSM);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Parity"], msg.parity);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Time"], msg.DTtimeRaw);
    }

    messages.push_back(msg);
}

void
ArincLabelsChart::OnLabelOnChartSelected(const QPointF &point)
{
    GetDataFromLabelOnChart(point);
}

bool
ArincLabelsChart::GetDataFromLabelOnChart(const QPointF &atPoint)
{
    int label = static_cast<int>(atPoint.y());

    const auto &dataVector = labelsSeries.at(label).second;

    auto finder = [&atPoint](const auto &pointAndData) {
        if (pointAndData.first == atPoint.x()) {
            return true;
        }
        else {
            return false;
        }
    };

    auto finding = std::find_if(dataVector.rbegin(), dataVector.rend(), finder);

    if (finding != dataVector.rend()) {
        selectedItem = (finding->second.msgNumber);
        
        emit MsgOnChartBeenSelected(selectedItem);

        return true;
    }
    return false;
}