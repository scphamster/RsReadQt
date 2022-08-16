#pragma once
//#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
//#include "wx/wx.h"   //this include leads to many windows symbols redefinition
#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <type_traits>
#include <concepts>
#include <mutex>
#include <condition_variable>
#include <filesystem>

#include <QtGlobal>
#include <QTranslator>
#include <QTime>
#include <qstring.h>
#include "qmutex.h"
#include <QMutexLocker>
#include <qwaitcondition.h>
#include <qreadwritelock.h>
#include <qfile.h>

//#include "fileapi.h"
#include "serialib.h"

#ifndef _T
#ifdef QT_VERSION
#define _T(__x) QObject::tr(__x)
//#define _T(__x) QStringLiteral(__x)
#endif
#endif

#ifdef DEBUG
#define assert_msg(__cond, __msg) Q_ASSERT(__cond)
#else
#define assert_msg(__cond, __msg) __cond
#endif

constexpr int  SERIAL_DATA_BUFSIZE = 32;
constexpr auto SERIAL_PORT_PREFIX  = "\\\\.\\";

template<typename T>
concept Appendable = requires(T t)
{
    // T::appendd;
    t.append(T());
};

template<typename _Key, typename _Tp>
struct myPair : public std::pair<_Key, _Tp> { };

template<typename _Key,
         typename _Tp,
         typename _Compare = std::less<_Key>,
         typename _Alloc   = std::allocator<std::pair<const _Key, _Tp>>>
struct superMap : public std::map<_Key, _Tp, _Compare, _Alloc> {
    using map = std::map<_Key, _Tp, _Compare, _Alloc>;

    template<typename... Args>
    superMap(Args &&...args)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::forward<Args>(args)...)
    { }

    superMap(const std::initializer_list<typename superMap::value_type> &_Ilist)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::forward<decltype(_Ilist)>(_Ilist))
    { }

    superMap(std::initializer_list<typename superMap::value_type> &&_Ilist)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::move(_Ilist))
    { }

    superMap() = default;

    size_t GetIndexByKeyIfAny(_Key k) const
    {
        auto found = this->find(k);
        if (found == this->end())
            return CB_ERR;   //-1

        return std::distance(this->begin(), found);
    }

    bool ConvToKeyByValueIfAny(_Tp value_tofind, _Key &dest) const
    {
        for (auto &[key, value_here] : *this) {
            if (value_tofind == value_here) {
                dest = key;
                return true;
            }
        }

        return false;
    }

    bool ConvToPairByKeyIfAny(_Key k, std::pair<_Key, _Tp> &dest) const
    {
        if (this->find(k) == this->end())
            return false;
        else {
            dest = std::move(std::pair{ k, map::at(k) });
            return true;
        }
    }

    bool ConvToPairByValueIfAny(const _Tp &value_tofind, std::pair<_Key, _Tp> &dest) const
    {
        for (const auto [key, value] : *this) {
            if (value == value_tofind) {
                dest = std::move(std::pair{ key, value });

                return true;
            }
        }

        return false;
    }

    bool ConvToPairByIdxIfAny(int idx, std::pair<_Key, _Tp> &dest) const
    {
        if (idx >= this->size()) {
            return false;
        }
        else {
            int iterator = 0;

            for (const auto &val : *this) {
                if (iterator == idx) {
                    dest = std::move(std::pair{ val.first, val.second });
                    return true;
                }

                iterator++;
            }

            return false;
        }
    }

    bool ContainsKey(_Key k) const noexcept
    {
        auto found = this->find(k);
        if (found == this->end())
            return false;
        else
            return true;
    }

    bool ContainsValue(_Tp value_tofind) const noexcept
    {
        for (const auto &[key, value] : *this) {
            if (value == value_tofind)
                return true;
        }
        return false;
    }

    std::vector<_Tp> GetAllValues() const noexcept
    {
        std::vector<_Tp> retvect;
        for (const auto &pair : *this) {
            retvect.push_back(this->second);
        }
    }
};

template<typename PairType, typename MapType, typename ValueType>
concept IsGood = requires
{
    std::same_as<PairType, typename MapType::value_type>;
    std::same_as<ValueType, typename MapType::mapped_type>;
};

template<typename PairType, typename MapType, typename ValueType>
    requires IsGood<PairType, MapType, ValueType>
struct _helper {
    bool ConvToPairByValueIfAny(PairType &dest, const MapType &dataset, const ValueType &requestedValue)
    {
        auto iterator = std::find_if(dataset.begin(), dataset.end(), [&requestedValue](auto &pair) {
            return (pair.second == requestedValue);
        });

        if (iterator == dataset.end()) {
            assert_msg(false, _T("Value is: ") + requestedValue);
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

                assert_msg(GetLastError() != ERROR_INSUFFICIENT_BUFFER, _T("Not insufficient buffer for path"));
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

template<typename T, typename Allocator = std::allocator<T>>
class deque_s {
  public:
    using QMutexLocker = QMutexLocker<QMutex>;

    void pop_front_wait(T &buf)
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);

        while (data.empty()) {
            // condition.wait(lk);
            qcondition.wait(lock.mutex());
        }

        buf = data.front();
        data.pop_front();
    }

    // void pop_all_wait(T &buf) { }

    bool empty()
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);
        bool         retval = data.empty();

        return retval;
    }

    void push_back(const T &buf)
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);

        data.push_back(buf);
        // condition.notify_one();
        qcondition.notify_one();
    }

    void clear()
    {
        QMutexLocker lock(&qmutex);
        data.clear();
    }

  private:
    std::deque<T, Allocator> data;
    // std::mutex               mutex;
    // std::condition_variable  condition{ mutex };

    QMutex         qmutex;
    QWaitCondition qcondition;
};

struct dataPacket {
    uint32_t          msg_counter     = 0;
    uint32_t          bytes_in_buffer = 0;
    QDateTime             msg_arrival_time;
    std::vector<char> data{ 0 };

    dataPacket() = default;
    dataPacket(size_t bufferLen, uint32_t msgN, QDateTime &&t)
      : msg_counter(msgN)
      , msg_arrival_time(std::move(t))
    {
        data.resize(bufferLen);
    }
};

class Serial : private dataPacket,
               private serialib {
  public:
    Serial(std::shared_ptr<_SerialConfigs<int, QString>>, deque_s<std::shared_ptr<dataPacket>> &);
    ~Serial() { Stop(); }

    char Open();
    bool IsOpen() { return serialib::isDeviceOpen(); }
    bool Read();
    void Flush() { flushReceiver(); }
    bool Stop();

    QString                             ConvRawToString(std::shared_ptr<Serial::dataPacket> data_item);
    std::shared_ptr<Serial::dataPacket> GetData();
    QString                             ConvRawToPrettyString(std::shared_ptr<dataPacket> data);

  private:
    std::shared_ptr<_SerialConfigs<int, QString>> serialConfigs;
    deque_s<std::shared_ptr<dataPacket>>         &databidgeData;
    uint64_t                                      dataPacketN = 0;
    int                                           err                 = 0;
};
