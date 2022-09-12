#pragma once

#include <vector>
#include <map>
#include <memory>
#include <deque>

#include <QThread>

#include "deque_s.hpp"

class ArincLabelModel2;
class ArincLabelModel;
class ArincDriver;
class ArincLabelsChart;
class LabelsInfo;
class QMainWindow;
class QTabWidget;
class QListWidget;
class dataPacket;

class Output : public QObject {
    Q_OBJECT

  public:
    explicit Output(deque_s<std::shared_ptr<::dataPacket>> &data,
                    std::shared_ptr<void>                   dataBridgeConfigs,
                    QTabWidget                             *tabs,
                    QMainWindow                            *parent = nullptr);
    ~Output();

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
    std::unique_ptr<ArincDriver>      arinc;
    std::unique_ptr<ArincLabelsChart> arincChart;
    // test
    std::shared_ptr<ArincLabelModel2> arincModel;
    // endtest

    QMainWindow *myParent = nullptr;
    QTabWidget  *tabWgt;
    // ArincLabelsChart      *arincChart = nullptr; //oldversion

    deque_s<std::shared_ptr<::dataPacket>> &rawData;
    QListWidget                            *rawMessages = nullptr;
    LabelsInfo                             *labelsInfo  = nullptr;
    ArincLabelModel                        *labels      = nullptr;
};
