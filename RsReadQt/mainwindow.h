#pragma once

#include <QtWidgets/QMainWindow>
#include <qdockwidget.h>
#include <qtextedit.h>
#include <qlistwidget.h>
#include <qevent.h>
#include <qline.h>
#include <qstring.h>

#include "global_configs.hpp"
#include "ui_mainwindow.h"
#include "Serial.h"
#include "serialthread.h"
#include "TextEdit.h"

class MyWidget : public QWidget {
    Q_OBJECT;

  public:
    MyWidget(QWidget *parent = nullptr)
      : QWidget(parent)
    { }

    ~MyWidget() noexcept { }

    void resizeTxtEditors(const QSize &newsize)
    {
        auto newheight = newsize.height();
        auto newwidth  = newsize.width();
        auto lhWidth   = newwidth * lhRhSplitPos - lhRhSplittWidth;
        auto rhWidth   = newwidth - lhWidth;

        lhTxtEdit->setGeometry(0, 0, lhWidth, newheight);
        rhTxtEdit->setGeometry(lhWidth + lhRhSplittWidth, 0, rhWidth - sliderWidth, newheight);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        if (lhTxtEdit == nullptr || rhTxtEdit == nullptr)
            return;

        resizeTxtEditors(event->size());
    }

    void ChangeProportions();

    void SetTxtEditors(auto *lhTxt, auto *rhTxt)
    {
        lhTxtEdit = lhTxt;
        rhTxtEdit = rhTxt;

        resizeTxtEditors(size());
    }

    void mouseMoveEvent(QMouseEvent *ev) override;

  private:
    TextEdit *lhTxtEdit                    = nullptr;
    TextEdit *rhTxtEdit                    = nullptr;
    double    lhRhSplitPos                 = 0.5;
    bool      userIsChangingProportionsNow = false;
};

class MainWindow : public QMainWindow,
                   public Ui::MainWindowClass {
    Q_OBJECT;
    // using TextEdit = QTextEdit;
    using TextEdit = TextEdit;

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    TextEdit *rawOutputTxt   = nullptr;
    TextEdit *asciiOutputTxt = nullptr;

  private slots:
    void OnSerialConfigureClicked();
    void OnStartSerialClicked();
    void OnStopSerialClicked();
    void OnThreadNotificaiton();

  protected:
    MyWidget *outputWidget = nullptr;

  private:
    uint64_t                             rowcounter = 0;
    std::shared_ptr<void>                databridgeConfig;
    deque_s<std::shared_ptr<dataPacket>> databridgeData;

    SerialThread    *readerThr = nullptr;
    TxtOutputThread *writerThr = nullptr;
};
