#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <ctime>
#include "Serial.h"
#include "qstring.h"

Serial::Serial(std::shared_ptr<_SerialConfigs<int, QString>> databridgeConfigs,
               deque_s<std::shared_ptr<dataPacket>>         &databidgeData)
  : serialConfigs{ databridgeConfigs }
  , databidgeData{ databidgeData }
{ }

char
Serial::Open()
{
    return openDevice(serialConfigs->GetSelectedPortName().toLocal8Bit(),
                      serialConfigs->GetSelectedBaudrate().first,
                      SERIAL_DATABITS_8,   // TODO: make all parameters be taken from serialConfigs
                      SERIAL_PARITY_NONE,
                      SERIAL_STOPBITS_1);
}

bool
Serial::Read()
{
    std::time_t msgtime;

    std::time(&msgtime);
    auto newdata = std::make_shared<dataPacket>(serialConfigs->GetSelectedMsgLen(), data_packet_counter++, msgtime);

    auto ans = readBytes(newdata->data._Unchecked_begin(), newdata->data.size(), 0);

    if (ans <= 0) {
        err = ans;
        return false;
    }

    newdata->bytes_in_buffer = ans;
    databidgeData.push_back(newdata);

    return true;
}

/**
 * @brief: closes devices if it was open
 *
 * \return true if device was open, otherwise false
 */
bool
Serial::Stop()
{
    if (isDeviceOpen()) {
        closeDevice();

        return true;
    }
    else {
        return false;
    }
}

QString
Serial::ConvRawToString(std::shared_ptr<Serial::dataPacket> data_item)
{
    QString retString;
    //retString.append(data_item->data);

    return retString;
}

std::shared_ptr<Serial::dataPacket>
Serial::GetData()
{
    auto data_item = std::make_shared<dataPacket>();
    if (databidgeData.empty() == false) {
        databidgeData.pop_front_wait(data_item);
    }

    return data_item;
}

QString
Serial::ConvRawToPrettyString(std::shared_ptr<dataPacket> data)
{
    char        buffer[100];
    tm         *timeinfo  = new tm;
    std::time_t arvl_time = data->msg_arrival_time;

    localtime_s(timeinfo, &arvl_time);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
    QString retstr = "Data Arival Time: ";
    retstr.append(buffer);

    retstr.append("   Bytes in buffer: ");
    retstr.append(std::to_wstring(data->bytes_in_buffer));

    retstr.append("RawData: ");

    for (int i = 0; (i < data->bytes_in_buffer) && (i < sizeof(data->data)); i++) {
        retstr.append(std::to_wstring((data->data[i])));
        retstr.append(" ");
    }
    retstr.append("\n");

    delete timeinfo;
    return retstr;
}