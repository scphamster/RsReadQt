#include "serialthread.h"

#include <QTime>
#include <QTextBlock>
#include <qtextedit.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <QFile>
#include <QFileDialog>
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

#include <windows.h>

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
ReadingThread::run()
{
    assert(serDevice.Open() == 1);

    while (serDevice.IsOpen()) {
        if (serDevice.Read()) {
            emit notificationDataArrived();
        }
    }
}

uint8_t
DTMsgElement::reverseBits(uint8_t data)
{
    data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
    data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
    data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
    return data;
}

OutputThread::OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                           QPlainTextEdit                         *lhTxt,
                           QPlainTextEdit                         *rhTxt,
                           QPlainTextEdit                         *decodedTxt,
                           std::shared_ptr<void>                   dataBridgeConfigs,
                           DataChart                              *chart)
  : dataToOutput{ data }
  , rawOutputTxt{ lhTxt }
  , asciiOutputTxt{ rhTxt }
  , decodedOutputTxt(decodedTxt)
  , diagram(chart)
{
    decodeConfigsFile  = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName();
    decodeConfigsFile2 = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName2();

    assert(decodeConfigsFile != QString{});

    if (decodeConfigsFile2 != QString{}) {
        GetDecodeConfigsFromFile2();
    }
    else {
        GetDecodeConfigsFromFile();
    }

    if (diagram->isInitialized)
        return;

    diagram->chart->setTitle("Labels");

    diagram->yaxis = new QValueAxis;
    diagram->xaxis = new QValueAxis;

    diagram->chart->addAxis(diagram->yaxis, Qt::AlignLeft);
    diagram->chart->addAxis(diagram->xaxis, Qt::AlignBottom);

    auto valuesRange = 400;

    diagram->yaxis->setRange(0, valuesRange);
    diagram->xaxis->setRange(0, 10);

    diagram->yaxis->setTickInterval(10);
    diagram->isInitialized = true;

    diagram->startOfOperation = QDateTime::currentDateTime();
}

OutputThread::~OutputThread() { }

void
OutputThread::AddLabelToDiagram(int labelIdx)
{
    if (diagram->labelsSeries.contains(labelIdx))
        return;

    auto &series      = diagram->labelsSeries[labelIdx];
    auto  labelMarker = new QImage(80, 80, QImage::Format::Format_ARGB32);
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

    // series->setBrush(QBrush(*labelMarker));

    diagram->chart->addSeries(&diagram->labelsSeries[labelIdx]);

    series.attachAxis(diagram->yaxis);
    series.attachAxis(diagram->xaxis);
}

void
OutputThread::ShowDiagram()
{
    auto  data = messages.back();
    qreal secondsFromStart =
      (-1) * static_cast<qreal>(QDateTime::currentDateTime().msecsTo(diagram->startOfOperation)) / 1000;

    if (!diagram->labelsSeries.contains(data.labelRaw))
        AddLabelToDiagram(data.labelRaw);

    diagram->labelsSeries[data.labelRaw].append(QPointF(secondsFromStart, data.labelRaw));

    if (diagram->yaxis->max() < data.labelRaw) {
        diagram->yaxis->setMax(data.labelRaw + 50);
    }

    if (diagram->xaxis->max() < secondsFromStart) {
        diagram->xaxis->setMax(secondsFromStart + 5);
        diagram->xaxis->setMin(secondsFromStart - 10);
    }

    // Beep(data.labelRaw * 4, 50);
}

void
OutputThread::ShowNormalizedRawData(auto data)
{
    NormalizeAndStoreMsg(data);
    QString decodedOut;
    auto    decoded = messages.back();

    decodedOut.append("Channel ");
    decodedOut.append(QString::number(decoded.channel));

    decodedOut.append(" Label ");
    decodedOut.append(QString::number(decoded.labelRaw));

    decodedOut.append(" SDI: ");
    decodedOut.append(QString::number(decoded.SDI));

    decodedOut.append(" Data ");
    decodedOut.append(QString::number(decoded.valueRaw));

    decodedOut.append(" SSM: ");
    decodedOut.append(QString::number(decoded.SSM));

    decodedOut.append(" Parity: ");
    decodedOut.append(QString::number(decoded.parity));

    decodedOut.append(" Time ");
    decodedOut.append(QString::number(decoded.timeRaw));

    decodedOutputTxt->appendPlainText(decodedOut);
}

