#include <deque>

#include <qjsonobject.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

#include <QMainWindow>
#include <QMessageBox>

#include "arinc.hpp"
#include "Serial.h"
#include "serial_private.hpp"
#include "datatrack.hpp"

class ArincPhysicalInterface::ArincDriverImpl {
  public:
    bool                                                         DTMsgAnatomyIsInitialized = false;
    uint64_t                                                     lastMsgReadedNum          = 0;
    QString                                                      DTMsgDecodeConfigsFile;
    std::map<QString, DTWordField>                               DTMsgAnatomy;
    std::deque<std::shared_ptr<ArincMsg>>                        messages;
    std::map<ArincLabel, std::vector<std::shared_ptr<ArincMsg>>> labels;
};

ArincPhysicalInterface::ArincPhysicalInterface()
  : impl{ std::make_unique<ArincDriverImpl>() }
{ }

ArincPhysicalInterface::~ArincPhysicalInterface() = default;

ArincPhysicalInterface::ArincPhysicalInterface(const QString &decode_file_name)
  : impl{ std::make_unique<ArincDriverImpl>() }
{
    SetDecodeConfigsFileName(decode_file_name);

    if (decode_file_name.isEmpty()) {
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
ArincPhysicalInterface::SetDecodeConfigsFileName(const QString &filename) noexcept(
  std::is_nothrow_assignable_v<QString, QString &>)
{
    impl->DTMsgDecodeConfigsFile = filename;
}

QString const &
ArincPhysicalInterface::GetDecodeConfigsFileName() const noexcept
{
    return impl->DTMsgDecodeConfigsFile;
}

void
ArincPhysicalInterface::GetDecodeConfigsFromFile()
{
    auto jsonfile = QFile{ GetDecodeConfigsFileName() };
    jsonfile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonfile.readAll();
    jsonfile.close();

    impl->DTMsgAnatomy.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    if (err.error != QJsonParseError::NoError) {
        QMessageBox errorMsg{ QMessageBox::Warning,
                              "Json file Error",
                              "Error occured during json file \" " + GetDecodeConfigsFileName() +
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

    impl->DTMsgAnatomy.clear();

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

            impl->DTMsgAnatomyIsInitialized = false;
            return;
        }

        if (chunk.activeBits.second > msgLenBits_calculated || chunk.activeBits.first > msgLenBits_calculated) {
            QMessageBox errorMsg{
                QMessageBox::Critical,
                "Json file Error",
                "Message part with name: " + chunkName.toString() +
                  " has active bits number exceeding number of bits calculated from \"Mesaage length bytes\""
            };

            impl->DTMsgAnatomyIsInitialized = false;
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

        impl->DTMsgAnatomy[chunkName.toString()] = std::move(chunk);
    }

    impl->DTMsgAnatomyIsInitialized = true;
}

void
ArincPhysicalInterface::NormalizeMsgItem(std::shared_ptr<DTdataPacket> data, DTWordField &configs, auto &container)
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
ArincPhysicalInterface::MsgPushBack(auto msg)
{
    impl->messages.push_back(msg);
}

void
ArincPhysicalInterface::NormalizeAndStoreMsg(std::shared_ptr<DTdataPacket> rawData)
{
    auto msg = std::make_shared<ArincMsg>();

    if (GetDecodeConfigsFileName() == QString{})
        return;

    msg->timeArrivalPC = rawData->msg_arrival_time;
    msg->msgNumber     = rawData->msgIdx;

    for (auto &msgChunk : impl->DTMsgAnatomy) {
        if (msgChunk.second.activeBits.first / 8 >= rawData->bytes_in_buffer ||
            msgChunk.second.activeBits.second / 8 >= rawData->bytes_in_buffer) {
            QMessageBox err{ QMessageBox::Critical, "Data read error", "Not enough bytes in buffer" };
        }
    }

    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["Channel"], msg->channel);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["Label"], msg->labelRaw);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["SDI"], msg->SDI);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["Data"], msg->valueRaw);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["SSM"], msg->SSM);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["Parity"], msg->parity);
    NormalizeMsgItem(rawData, impl->DTMsgAnatomy["Time"], msg->DTtimeRaw);

    msg->label.InitByCode(msg->labelRaw);
    msg->_label.Set(msg->labelRaw);

    impl->messages.push_back(msg);
    impl->labels[msg->label].push_back(msg);
}

std::shared_ptr<ArincMsg>
ArincPhysicalInterface::FirstMsg() const noexcept(std::is_nothrow_constructible_v<std::shared_ptr<ArincMsg>>)
{
    return impl->messages.front();
}

std::shared_ptr<ArincMsg>
ArincPhysicalInterface::LastMsg() const noexcept(std::is_nothrow_constructible_v<std::shared_ptr<ArincMsg>>)
{
    return impl->messages.back();
}

ArincQModel::MsgNumT
ArincPhysicalInterface::GetLastReadedMsgNumber() const noexcept
{
    return impl->lastMsgReadedNum;
}

void
ArincPhysicalInterface::SetLastReadedMsgNumber(ArincQModel::MsgNumT count) noexcept
{
    impl->lastMsgReadedNum = count;
}
