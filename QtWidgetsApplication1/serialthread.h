#pragma once
#include <qthread.h>
#include <qmutex.h>
#include "Serial.h"
#include <qtextedit.h>
#include <qplaintextedit.h>

class SerialThread : public QThread {
    Q_OBJECT
  public:
    SerialThread() = delete;
    SerialThread(std::shared_ptr<void> databridgeConfig, deque_s<std::shared_ptr<::dataPacket>> &data, QObject *parent);
    ~SerialThread();
    void run() override;

  signals:
    void notificationDataArrived();

  private:
    Serial serDevice;
};

class TxtOutputThread : public QThread {
    Q_OBJECT

  public:
    TxtOutputThread() = delete;
    TxtOutputThread(deque_s<std::shared_ptr<::dataPacket>> &data, QPlainTextEdit *lhTxt, QPlainTextEdit *rhTxt);


  public slots:
    void ShowNewData();

  private:
    deque_s<std::shared_ptr<::dataPacket>> &dataToOutput;
    QPlainTextEdit                              *rawOutputTxt = nullptr;
    QPlainTextEdit                              *asciiOutputTxt = nullptr;
};
