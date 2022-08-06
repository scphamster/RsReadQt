#pragma once
#include <qthread.h>
#include <qmutex.h>
#include "Serial.h"
#include <qtextedit.h>
#include <qplaintextedit.h>
#include <QTime>

#include <vector>
#include <map>
#include <memory>
#include <deque>

//DT = DataTrack

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

class ArincLabel : protected std::pair<int, QString> {
  public:
    using pair = std::pair<int, QString>;

    enum LABELS {
        UNDEFINED = -1
    };

    ArincLabel()
      : pair(UNDEFINED, "")
    { }

    ArincLabel(int code)
      : pair(code, ConvertCodeToName(code))
    { }

    auto           Code() { return first; }
    auto           Name() { return second; }
    static QString ConvertCodeToName(int code) { return QString{}; }   // TODO: implement
};

class ArincMessage {
  public:
    QTime      timeArrivalPC;
    QTime      timeArrivalDT;
    int        channel;
    //ArincLabel labelRaw;
    uint8_t    labelRaw;
    uint64_t   valueRaw;
    uint64_t   timeRaw;
};

class DTMsgElement {
  public:
    enum class BitOrder {
        MSB = 0,
        LSB = 1
    };

    enum class ByteOrder {
        MSB = 0,
        LSB = 1
    };

    enum class DataFormat {
        BIN = 0,
        DEC,
        OCT,
        HEX,
        BCD
    };

    int                 length     = 0;
    BitOrder            bitOrder   = BitOrder::MSB;
    ByteOrder           byteOrder  = ByteOrder::MSB;
    std::pair<int, int> activeBits = std::pair<int, int>{ 0, 0 };
    int                 startbyte  = -1;
    DataFormat          dataFormat = DataFormat::BIN;

    static uint8_t reverseBits(uint8_t data);
};

class DecodedData {
  public:
    void GetDecodeConfigsFromFile();
    void NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTMsgElement &configs, auto &container);

  protected:
    std::deque<ArincMessage>                       messages;
    std::map<ArincLabel, std::vector<ArincMessage>> labels;

    std::map<QString, DTMsgElement> DTMessageStructure;
    QString                         decodeConfigsFile;

  private:
};

class TxtOutputThread : public QThread,
                        protected DecodedData {
    Q_OBJECT

  public:
    TxtOutputThread() = delete;
    TxtOutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                    QPlainTextEdit                         *lhTxt,
                    QPlainTextEdit                         *rhTxt,
                    QPlainTextEdit                         *decodedTxt,
                    std::shared_ptr<void>                   dataBridgeConfigs);

    void ShowNormalizedRawData(auto data);
  public slots:
    void ShowNewData();

  private:
    deque_s<std::shared_ptr<::dataPacket>> &dataToOutput;
    QPlainTextEdit                         *rawOutputTxt     = nullptr;
    QPlainTextEdit                         *asciiOutputTxt   = nullptr;
    QPlainTextEdit                         *decodedOutputTxt = nullptr;
};
