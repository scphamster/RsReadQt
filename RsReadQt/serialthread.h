#pragma once
#include <qthread.h>
#include <qmutex.h>

#include <QTime>
#include <qplaintextedit.h>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <qvalueaxis.h>
#include <QScatterSeries>
#include <QLineSeries>
#include <QSplineSeries>

#include <vector>
#include <map>
#include <memory>
#include <deque>

#include "Serial.h"   //this header should be last
// DT = DataTrack

#ifndef DEBUG
#ifdef assert
#undef assert
#endif

#define assert(__expr) __expr
#endif   // DEBUG

class ReadingThread : public QThread {
    Q_OBJECT
  public:
    ReadingThread() = delete;
    ReadingThread(std::shared_ptr<void> databridgeConfig, deque_s<std::shared_ptr<::dataPacket>> &data, QObject *parent);
    //~ReadingThread();
    void run() override;

  public slots:
    void quit();

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
    QTime timeArrivalPC;
    QTime timeArrivalDT;
    int   channel;
    // ArincLabel labelRaw;
    uint8_t  labelRaw;
    uint64_t valueRaw;
    uint8_t  SSM;
    uint8_t  parity;
    uint64_t timeRaw;
    uint8_t  SDI;
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

class DTMsgElement2 {
  public:
    enum {
        UNDEFINED = -1
    };

    enum class BitOrder {
        UNDEFINED = -1,
        NORMAL    = 0,
        REVERSE   = 1
    };

    enum class DataFormat {
        UNDEFINED = -1,
        BIN       = 0,
        DEC,
        OCT,
        HEX,
        BCD
    };

    int                 length     = UNDEFINED;
    BitOrder            bitOrder   = BitOrder::UNDEFINED;
    std::pair<int, int> activeBits = std::pair<int, int>{ UNDEFINED, UNDEFINED };
    DataFormat          dataFormat = DataFormat::UNDEFINED;

    static uint8_t convertToReverseBits(uint8_t data)
    {
        data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
        data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
        data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
        return data;
    }
};

class DecodedData {
  public:
    void GetDecodeConfigsFromFile();
    void GetDecodeConfigsFromFile2();
    void NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTMsgElement &configs, auto &container);
    void NormalizeAndStoreMsgItem2(std::shared_ptr<dataPacket> data, DTMsgElement2 &elemStructure, auto &container);

  protected:
    std::deque<ArincMessage>                        messages;
    std::map<ArincLabel, std::vector<ArincMessage>> labels;

    std::map<QString, DTMsgElement>  DTMessageStructure;
    std::map<QString, DTMsgElement2> DTMsgAnatomy;

    QString decodeConfigsFile;
    QString decodeConfigsFile2;

    bool newConfigsFile      = false;
    bool anatomyIsConfigured = false;

  private:
};

class DataChart {
  public:
    QGraphicsScene            *scene  = nullptr;
    QGraphicsView             *view   = nullptr;
    QChartView                *chview = nullptr;
    QChart                    *chart  = nullptr;
    QValueAxis                *yaxis  = nullptr;
    QValueAxis                *xaxis  = nullptr;
    QLineSeries               *series = nullptr;
    std::map<int, QLineSeries> labelsSeries;

    bool      isInitialized = false;
    QDateTime startOfOperation;
};

class OutputThread : public QThread,
                     protected DecodedData {
    Q_OBJECT

  public:
    OutputThread() = delete;
    OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                 QPlainTextEdit                         *lhTxt,
                 QPlainTextEdit                         *rhTxt,
                 QPlainTextEdit                         *decodedTxt,
                 std::shared_ptr<void>                   dataBridgeConfigs,
                 DataChart                              *chart);

    ~OutputThread();

    void ShowNormalizedRawData(auto data);
    void ShowDiagram();
    void AddLabelToDiagram(int labelIdx);

  public slots:
    void ShowNewData();
    bool SaveSession();
    // void quit();

  private:
    deque_s<std::shared_ptr<::dataPacket>> &dataToOutput;
    QPlainTextEdit                         *rawOutputTxt     = nullptr;
    QPlainTextEdit                         *asciiOutputTxt   = nullptr;
    QPlainTextEdit                         *decodedOutputTxt = nullptr;
    QGraphicsScene                         *graphics         = nullptr;
    DataChart                              *diagram          = nullptr;
};
