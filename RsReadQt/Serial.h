#pragma once

#include <vector>
#include <deque>
#include <map>
#include <algorithm>
#include <type_traits>
#include <concepts>

#include <QtGlobal>
#include <QTime>
#include <QFile>

#include "deque_s.hpp"
#include "supermap.hpp"
#include "arinc_model_configs.hpp"

class DTdataPacket;
template<std::integral, Appendable>
class _SerialConfigs;
class serialib;
using SerialConfigs = _SerialConfigs<int, QString>;

class SerialInterface {
  public:
    SerialInterface(std::shared_ptr<::SerialConfigs>, deque_s<std::shared_ptr<DTdataPacket>> &);
    ~SerialInterface();

    char Open();
    bool IsOpen() const;
    bool Read();
    void Flush() const;
    bool Stop();

    std::shared_ptr<DTdataPacket> GetData();

  private:
    std::unique_ptr<serialib>                     __serialdevice;
    std::shared_ptr<_SerialConfigs<int, QString>> serialConfigs;
    deque_s<std::shared_ptr<DTdataPacket>>         &databidgeData;
    ArincQModel::MsgNumT                          dataPacketN = 0;
    int                                           err         = 0;
};
