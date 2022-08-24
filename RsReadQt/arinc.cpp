#include <qjsonobject.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

#include <QMainWindow>
#include <QMessageBox>

#include "arinc.hpp"


Arinc::Arinc(const QString &decode_file_name)
  : decodeSpecsFileName{ decode_file_name }
{
    if (decodeSpecsFileName.isEmpty()) {
        auto nofile = QMessageBox{
            QMessageBox::Icon::Critical,
            "File error",
            "No file specified for DataTrack message decoding",
        };

        nofile.exec();
    }

    GetDecodeConfigsFromFile();
}

void
Arinc::GetDecodeConfigsFromFile()
{
    auto jsonfile = QFile{ decodeSpecsFileName };
    jsonfile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonfile.readAll();
    jsonfile.close();

    DTMsgAnatomy.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    if (err.error != QJsonParseError::NoError) {
        QMessageBox errorMsg{ QMessageBox::Warning,
                              "Json file Error",
                              "Error occured during json file \" " + decodeSpecsFileName +
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
Arinc::NormalizeMsgItem(std::shared_ptr<dataPacket> data, DTWordField &configs, auto &container)
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

    if (decodeSpecsFileName == QString{})
        return;

    msg.timeArrivalPC = rawData->msg_arrival_time;
    msg.msgNumber     = rawData->msg_counter;

    for (auto &msgChunk : DTMsgAnatomy) {
        if (msgChunk.second.activeBits.first / 8 >= rawData->bytes_in_buffer ||
            msgChunk.second.activeBits.second / 8 >= rawData->bytes_in_buffer) {
            QMessageBox err{ QMessageBox::Critical, "Data read error", "Not enough bytes in buffer" };
        }
    }

    NormalizeMsgItem(rawData, DTMsgAnatomy["Channel"], msg.channel);
    NormalizeMsgItem(rawData, DTMsgAnatomy["Label"], msg.labelRaw);
    NormalizeMsgItem(rawData, DTMsgAnatomy["SDI"], msg.SDI);
    NormalizeMsgItem(rawData, DTMsgAnatomy["Data"], msg.valueRaw);
    NormalizeMsgItem(rawData, DTMsgAnatomy["SSM"], msg.SSM);
    NormalizeMsgItem(rawData, DTMsgAnatomy["Parity"], msg.parity);
    NormalizeMsgItem(rawData, DTMsgAnatomy["Time"], msg.DTtimeRaw);

    msg.label.InitByCode(msg.labelRaw);
    messages.push_back(msg);
    labels[msg.label].push_back(msg);
}
