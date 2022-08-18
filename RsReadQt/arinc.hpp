#pragma once
#include <utility>
#include <memory>
#include <map>
#include <deque>

#include <QObject>
#include <QWidget>
#include <QBoxLayout>
#include <QScrollBar>
#include <QString>
#include <QDateTime>
#include <QTime>

#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>

#include "datatrack.hpp"
#include "Serial.h"

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

class ScrollBar : public QScrollBar {
    Q_OBJECT

  public:
    template<typename... Args>
    ScrollBar(Args... args)
      : QScrollBar(std::forward<decltype(args)>(args)...)
    { }

    bool ShouldSkipValueAdjustment() { return skipValueAdjustment; }
    bool ShouldSkipValueChangedEvt() { return skipValueChangedEvt; }
    void SetSkipValueAjustment(bool skip) { skipValueAdjustment = skip; }
    void SetSkipValueChangedEvt(bool skip) { skipValueChangedEvt = skip; }

  private:
    bool skipValueAdjustment = false;
    bool skipValueChangedEvt = false;
};

// TODO: make QWidget protected
class ArincLabelsChart : public QWidget {
    Q_OBJECT
  public:
    ArincLabelsChart() = default;
    ArincLabelsChart(QWidget *parent);
    void OnLabelOnChartSelected(const QPointF &);

    bool GetDataFromLabelOnChart(const QPointF &);
    int  GetIdxOfSelectedMessage() { return idxOfSelectedMsg; }

    bool IsSomeLabelSelected() const noexcept { return idxOfSelectedMsg != ItemSelection::NOTSELECTED; }
    void AddLabel(int channel, int labelIdx);
    void AppendLabelSeries(uint8_t label_num, qreal msg_time);
    void Append(const ArincMsg &msg);

    enum ItemSelection {
        NOTSELECTED = UINT64_MAX
    };

  protected:
    void resizeEvent(QResizeEvent *evt) override;
    void wheelEvent(QWheelEvent *evt) override;
    void _AdjustAxisToScroll(QValueAxis *axis, ScrollBar *scroll, int value);
    void _AdjustScrollToAxis(ScrollBar *scroll, qreal min, qreal max);

  signals:
    void MsgOnChartBeenSelected(uint64_t msgN);

  private slots:
    void AdjustVScroll(qreal min, qreal max);
    void AdjustHScroll(qreal min, qreal max);
    void OnVScrollValueChanged(int value);
    void OnHScrollValueChanged(int value);

  private:
    bool eventFilter(QObject *obj, QEvent *evt) override;

    bool      isInitialized = false;
    QDateTime startOfOperation;

    uint64_t     idxOfSelectedMsg          = NOTSELECTED;
    QLineSeries *selectedMsgSeriesAffinity = nullptr;

    ScrollBar   *hscroll         = nullptr;
    ScrollBar   *vscroll         = nullptr;
    QVBoxLayout *sizer           = nullptr;
    bool         manualAxisRange = false;
    qreal        haxisZoom       = 1.0;
    qreal        vaxisZoom       = 1.0;
    qreal        hmax = 0, hmin = 0;
    qreal        vmax = 0, vmin = 0;
    qreal        hAxisVisibleLen = -1;
    qreal        vAxisVisibleLen = -1;

    QChart      *chart  = nullptr;
    QChartView  *chview = nullptr;
    QValueAxis  *yaxis  = nullptr;
    QValueAxis  *xaxis  = nullptr;
    QLineSeries *series = nullptr;

    std::map<int, std::pair<QLineSeries *, std::vector<std::pair<qreal, const ArincMsg &>>>> labelsSeries;

    // mutexes for elimination of self event generation
    bool skipHScrollValueAdjustment = false;
    bool skipVScrollValueAdjustment = false;
    bool skipHScrollValueChangedEvt = false;
    bool skipVScrollValueChangedEvt = false;
};

class Arinc {
  public:
    Arinc() = default;
    Arinc(QString decodingFile);

    void GetDecodeConfigsFromFile();
    void NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void NormalizeMsgItem(std::shared_ptr<dataPacket> data, DTWordField &elemStructure, auto &container);
    void MsgPushBack(ArincMsg &msg) { messages.push_back(msg); }
    auto MsgPopFront() { return messages.front(); }

    // TODO: make setters getters
    std::deque<ArincMsg>                        messages;
    std::map<ArincLabel, std::vector<ArincMsg>> labels;

    std::map<QString, DTWordField> DTMsgAnatomy;

    QString decodeConfigsFile;

    bool     newConfigsFile      = false;
    bool     anatomyIsConfigured = false;
    uint64_t lastMsgReadedNum    = 0;

  protected:
  private:
};
