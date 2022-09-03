#pragma once
#include <utility>
#include <memory>
#include <map>
#include <deque>

#include <QObject>
#include <QWidget>
#include <QBoxLayout>
#include <QCheckBox>
#include <QScrollBar>
#include <QString>
#include <QDateTime>
#include <QTime>

#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>

#include "datatrack.hpp"
#include "labelfilter.hpp"
#include "Serial.h"

class _ArincLabel;

class ArincLabel : protected std::pair<int, QString> {
  public:
    using codeName = std::pair<int, QString>;

    enum LABELS {
        UNDEFINED = -1
    };

    ArincLabel()
      : codeName(UNDEFINED, QString{})
    { }

    explicit ArincLabel(int code)
      : codeName(code, ConvertCodeToName(code))
    { }

    bool operator<(const ArincLabel &_Rhs) const noexcept { return (_Rhs.first > this->first); }

    auto GetCode() { return first; }
    auto GetName() { return second; }

    void InitByCode(int raw)
    {
        first  = raw;
        second = QString::number(raw, 8);
    }

    static QString ConvertCodeToName(int code) { return QString::number(code, 8); };
};

class ArincMsg {
  public:
    QDateTime   timeArrivalPC;
    int         channel = 0;
    ArincLabel  label;
    _ArincLabel _label;
    uint8_t     labelRaw  = 0;
    uint64_t    valueRaw  = 0;
    double      value     = 0;
    uint8_t     SSM       = 0;
    uint8_t     parity    = 0;
    uint64_t    DTtimeRaw = 0;
    QTime       DTtime;
    uint8_t     SDI       = 0;
    uint64_t    msgNumber = 0;

    bool msgIsHealthy = false;
};

// struct DTMsgStructure {
//     enum {
//         Undefined = -1
//     };
//
//     QList<QString> msgParts;
//     size_t         msgPartsNumber = Undefined;
//
//
//
//     private:
//     std::map<QString, DTWordField> anatomy;
// };

class Arinc {
  public:
    Arinc() = default;
    explicit Arinc(const QString &decode_file_name);

    void           GetDecodeConfigsFromFile();
    void           NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void           NormalizeMsgItem(std::shared_ptr<dataPacket> data, DTWordField &elemStructure, auto &container);
    void           MsgPushBack(auto msg) { messages.push_back(msg); }
    decltype(auto) FirstMsg() { return messages.front(); }
    decltype(auto) LastMsg() { return messages.back(); }

    // TODO: make setters getters
    std::deque<std::shared_ptr<ArincMsg>>                        messages;
    std::map<ArincLabel, std::vector<std::shared_ptr<ArincMsg>>> labels;

    std::map<QString, DTWordField> DTMsgAnatomy;

    // todo: rework anatomy concept to be more versatile: normalization of raw data shold look like:
    // for (auto part : receiverMsgAnatomy.msgParts)
    //  { NormalizeMsgItem(rawData, part, msg[part]); }
    // DTMsgStructure receiverMsgAnatomy;
    //  DecodeRawMsgFromReceiver(rawData) -> ArincMsg {

    QString decodeSpecsFileName;

    bool     newConfigsFile      = false;
    bool     anatomyIsConfigured = false;
    uint64_t lastMsgReadedNum    = 0;

  protected:
  private:
};
