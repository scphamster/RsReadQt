#pragma once
#include <utility>
#include <memory>
#include <map>

#include "datatrack.hpp"
#include "labelfilter.hpp"

#include "arinc_model_configs.hpp"

class _ArincLabel;
class dataPacket;

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
    void SetSelection(bool if_selected = true) noexcept { isSelected = if_selected; }
    void SetChannel(ArincQModel::ChannelNumT ch) noexcept { channel = ch; }
    void SetLabel(const _ArincLabel &new_label) noexcept(std::is_nothrow_assignable_v<_ArincLabel, _ArincLabel>)
    {
        _label = new_label;
    }
    void SetValue(ArincQModel::ValueT new_value) noexcept { valueRaw = new_value; }
    void SetSSM(ArincQModel::ssmT new_ssm) noexcept { SSM = new_ssm; }
    void SetParity(ArincQModel::ParityT new_parity) noexcept { parity = new_parity; }
    void SetDTRawTime(ArincQModel::DTRawTime new_time) noexcept { DTtimeRaw = new_time; }
    void SetSDI(ArincQModel::sdiT new_sdi) noexcept { SDI = new_sdi; }
    void SetMsgNumber(ArincQModel::MsgNumT new_msg_num) noexcept { msgNumber = new_msg_num; }

    ArincQModel::ChannelNumT GetChannel() const noexcept { return channel; }
    _ArincLabel              GetLabel() const noexcept(std::is_nothrow_constructible_v<_ArincLabel>) { return _label; }
    ArincQModel::ValueT      GetValue() const noexcept { return valueRaw; }
    ArincQModel::ssmT        GetSSM() const noexcept { return SSM; }
    ArincQModel::ParityT     GetParity() const noexcept { return parity; }
    ArincQModel::DTRawTime   GetDTRawTime() const noexcept { return DTtimeRaw; }
    ArincQModel::sdiT        GetSDI() const noexcept { return SDI; }
    ArincQModel::MsgNumT     GetMsgNumber() const noexcept { return msgNumber; }

    QDateTime GetTimeArival() const { return timeArrivalPC; }
    template<typename RetType = _ArincLabel>
    RetType GetLabel() const noexcept
    {
        return _label;
    }
    template<>
    ArincQModel::LabelNumT GetLabel() const noexcept
    {
        return labelRaw;
    }
    template<>
    ArincQModel::LabelTxtT GetLabel() const noexcept
    {
        return ArincQModel::LabelTxtT::number(labelRaw, 8);
    }

    uint8_t     labelRaw  = 0;
    uint8_t     SSM       = 0;
    uint8_t     parity    = 0;
    uint8_t     SDI       = 0;
    uint64_t    valueRaw  = 0;
    uint64_t    msgNumber = 0;
    uint64_t    DTtimeRaw = 0;
    int         channel   = 0;
    double      value     = 0;
    QDateTime   timeArrivalPC;
    QTime       DTtime;
    ArincLabel  label;
    _ArincLabel _label;

    bool isSelected   = false;
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

class ArincDriver {
  public:
    ArincDriver();
    ~ArincDriver();
    explicit ArincDriver(const QString &decode_file_name);

    void SetDecodeConfigsFileName(const QString &filename) noexcept(std::is_nothrow_assignable_v<QString, QString &>);
    QString const &GetDecodeConfigsFileName() const noexcept;
    void           GetDecodeConfigsFromFile();
    void           NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void           NormalizeMsgItem(std::shared_ptr<dataPacket> data, DTWordField &elemStructure, auto &container);
    void           MsgPushBack(auto msg);
    std::shared_ptr<ArincMsg> FirstMsg() const noexcept(std::is_nothrow_constructible_v<std::shared_ptr<ArincMsg>>);
    std::shared_ptr<ArincMsg> LastMsg() const noexcept(std::is_nothrow_constructible_v<std::shared_ptr<ArincMsg>>);
    ArincQModel::MsgNumT      GetLastReadedMsgNumber() const noexcept;
    void                      SetLastReadedMsgNumber(ArincQModel::MsgNumT count) noexcept;

  protected:
  private:
    struct ArincDriverImpl;
    std::unique_ptr<ArincDriverImpl> impl;
};
