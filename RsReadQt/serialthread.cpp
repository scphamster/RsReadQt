#include "serialthread.h"
#include <qtextedit.h>
#include <QTime>
#include <QTextBlock>
#include <qfile.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>

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

TxtOutputThread::TxtOutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                                 QPlainTextEdit                         *lhTxt,
                                 QPlainTextEdit                         *rhTxt,
                                 QPlainTextEdit                         *decodedTxt,
                                 std::shared_ptr<void>                   dataBridgeConfigs)
  : dataToOutput{ data }
  , rawOutputTxt{ lhTxt }
  , asciiOutputTxt{ rhTxt }
  , decodedOutputTxt(decodedTxt)
{
    decodeConfigsFile = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName();
    assert(decodeConfigsFile != QString{});
    GetDecodeConfigsFromFile();
}

void
TxtOutputThread::ShowNormalizedRawData(auto data)
{
    NormalizeAndStoreMsg(data);
    QString decodedOut;
    auto    decoded = messages.back();

    decodedOut.append("Channel ");
    decodedOut.append(QString::number(decoded.channel));

    decodedOut.append("Label ");
    decodedOut.append(QString::number(decoded.labelRaw));

    decodedOut.append("Data ");
    decodedOut.append(QString::number(decoded.valueRaw));

    decodedOut.append("Time ");
    decodedOut.append(QString::number(decoded.timeRaw));

    decodedOutputTxt->appendPlainText(decodedOut);
}

void
TxtOutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data);

    ShowNormalizedRawData(data);

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

void
DecodedData::GetDecodeConfigsFromFile()
{
    auto jsonFile = QFile(decodeConfigsFile);
    jsonFile.setPermissions(QFileDevice::Permission::ReadGroup);
    jsonFile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonFile.readAll();

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
        auto iterator = std::make_reverse_iterator(data->data.begin() + elemStructure.startbyte + (elemStructure.length - 1)) - 1;
        reordreBitsBytes(iterator);
    }
    else {
        auto iterator = data->data.begin() + elemStructure.startbyte;
        reordreBitsBytes(iterator);
    }
}

void
DecodedData::NormalizeAndStoreMsg(std::shared_ptr<dataPacket> rawData)
{
    ArincMessage msg;

    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Channel"], msg.channel);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Label"], msg.labelRaw);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Data"], msg.valueRaw);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Time"], msg.timeRaw);

    messages.push_back(msg);
}