#pragma once

#include "ui_mainwindow.h"
#include <QtWidgets/QMainWindow>
#include <qstring.h>
#include <qdockwidget.h>
#include <qtabwidget.h>
#include <qtextedit.h>
#include <qevent.h>

#include "global_configs.hpp"
#include "TextEdit.h"
#include "serialthread.h"
#include "Serial.h"   //this header should be last

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
    deque_s<std::shared_ptr<dataPacket>> databridgeData;

    ReadingThread *readerThr = nullptr;
    OutputThread  *writerThr = nullptr;

    // std::shared_ptr<QChart>     chart;
    QChart     *chart = nullptr;
    QChartView *chview;
};