void
OutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data);

    ShowNormalizedRawData(data);
    ShowDiagram();

    QString rawOutput;
    QString asciiOutput;

    rawOutput.append("\n");
    rawOutput.append(" #");
    rawOutput.append(QString::number(data->msg_counter));
    rawOutput.append(QTime::currentTime().toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    // TODO: use vector range for loop
    for (int i = 0; (i < data->bytes_in_buffer) && (i < sizeof(data->data)); i++) {
        rawOutput.append(QString::number((data->data[i])));
        rawOutput.append(" ");
    }

    asciiOutput.append(QString("\nMsg number: "));
    asciiOutput.append(QString::number(data->msg_counter));
    asciiOutput.append(QString(":\n"));
    asciiOutput.append(QByteArray(data->data._Unchecked_begin(), data->bytes_in_buffer));

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

bool
OutputThread::SaveSession()
{
    auto saveto_fileAddress = QFileDialog::getSaveFileName(nullptr, tr("Save session to file"), "LastSession.txt");

    QString messagesText;

    messagesText.append(decodedOutputTxt->toPlainText());
    QFile lastSessionFile(saveto_fileAddress);

    lastSessionFile.open(QIODeviceBase::OpenModeFlag::Append);
    lastSessionFile.write(messagesText.toLocal8Bit(), messagesText.length());
    lastSessionFile.close();

    return true;
}

void
DecodedData::GetDecodeConfigsFromFile()
{
    auto jsonFile = QFile(decodeConfigsFile);
    jsonFile.setPermissions(QFileDevice::Permission::ReadGroup);
    jsonFile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonFile.readAll();
    jsonFile.close();

    DTMessageStructure.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    auto obj           = jsondoc.object();
    auto dataStructure = obj["Data structure"].toObject();

    auto bytesInMsg    = dataStructure.value("Message length");
    auto msgElemsNames = dataStructure.value("Message parts").toArray();

    for (auto name : msgElemsNames) {
        DTMsgElement elem;
        auto         elemConfigs = dataStructure.value(name.toString()).toObject();

        elem.length = elemConfigs.value("Length").toInt();

        if (elem.length > 1) {
            if (elemConfigs.value("Byte order").toString() == "MSB") {
                elem.byteOrder = DTMsgElement::ByteOrder::MSB;
            }
            else {
                elem.byteOrder = DTMsgElement::ByteOrder::LSB;
            }
        }

        if (elemConfigs.value("Bit order").toString() == "MSB") {
            elem.bitOrder = DTMsgElement::BitOrder::MSB;
        }
        else {
            elem.bitOrder = DTMsgElement::BitOrder::LSB;
        }

        auto activeBits        = elemConfigs.value("Active bits").toArray();
        elem.activeBits.first  = activeBits.at(0).toInt();
        elem.activeBits.second = activeBits.at(1).toInt();

        elem.startbyte = elemConfigs.value("Byte index").toInt();

        if (elemConfigs.value("Format").toString() == "BIN")
            elem.dataFormat = DTMsgElement::DataFormat::BIN;
        else if (elemConfigs.value("Format").toString() == "DEC")
            elem.dataFormat = DTMsgElement::DataFormat::DEC;
        else if (elemConfigs.value("Format").toString() == "OCT")
            elem.dataFormat = DTMsgElement::DataFormat::OCT;
        else if (elemConfigs.value("Format").toString() == "HEX")
            elem.dataFormat = DTMsgElement::DataFormat::HEX;
        else if (elemConfigs.value("Format").toString() == "BCD")
            elem.dataFormat = DTMsgElement::DataFormat::BCD;

        auto elemName                = name.toString();
        DTMessageStructure[elemName] = std::move(elem);
    }
}

void
DecodedData::GetDecodeConfigsFromFile2()
{
    auto jsonfile = QFile{ decodeConfigsFile2 };
    jsonfile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonfile.readAll();
    jsonfile.close();

    DTMsgAnatomy.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    if (err.error != QJsonParseError::NoError) {
        QMessageBox errorMsg{ QMessageBox::Warning,
                              "Json file Error",
                              "Error occured during json file \" " + decodeConfigsFile2 +
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

        DTMsgElement2 chunk;
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

        chunk.bitOrder = configs["Reverse?"].toBool() ? DTMsgElement2::BitOrder::REVERSE : DTMsgElement2::BitOrder::NORMAL;
        auto format    = configs["Encoding"].toString();

        if (format == "BIN")
            chunk.dataFormat = DTMsgElement2::DataFormat::BIN;
        else if (format == "DEC")
            chunk.dataFormat = DTMsgElement2::DataFormat::DEC;
        else if (format == "OCT")
            chunk.dataFormat = DTMsgElement2::DataFormat::OCT;
        else if (format == "HEX")
            chunk.dataFormat = DTMsgElement2::DataFormat::HEX;
        else if (format == "BCD")
            chunk.dataFormat = DTMsgElement2::DataFormat::BCD;
        else {
            chunk.dataFormat = DTMsgElement2::DataFormat::UNDEFINED;

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
DecodedData::NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTMsgElement &elemStructure, auto &container)
{
    assert(elemStructure.length > 0);
    assert(elemStructure.activeBits.first < elemStructure.activeBits.second);
    assert(elemStructure.startbyte >= 0 && elemStructure.startbyte < data->bytes_in_buffer);

    container = 0;

    auto reordreBitsBytes = [&elemStructure, &container](auto &iterator) {
        for (int i = 0; i < elemStructure.length; i++) {
            uint8_t byte;

            if (elemStructure.bitOrder == DTMsgElement::BitOrder::MSB) {
                byte = DTMsgElement::reverseBits(*iterator);
            }
            else
                byte = *iterator;

            container |= byte << (i * 8);
            iterator++;
        }
    };

    if (elemStructure.byteOrder == DTMsgElement::ByteOrder::MSB && elemStructure.length > 1) {
        auto iterator =
          std::make_reverse_iterator(data->data.begin() + elemStructure.startbyte + (elemStructure.length - 1)) - 1;
        reordreBitsBytes(iterator);
    }
    else {
        auto iterator = data->data.begin() + elemStructure.startbyte;
        reordreBitsBytes(iterator);
    }
}

// void
// DecodedData::NormalizeAndStoreMsgItem2(std::shared_ptr<dataPacket> data, DTMsgElement2 &configs, auto &container)
//{
//     auto firstByteNum = configs.activeBits.first / 8;
//     auto lastByteNum  = configs.activeBits.second / 8;
//
//     container                   = 0;
//     auto alignBitsNum           = configs.activeBits.first % 8;
//     auto firstByteFirstBitNum   = configs.activeBits.first % 8;
//     auto firstByteActiveBitsNum = 8 - firstByteFirstBitNum;
//     auto lastByteLastBitNum     = configs.activeBits.second % 8;
//
//     for (int i = firstByteNum; i <= lastByteNum; i++) {
//         uint8_t byte = data->data.at(i);
//
//         if (i == firstByteNum) {
//             byte &= ((1 << firstByteActiveBitsNum) - 1) << firstByteFirstBitNum;
//         }
//         else if (i == lastByteNum) {
//             byte &= (1 << (lastByteLastBitNum + 1)) - 1;
//         }
//
//         if (configs.bitOrder == DTMsgElement2::BitOrder::REVERSE) {
//             byte = DTMsgElement2::convertToReverseBits(byte);
//
//             if (i == firstByteNum) {
//                 byte = byte << firstByteFirstBitNum;
//             }
//             else if (i == lastByteNum) {
//                 byte = byte >> (7 - lastByteLastBitNum);
//             }
//         }
//
//         auto leftShiftBitsNum = (i - firstByteNum) * 8 - firstByteFirstBitNum;
//
//         if (i == firstByteNum) {
//             container |= byte >> firstByteFirstBitNum;
//         }
//         else {
//             container |= byte << leftShiftBitsNum;
//         }
//     }
// }

void
DecodedData::NormalizeAndStoreMsgItem2(std::shared_ptr<dataPacket> data, DTMsgElement2 &configs, auto &container)
{
    auto firstByteNum = configs.activeBits.first / 8;
    auto lastByteNum  = configs.activeBits.second / 8;

    //auto byteCounter = (firstByteNum < lastByteNum) ? 0 : lastByteNum;
    //auto byteCounterIncr = (firstByteNum < lastByteNum) ? 1 : lastByteNum;

    container                   = 0;
    auto alignBitsNum           = configs.activeBits.first % 8;
    auto firstByteFirstBitNum   = configs.activeBits.first % 8;
    auto firstByteActiveBitsNum = 8 - firstByteFirstBitNum;
    auto lastByteLastBitNum     = configs.activeBits.second % 8;

    auto     bitsNumber  = configs.activeBits.second - configs.activeBits.first + 1;
    auto     firstBitNum = configs.activeBits.first % 8;
    uint64_t globalMask  = ((1 << bitsNumber) - 1) << firstBitNum;

    for (int i = firstByteNum; i <= lastByteNum; i++) {
        auto byte = static_cast<uint8_t>(data->data.at(i));

        if (configs.bitOrder == DTMsgElement2::BitOrder::REVERSE) {
            byte = DTMsgElement2::convertToReverseBits(byte);
        }

        uint8_t maskForThisByte = globalMask >> ((i - firstByteNum) * 8);
        byte &= maskForThisByte;

        auto leftShiftBitsNum = (i - firstByteNum) * 8 - firstByteFirstBitNum;

        if (i == firstByteNum) {
            container |= byte >> firstByteFirstBitNum & 0xff;
        }
        else {
            container |= (byte & 0xff) << leftShiftBitsNum;
        }
    }
}

void
DecodedData::NormalizeAndStoreMsg(std::shared_ptr<dataPacket> rawData)
{
    ArincMessage msg;

    if (decodeConfigsFile2 != QString{}) {
        msg.timeArrivalPC = rawData->msg_arrival_time;

        for (auto &msgChunk : DTMsgAnatomy) {
            if (msgChunk.second.activeBits.first / 8 >= rawData->bytes_in_buffer ||
                msgChunk.second.activeBits.second / 8 >= rawData->bytes_in_buffer) {
                QMessageBox err{ QMessageBox::Critical, "Data read error", "Not enough bytes in buffer" };
            }
        }

        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["Channel"], msg.channel);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["Label"], msg.labelRaw);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["SDI"], msg.SDI);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["Data"], msg.valueRaw);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["SSM"], msg.SSM);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["Parity"], msg.parity);
        NormalizeAndStoreMsgItem2(rawData, DTMsgAnatomy["Time"], msg.timeRaw);
    }
    else {
        NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Channel"], msg.channel);
        NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Label"], msg.labelRaw);
        NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Data"], msg.valueRaw);
        NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Time"], msg.timeRaw);
    }

    messages.push_back(msg);
}