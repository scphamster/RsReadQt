#pragma once

#include "arinc.hpp"

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

    //struct LabelData {
    //    LineSeries                          *series  = nullptr;
    //    LabelConfigsItem                    *configs = nullptr;
    //    std::vector<qreal, const ArincMsg &> messages;
    //};

    std::map<int, std::pair<LineSeries *, std::vector<std::pair<qreal, const ArincMsg &>>>> labelsSeries;
    //std::map<int, std::unique_ptr<LabelData>>                                               labels;


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
