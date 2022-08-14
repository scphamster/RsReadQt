#pragma once

#include <vector>
#include <map>
#include <memory>
#include <deque>

#include <QMainWindow>
#include <QTabWidget>

#include <QTime>
#include <qthread.h>
#include <qmutex.h>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QValueAxis>
#include <QScatterSeries>
#include <QLineSeries>
#include <QSplineSeries>

#include <QPlainTextEdit>
#include <qlistview.h>
#include <qlistwidget.h>

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

    void Pause(bool makePuase = true);
    void run() override;

  public slots:
    void quit();

  signals:
    void notificationDataArrived();

  private:
    Serial serDevice;
    bool   isPaused = false;
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
    uint64_t msgNumber = 0;

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

class ArincLabelsChart : public QObject {
    Q_OBJECT
  public:
    void OnLabelOnChartSelected(const QPointF &);
    bool GetDataFromLabelOnChart(const QPointF &);

    QChart                                                                                  *chart  = nullptr;
    QChartView                                                                              *chview = nullptr;
    QValueAxis                                                                              *yaxis  = nullptr;
    QValueAxis                                                                              *xaxis  = nullptr;
    QLineSeries                                                                             *series = nullptr;
    std::map<int, std::pair<QLineSeries *, std::vector<std::pair<qreal, const ArincMsg &>>>> labelsSeries;

    enum ItemSelection {
        NOTSELECTED = UINT64_MAX
    };

    bool      isInitialized = false;
    QDateTime startOfOperation;

    uint64_t selectedItem = NOTSELECTED;

  signals:
    void MsgOnChartBeenSelected(uint64_t msgN);

  private:
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

    bool             newConfigsFile      = false;
    bool             anatomyIsConfigured = false;
    uint64_t         lastMsgReadedNum    = 0;
    ArincLabelsChart diagram;

  private:
};

class OutputThread : public QThread,
                     protected Arinc {
    Q_OBJECT

  public:
    OutputThread() = delete;
    OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                 std::shared_ptr<void>                   dataBridgeConfigs,
                 QTabWidget                             *tabs,
                 // std::shared_ptr<QChart>                 ch,
                 QChart      *ch,
                 QChartView  *chvw,
                 QMainWindow *parent = nullptr);
    ~OutputThread();

    void NormalizeRawData(const auto &data, QString &);
    void ShowDiagram();
    void AddLabelToDiagram(int labelIdx);

  public slots:
    void ShowNewData();
    bool SaveSession();
    void ScrollAndSelectMsg(uint64_t msgN);
    // void quit();

  private:
    QMainWindow *myParent = nullptr;
    QTabWidget  *tabWgt;

    deque_s<std::shared_ptr<::dataPacket>> &dataToOutput;

    QListWidget *outputList = nullptr;
};
