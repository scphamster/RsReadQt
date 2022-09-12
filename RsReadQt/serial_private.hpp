#pragma once
#include <QDateTime>
#include "serialib.h"
#include "arinc_model_configs.hpp"


#ifndef _T
#ifdef QT_VERSION
#define _T(__x) QObject::tr(__x)
//#define _T(__x) QStringLiteral(__x)
#endif
#endif

constexpr int  SERIAL_DATA_BUFSIZE = 32;
constexpr auto SERIAL_PORT_PREFIX  = "\\\\.\\";

template<typename PairType, typename MapType, typename ValueType>
    requires IsGood<PairType, MapType, ValueType>
struct _helper {
    bool ConvToPairByValueIfAny(PairType &dest, const MapType &dataset, const ValueType &requestedValue)
    {
        auto iterator = std::find_if(dataset.begin(), dataset.end(), [&requestedValue](const auto &pair) {
            return (pair.second == requestedValue);
        });

        if (iterator == dataset.end()) {
            false, _T("Value is: ") + requestedValue;
            return false;
        }

        dest = *iterator;
        return true;
    }
};

template<std::integral _Key, Appendable _Val>
class _SerialConfigs : protected _helper<std::pair<_Key, _Val>, superMap<_Key, _Val>, _Val> {
  public:
    using single_config_t     = std::pair<_Key, _Val>;
    using configs_container_t = superMap<_Key, _Val>;

    enum portSelection {
        PORT_NOT_SELECTED = -1
    };

