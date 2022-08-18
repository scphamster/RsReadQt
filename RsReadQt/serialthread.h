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

#include "arinc.hpp"
#include "datatrack.hpp"
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

class OutputThread : public QThread {
    Q_OBJECT

  public:
    OutputThread() = delete;
    OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                 std::shared_ptr<void>                   dataBridgeConfigs,
                 QTabWidget                             *tabs,
                 QMainWindow *parent = nullptr);
    ~OutputThread();

    void NormalizeRawData(const auto &data, QString &);
    void ShowDiagram();
    

  public slots:
    void ShowNewData();
    bool SaveSession();
    void ScrollAndSelectMsg(uint64_t msgN);
    void testSlot(int);

  private:
    QMainWindow           *myParent = nullptr;
    QTabWidget            *tabWgt;
    std::unique_ptr<Arinc> arinc;
    ArincLabelsChart       *arincChart = nullptr;

    deque_s<std::shared_ptr<::dataPacket>> &dataToOutput;
    QListWidget                            *rawMessages = nullptr;

    
};
