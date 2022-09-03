#pragma once

#include <vector>
#include <map>
#include <memory>
#include <deque>

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeView>
#include <QPlainTextEdit>
#include <qlistview.h>
#include <qlistwidget.h>

#include <QList>
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

#include "arinc.hpp"
#include "arinc_chart.hpp"
#include "datatrack.hpp"
#include "labelfilter.hpp"
//#include "_ArincLabelModel.hpp"
#include "Serial.h"   //this header should be last

class _ArincLabelModel;
// DT = DataTrack

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

class LabelsInfo : public QTreeWidget {
    Q_OBJECT

  public:
    enum Column {
        Name = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram
    };

    template<typename... Args>
    LabelsInfo(Args... args)
      : QTreeWidget(std::forward<Args>(args)...)
    { }

    void Update(auto msg, int counter, QImage &&);
    bool ShouldBeepOnArival(int label);

  public slots:
    void OnLabelInfoChanged(QTreeWidgetItem *item, int column);

  signals:
    void LabelVisibilityChoiceChanged(int label, Qt::CheckState checkstate);

  private:
    std::map<int, QTreeWidgetItem *> labels;
    bool                             skipDataChangeEvent = false;
};

class Output : public QObject {
    Q_OBJECT

  public:
    Output() = delete;
    Output(deque_s<std::shared_ptr<::dataPacket>> &data,
           std::shared_ptr<void>                   dataBridgeConfigs,
           QTabWidget                             *tabs,
           QMainWindow                            *parent = nullptr);
    ~Output() final = default;

    void CreateRawOutput();
    void CreateLabelsInfo();
    void CreateFilter();
    void NormalizeRawData(const auto &data, QString &);
    void ShowDiagram();

  public slots:
    void ShowNewData();
    bool SaveSession();
    void ScrollAndSelectMsg(size_t msgN);

  private:
    QMainWindow           *myParent = nullptr;
    QTabWidget            *tabWgt;
    std::unique_ptr<Arinc> arinc;
    // ArincLabelsChart      *arincChart = nullptr;
    std::unique_ptr<ArincLabelsChart> arincChart;

    deque_s<std::shared_ptr<::dataPacket>> &rawData;
    QListWidget                            *rawMessages = nullptr;
    LabelsInfo                             *labelsInfo  = nullptr;
    ArincLabelModel                        *labels      = nullptr;

    // test
    std::shared_ptr<_ArincLabelModel>      arincModel;

    // endtest
};