    void UpdateAvlblPorts()
    {
        constexpr int NotPresent = 0;
        ResetAvlblPorts();
        WCHAR pathBuffer[5000];         // buffer to store the path of the COMPORTS
        for (int i = 0; i < 255; i++)   // checking ports from COM0 to COM255
        {
            QString portname = QStringLiteral("COM");
            portname.append(QString::number(i));

            auto p = std::wstring(L"COM") + std::to_wstring(i);

            // TODO: cleanup after tests
            // auto port = portname.toStdWString().c_str();
            // auto bufsze = sizeof(pathBuffer) / sizeof(pathBuffer[0]);

            DWORD portIsPresent = QueryDosDevice(p.c_str(), pathBuffer, sizeof(pathBuffer) / sizeof(pathBuffer[0]));

            if (portIsPresent != NotPresent)   // QueryDosDevice returns zero if it didn't find an object
            {
                avlblPorts[i] = portname;

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    // todo: log error
                    return;
                }
            }
        }
    }

    // getters
    const configs_container_t GetAvlblComPorts() const noexcept { return avlblPorts; }
    const configs_container_t GetAvlblBaudrates() const noexcept { return avlblBaudrates; }
    const configs_container_t GetAvlblDatabits() const noexcept { return avlblDatabits; }
    const configs_container_t GetAvlblStopbits() const noexcept { return avlblStopbits; }
    const configs_container_t GetAvlblParities() const noexcept { return avlblParities; }

    const single_config_t GetSelectedPort() const noexcept { return port; }
    const QString         GetSelectedPortName() const noexcept { return port.second; }
    bool                  PreviouslySelectedPortIsPresent() const noexcept
    {
        if (avlblPorts.ContainsValue(GetSelectedPortName()))
            return true;
        else
            return false;
    }
    const single_config_t              GetSelectedBaudrate() const noexcept { return baudrate; }
    const single_config_t              GetSelectedDatabits() const noexcept { return databits; }
    const single_config_t              GetSelectedStopbits() const noexcept { return stopbits; }
    const single_config_t              GetSelectedParity() const noexcept { return parity; }
    const _Key                         GetSelectedMsgLen() const noexcept { return dataTrackMsgLen; }
    const std::vector<single_config_t> GetAllConfigs() noexcept(
      std::is_nothrow_constructible_v<std::vector<single_config_t>, single_config_t>)
    {
        return std::vector<single_config_t>{ port, baudrate, databits, stopbits, parity };
    }
    QString GetConfigsFileName() { return decodeConfigsFile2; }

    using _helper<std::pair<_Key, _Val>, superMap<_Key, _Val>, _Val>::ConvToPairByValueIfAny;

    // setters, return true on success otherwise - false
    bool SetBaudrateByValue(const QString &value) { return ConvToPairByValueIfAny(baudrate, avlblBaudrates, value); }
    bool SetDatabitsByValue(const QString &value) { return ConvToPairByValueIfAny(databits, avlblDatabits, value); }
    bool SetStopbitsByValue(const QString &value) { return ConvToPairByValueIfAny(stopbits, avlblStopbits, value); }
    bool SetParityByValue(const QString &value) { return ConvToPairByValueIfAny(parity, avlblParities, value); }
    bool SetMsgLen(const _Key newMsgLen) noexcept
    {
        if (newMsgLen < avlblMsgLen.first || newMsgLen > avlblMsgLen.second)
            return false;

        dataTrackMsgLen = newMsgLen;
        return true;
    }
    void                                SetConfigsFileName2(const QString &name) { decodeConfigsFile2 = name; }
    bool                                ConfigsFileNameIsSpecified2() { return !decodeConfigsFile2.empty(); }
    static const inline single_config_t PORT_UNDEFINED{ PORT_NOT_SELECTED, _T("Undefined") };

    bool SetPortByName(const QString &portname) noexcept(
      std::is_nothrow_invocable_v<decltype(&configs_container_t::ConvToKeyByValueIfAny), QString, int>)
    {
        return ConvToPairByValueIfAny(port, avlblPorts, portname);
    }
    bool SetPortByKey(const int portKey) noexcept(
      std::is_nothrow_invocable_v<decltype(&configs_container_t::ContainsKey), const int>)
    {
        if (avlblPorts.ContainsKey(portKey)) {
            port.first  = portKey;
            port.second = avlblPorts[portKey];
            return true;
        }
        else {
            port = PORT_UNDEFINED;
            return false;
        }
    }
    void SetPortToUndefined() noexcept { port = PORT_UNDEFINED; }
    void ResetAvlblPorts() noexcept
    {
        avlblPorts.clear();
        avlblPorts = { { PORT_NOT_SELECTED, _T("Undefined" ) } };
    }

  protected:
    // limits
    configs_container_t avlblPorts{ { PORT_NOT_SELECTED, _T("Undefined" ) } };

    static const inline configs_container_t avlblBaudrates{
        { 110, _T("110" ) },       { 300, _T("300" ) },       { 600, _T("600" ) },      { 1200, _T("1200" ) },
        { 2400, _T("2400" ) },     { 4800, _T("4800" ) },     { 9600, _T("9600" ) },    { 14400, _T("14400" ) },
        { 19200, _T("19200" ) },   { 38400, _T("38400" ) },   { 56000, _T("56000" ) },  { 57600, _T("57600" ) },
        { 115200, _T("115200" ) }, { 128000, _T("128000" ) }, { 256000, _T("256000" ) }
    };

    static const inline configs_container_t avlblDatabits{ { SERIAL_DATABITS_5, _T("5"  ) },
                                                           { SERIAL_DATABITS_6, _T("6"  ) },
                                                           { SERIAL_DATABITS_7, _T("7"  ) },
                                                           { SERIAL_DATABITS_8, _T("8"  ) },
                                                           { SERIAL_DATABITS_16, _T( "16 ") } };
    static const inline configs_container_t avlblStopbits{ { SERIAL_STOPBITS_1, _T("1" ) },
                                                           { SERIAL_STOPBITS_1_5, _T("1_5" ) },
                                                           { SERIAL_STOPBITS_2, _T("2" ) } };

    static const inline configs_container_t avlblParities{ { SERIAL_PARITY_NONE, _T("NONE" ) },
                                                           { SERIAL_PARITY_EVEN, _T("EVEN" ) },
                                                           { SERIAL_PARITY_ODD, _T("ODD" ) },
                                                           { SERIAL_PARITY_MARK, _T("MARK" ) },
                                                           { SERIAL_PARITY_SPACE, _T("SPACE" ) } };

    static const inline std::pair<_Key, _Key> avlblMsgLen{ 1, 8 };

    // selections stored here
    single_config_t port     = PORT_UNDEFINED;
    single_config_t baudrate = { 115200, _T("115200" ) };
    single_config_t databits = { SERIAL_DATABITS_8, _T("8" ) };
    single_config_t stopbits = { SERIAL_STOPBITS_1, _T("1" ) };
    single_config_t parity   = { SERIAL_PARITY_NONE, _T("NONE" ) };
    _Key            dataTrackMsgLen{ 8 };
    QString         decodeConfigsFile2;
};

using SerialConfigs = _SerialConfigs<int, QString>;

struct dataPacket {
    dataPacket() = default;
    dataPacket(int bufferLen, ArincQModel::MsgNumT msg_idx, QDateTime &&t)
      : msgIdx(msg_idx)
      , msg_arrival_time(std::move(t))
    {
        data.resize(static_cast<size_t>(bufferLen));
    }

    ArincQModel::MsgNumT msgIdx          = 0;
    uint32_t             bytes_in_buffer = 0;
    QDateTime            msg_arrival_time;
    std::vector<char>    data{ 0 };
};
