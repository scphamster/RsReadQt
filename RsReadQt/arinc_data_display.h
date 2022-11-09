#pragma once

#include <vector>
#include <map>
#include <memory>
#include <deque>

#include <QThread>

#include "deque_s.hpp"
#include "datatrack.hpp"

class ArincLabelModel2;
class ArincLabelModel;
class ArincPhysicalInterface;
class ArincLabelsChart;
class LabelsInfo;
class QMainWindow;
class QTabWidget;
class QListWidget;
class DTdataPacket;

class ArincDataDisplay : public QObject {
    Q_OBJECT

  public:
    explicit ArincDataDisplay(deque_s<std::shared_ptr<::DTdataPacket>> &data,
                              std::shared_ptr<void>                   dataBridgeConfigs,
                              QTabWidget                             *tabs,
                              QMainWindow                            *parent = nullptr);
    ~ArincDataDisplay();

    void CreateRawOutput();
    void CreateLabelsInfo();
    void CreateFilter();
    void NormalizeRawData(const std::shared_ptr<DTdataPacket> &data, QString &);
    void ShowDiagram();

  public slots:
    void ShowNewData();
    bool SaveSession();
    void ScrollAndSelectMsg(size_t msgN);

  private:
    QMainWindow *myParent = nullptr;
    QTabWidget  *tabWgt;

    deque_s<std::shared_ptr<::DTdataPacket>> &rawDTData;

    std::unique_ptr<ArincPhysicalInterface> arincInterface;
    QListWidget                            *rawArincMessages = nullptr;

    std::unique_ptr<ArincLabelsChart> arincChart;
    ArincLabelModel                  *labels     = nullptr;
    LabelsInfo                       *labelsInfo = nullptr;

    // test
    std::shared_ptr<ArincLabelModel2> arincModel;
    // ArincLabelsChart      *arincChart = nullptr; //oldversion
    // endtest
};
