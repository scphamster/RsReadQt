#include <vector>
#include <QTime>
#include "Serial.h"
#include "deque_s.hpp"
#include "serial_private.hpp"
#include "datatrack.hpp"
#include "serialib.h"

SerialInterface::SerialInterface(std::shared_ptr<::SerialConfigs>        databridgeConfigs,
                                 deque_s<std::shared_ptr<DTdataPacket>> &databidgeData)
  : serialConfigs{ databridgeConfigs }
  , databidgeData{ databidgeData }
  , __serialdevice{ std::make_unique<serialib>() }
{ }

SerialInterface::~SerialInterface()
{
    Stop();
}

char
SerialInterface::Open()
{
    return __serialdevice->openDevice(serialConfigs->GetSelectedPortName().toLocal8Bit(),
                                      serialConfigs->GetSelectedBaudrate().first,
                                      SERIAL_DATABITS_8,   // TODO: make all parameters be taken from serialConfigs
                                      SERIAL_PARITY_NONE,
                                      SERIAL_STOPBITS_1);
}

bool
SerialInterface::IsOpen() const
{
    return __serialdevice->isDeviceOpen();
}

bool
SerialInterface::Read()
{
    auto newdata =
      std::make_shared<DTdataPacket>(serialConfigs->GetSelectedMsgLen(), dataPacketN++, QDateTime::currentDateTime());

    auto ans = __serialdevice->readBytes(newdata->data._Unchecked_begin(), newdata->data.size(), 0);

    if (ans <= 0) {
        err = ans;
        dataPacketN--;
        return false;
    }

    newdata->bytes_in_buffer = ans;

    databidgeData.push_back(newdata);

    return true;
}

void
SerialInterface::Flush() const
{
    __serialdevice->flushReceiver();
}

/**
 * @brief: closes devices if it was open
 *
 * \return true if device was open, otherwise false
 */
bool
SerialInterface::Stop()
{
    if (__serialdevice->isDeviceOpen()) {
        __serialdevice->closeDevice();

        return true;
    }
    else {
        return false;
    }
}

std::shared_ptr<DTdataPacket>
SerialInterface::GetData()
{
    auto data_item = std::make_shared<DTdataPacket>();
    if (databidgeData.empty() == false) {
        databidgeData.pop_front_wait(data_item);
    }

    return data_item;
}
