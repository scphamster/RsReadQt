#include "reading_thread.hpp"
#include "Serial.h"

class ReadingThread::ReadingThreadImpl {
  public:
    explicit ReadingThreadImpl(std::shared_ptr<void> databidgeData, deque_s<std::shared_ptr<::DTdataPacket>> &data);

    SerialInterface serDevice;
    bool   isPaused = false;
};

ReadingThread::ReadingThreadImpl::ReadingThreadImpl(std::shared_ptr<void>                   databidgeConfigs,
                                                    deque_s<std::shared_ptr<::DTdataPacket>> &data)
  : serDevice(std::static_pointer_cast<::SerialConfigs>(databidgeConfigs), data)
{ }

ReadingThread::ReadingThread(std::shared_ptr<void>                   databidgeData,
                             deque_s<std::shared_ptr<::DTdataPacket>> &data,
                             QObject                                *parent)
  : impl{ std::make_unique<ReadingThreadImpl>(databidgeData, data) }
  , QThread(parent)
{ }

ReadingThread::~ReadingThread() = default;

void
ReadingThread::quit()
{
    impl->serDevice.Stop();
    QThread::quit();
}

void
ReadingThread::Pause(bool makePause)
{
    impl->isPaused = makePause;

    if (not makePause)
        impl->serDevice.Flush();
}

void
ReadingThread::run()
{
    // TODO: add error checking to serial
    impl->serDevice.Open();

    while (impl->serDevice.IsOpen()) {
        if (impl->isPaused)
            continue;

        if (impl->serDevice.Read()) {
            emit notificationDataArrived();
        }
    }
}