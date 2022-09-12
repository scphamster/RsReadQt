#pragma once
#include <QWidget>
#include <QDateTime>

class QCheckBox;
class ArincMsg;
class ScrollBar;
class QValueAxis;
class LineSeries;
class QChartView;
class QChart;

// TODO: make QWidget protected
class ArincLabelsChart : public QWidget {
    Q_OBJECT
  public:
    enum ItemSelection {
        NOTSELECTED = INT64_MAX
    };

    ArincLabelsChart() = default;
    explicit ArincLabelsChart(QWidget *parent);

    void OnLabelOnChartSelected(const QPointF &);

    void Append(std::shared_ptr<ArincMsg>);
    void AddLabel(int channel, int labelIdx);

    bool SetMsgOnChartToSelectedState(const QPointF &);
    int  GetIdxOfSelectedMessage();
    auto GetLabelMarker(int label);
    bool IsSomeLabelSelected() const noexcept;

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
    uint64_t    idxOfSelectedMsg        = NOTSELECTED;
    LineSeries *seriesOwningSelectedMsg = nullptr;

    QCheckBox  *autoRangeChBox = nullptr;
    ScrollBar  *hscroll        = nullptr;
    ScrollBar  *vscroll        = nullptr;
    QChart     *chart          = nullptr;
    QChartView *chview         = nullptr;
    QValueAxis *vaxis          = nullptr;
    QValueAxis *haxis          = nullptr;

    std::map<int, std::pair<LineSeries *, std::vector<std::pair<qreal, std::shared_ptr<ArincMsg>>>>> labelsSeries;

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
