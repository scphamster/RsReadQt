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

class ArincMsg {
  public:
    QDateTime timeArrivalPC;
    int       channel;
    // ArincLabel labelRaw;
    uint8_t  labelRaw;
    uint64_t valueRaw;
    double   value;
    uint8_t  SSM;
    uint8_t  parity;
    uint64_t DTtimeRaw;
    QTime    DTtime;
    uint8_t  SDI;

    bool msgIsHealthy = false;
};

struct DTWordField {
    enum {
        UNDEFINED = -1
    };

    enum class BitOrder {
        UNDEFINED = -1,
        NORMAL    = 0,
        REVERSE   = 1
    };

    enum class ByteOrder {
        UNDEFINED = -1,
        NORMAL,
        REVERSE
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
    BitOrder            bitOrder   = BitOrder ::UNDEFINED;
    ByteOrder           byteOrder  = ByteOrder::UNDEFINED;
    std::pair<int, int> activeBits = std::pair<int, int>{ UNDEFINED, UNDEFINED };
    DataFormat          dataFormat = DataFormat::UNDEFINED;

    static uint8_t reverseBitsInByte(uint8_t data)
    {
        data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
        data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
        data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
        return data;
    }
};

class Arinc {
  public:
    void GetDecodeConfigsFromFile();
    void NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTWordField &elemStructure, auto &container);

  protected:
    std::deque<ArincMsg>                        messages;
    std::map<ArincLabel, std::vector<ArincMsg>> labels;

    std::map<QString, DTWordField> DTMsgAnatomy;

    QString decodeConfigsFile;

    bool newConfigsFile      = false;
    bool anatomyIsConfigured = false;

  private:
};

class ArincLabelsChart {
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
                     protected Arinc {
    Q_OBJECT

  public:
    OutputThread() = delete;
    OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                 QPlainTextEdit                         *lhTxt,
                 QPlainTextEdit                         *rhTxt,
                 QPlainTextEdit                         *decodedTxt,
                 std::shared_ptr<void>                   dataBridgeConfigs,
                 ArincLabelsChart                              *chart);

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
    ArincLabelsChart                       *diagram          = nullptr;
};
