#pragma once
#include <memory>

#include "ui_mainwindow.h"
#include <QtWidgets/QMainWindow>

#include "deque_s.hpp"
#include "global_configs.hpp"

class DTdataPacket;
class QTabWidget;
class ReadingThread;
class ArincDataDisplay;

class MainWindow : public QMainWindow,
                   public Ui::MainWindowClass {
    Q_OBJECT;

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void OnSerialConfigureClicked();
    void OnStartSerialClicked();
    void OnStopSerialClicked();

  protected:
  private:
    QTabWidget                          *outputTabs = nullptr;
    std::shared_ptr<void>                databridgeConfig;
    deque_s<std::shared_ptr<DTdataPacket>> databridgeData;

    ReadingThread    *readerThr    = nullptr;
    ArincDataDisplay *arincDisplay = nullptr;
};
