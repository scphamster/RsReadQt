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

class dataPacket;
template<std::integral, Appendable>
class _SerialConfigs;
class serialib;
using SerialConfigs = _SerialConfigs<int, QString>;

class Serial {
  public:
    Serial(std::shared_ptr<::SerialConfigs>, deque_s<std::shared_ptr<dataPacket>> &);
    ~Serial();

    char Open();
    bool IsOpen() const;
    bool Read();
    void Flush() const;
    bool Stop();

    std::shared_ptr<dataPacket> GetData();

  private:
    std::unique_ptr<serialib>                     __serialdevice;
    std::shared_ptr<_SerialConfigs<int, QString>> serialConfigs;
    deque_s<std::shared_ptr<dataPacket>>         &databidgeData;
    ArincQModel::MsgNumT                          dataPacketN = 0;
    int                                           err         = 0;
};
