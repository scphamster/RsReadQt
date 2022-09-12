#pragma once
#include <QThread>
#include "deque_s.hpp"

class dataPacket;

class ReadingThread : public QThread {
    Q_OBJECT
  public:
    // ReadingThread() = delete;
    ReadingThread(std::shared_ptr<void> databridgeConfig, deque_s<std::shared_ptr<::dataPacket>> &data, QObject *parent);
    ~ReadingThread();

    void Pause(bool makePuase = true);
    void run() override;

  public slots:
    void quit();

  signals:
    void notificationDataArrived();

  private:
    class ReadingThreadImpl;
    std::unique_ptr<ReadingThreadImpl> impl;
};