#pragma once
#include <utility>
#include <memory>
#include <map>
#include <deque>

#include <QObject>
#include <QWidget>
#include <QBoxLayout>
#include <QCheckBox>
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

class ArincLabel : protected std::pair<int, QString> {
  public:
    using pair = std::pair<int, QString>;

    enum LABELS {
        UNDEFINED = -1
    };

    ArincLabel()
      : pair(UNDEFINED, QString{})
    { }

    explicit ArincLabel(int code)
      : pair(code, ConvertCodeToName(code))
    { }

    bool operator<(const ArincLabel &_Rhs) const noexcept { return (_Rhs.first > this->first); }

    auto GetCode() { return first; }
    auto GetName() { return second; }

    void InitByCode(int raw)
    {
        first  = raw;
        second = QString::number(raw, 8);
    }

    static QString ConvertCodeToName(int code) { return QString::number(code, 8); };
};

class ArincMsg {
  public:
    QDateTime  timeArrivalPC;
    int        channel = 0;
    ArincLabel label;
    uint8_t    labelRaw = 0;
    uint64_t   valueRaw = 0;
    double     value = 0;
    uint8_t    SSM = 0;
    uint8_t    parity = 0;
    uint64_t   DTtimeRaw = 0;
    QTime      DTtime;
    uint8_t    SDI = 0;
    uint64_t   msgNumber = 0;

    bool msgIsHealthy = false;
};

class ScrollBar : public QScrollBar {
    Q_OBJECT

  public:
    template<typename... Args>
     explicit ScrollBar(Args... args)
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

class LineSeries : public QLineSeries {
    Q_OBJECT

  public:
    template<typename... Args>
    explicit LineSeries(Args... args)
      : QLineSeries(std::forward<Args>(args)...)
    { }

    void SetMarkerVisibility(bool visible);
    bool MarkerIsVisible() { return isVisible; }

  private:
    QImage _lightMarker;
    QImage _selectedLightMarker;
    bool   isVisible = true;
};

// TODO: make QWidget protected
class ArincLabelsChart : public QWidget {
    Q_OBJECT
  public:
    ArincLabelsChart() = default;
    explicit ArincLabelsChart(QWidget *parent);
    void OnLabelOnChartSelected(const QPointF &);

    bool GetDataFromLabelOnChart(const QPointF &);
    int  GetIdxOfSelectedMessage() { return idxOfSelectedMsg; }

    bool IsSomeLabelSelected() const noexcept { return idxOfSelectedMsg != ItemSelection::NOTSELECTED; }
    void AddLabel(int channel, int labelIdx);
    void Append(const ArincMsg &msg);
    auto GetLabelMarker(int label) { return labelsSeries.at(label).first->lightMarker(); }

    enum ItemSelection {
        NOTSELECTED = INT64_MAX
    };

  protected:
    void resizeEvent(QResizeEvent *evt) override;
    void wheelEvent(QWheelEvent *evt) override;
    void _AdjustAxisToScroll(QValueAxis *axis, ScrollBar *scroll, int value);
    void _AdjustScrollToAxis(ScrollBar *scroll, qreal min, qreal max);


  signals:
    void MsgOnChartBeenSelected(uint64_t msgN);

  public slots:
    void SetLabelVisibility(int label, Qt::CheckState checkstate);

  private:
    bool eventFilter(QObject *obj, QEvent *evt) override;

    QDateTime   startOfOperation;
    uint64_t    idxOfSelectedMsg          = NOTSELECTED;
    LineSeries *selectedMsgSeriesAffinity = nullptr;

    QCheckBox  *autoRangeChBox = nullptr;
    ScrollBar  *hscroll        = nullptr;
    ScrollBar  *vscroll        = nullptr;
    QChart     *chart          = nullptr;
    QChartView *chview         = nullptr;
    QValueAxis *vaxis          = nullptr;
    QValueAxis *haxis          = nullptr;

    std::map<int, std::pair<LineSeries *, std::vector<std::pair<qreal, const ArincMsg &>>>> labelsSeries;

    bool  manualAxisRange = false;
    qreal haxisZoom       = 1.0;
    qreal vaxisZoom       = 1.0;
    qreal hmax = 0, hmin = 0;
    qreal vmax = 0, vmin = 0;

    // mutexes for elimination of self event generation
    bool skipHScrollValueAdjustment = false;
    bool skipVScrollValueAdjustment = false;
    bool skipHScrollValueChangedEvt = false;
    bool skipVScrollValueChangedEvt = false;
};

class Arinc {
  public:
    Arinc() = default;
    explicit Arinc(const QString &decode_file_name);

    void           GetDecodeConfigsFromFile();
    void           NormalizeAndStoreMsg(std::shared_ptr<dataPacket> data);
    void           NormalizeMsgItem(std::shared_ptr<dataPacket> data, DTWordField &elemStructure, auto &container);
    void           MsgPushBack(ArincMsg &msg) { messages.push_back(msg); }
    decltype(auto) FirstMsg() { return messages.front(); }
    decltype(auto) LastMsg() { return messages.back(); }

    // TODO: make setters getters
    std::deque<ArincMsg>                        messages;
    std::map<ArincLabel, std::vector<ArincMsg>> labels;

    std::map<QString, DTWordField> DTMsgAnatomy;

    QString decodeSpecsFileName;

    bool     newConfigsFile      = false;
    bool     anatomyIsConfigured = false;
    uint64_t lastMsgReadedNum    = 0;

  protected:
  private:
};
