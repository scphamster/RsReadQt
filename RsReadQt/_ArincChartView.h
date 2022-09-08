#pragma once
#include <type_traits>

#include <QDateTime>
#include <QPointF>
#include <QImage>

#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QLineSeries>

#include <QAbstractItemView>

#include "arinc_model_configs.hpp"

class ArincMsg;
class QDateTime;
class _ArincChartView;

using XPositionT = double;

struct ViewArea {
  public:
    LabelNumT  lowestLabel;
    LabelNumT  highestLabel;
    XPositionT leftBorder;
    XPositionT rightBorder;
};

class ViewArincLabelItem {
  public:
    ViewArincLabelItem() = default;
    explicit ViewArincLabelItem(std::shared_ptr<ArincMsg> msg, XPositionT position)
      : arincMsg{ msg }
      , xPosition{ position }
    { }

    auto GetPosition() const noexcept { return xPosition; }
    auto GetMsg() const noexcept(std::is_nothrow_constructible_v<decltype(arincMsg)>) { return arincMsg; }

  private:
    XPositionT                xPosition;
    std::shared_ptr<ArincMsg> arincMsg;
};

class ViewArincLabel {
  public:
    enum {
        Undefined = -1
    };

    ViewArincLabel() = default;

    explicit ViewArincLabel(LabelNumT labelnum = static_cast<LabelNumT>(Undefined), QImage marker = QImage{})
      : labelNumber{ labelnum }
      , labelMarker{ marker }
    { }

    auto GetMarkerImage() const noexcept { return labelMarker; }
    auto GetLabelNumber() const noexcept { return labelNumber; }

    auto AppendMsg(auto msg) { items.push_back(msg); }

    decltype(auto) begin() const noexcept(noexcept(std::declval<decltype(items)>().begin())) { return items.begin(); }
    decltype(auto) end() const noexcept(noexcept(std::declval<decltype(items)>().end())) { return items.end(); }
    decltype(auto) begin() noexcept(noexcept(std::declval<decltype(items)>().begin())) { return items.begin(); }
    decltype(auto) end() noexcept(noexcept(std::declval<decltype(items)>().end())) { return items.end(); }
    decltype(auto) size() const noexcept(noexcept(std::declval<decltype(items)>().size())) { return items.size(); }

  private:
    LabelNumT                                        labelNumber = Undefined;
    QImage                                           labelMarker;
    std::vector<std::shared_ptr<ViewArincLabelItem>> items;
};

class ViewArincLabels {
  public:
    auto               GetItemAtPoint(QPointF p);
    auto               InsertItemAtPoint(QPointF p);
    [[nodiscard]] auto GetVisibleItemsAtArea(auto area);
    [[nodiscard]] auto GetLabel(LabelNumT label);
    auto               AppendLabel(auto label) { labels.push_back(label); }
    bool AppendMsg(auto msg, auto xpos);   // check if label exists, if no -> create new // else append to label
    [[nodiscard]] auto ContainsLabel(auto label) const
    {
        return std::any_of(labels.begin(), labels.end(), [&label](const auto &viewlabel) {
            if (viewlabel->GetLabelNumber() == label)
                return true;
            else
                return false;
        });
    }

    decltype(auto) begin() const noexcept(noexcept(std::declval<decltype(labels)>().begin())) { return labels.begin(); }
    decltype(auto) end() const noexcept(noexcept(std::declval<decltype(labels)>().end())) { return labels.end(); }
    decltype(auto) begin() noexcept(noexcept(std::declval<decltype(labels)>().begin())) { return labels.begin(); }
    decltype(auto) end() noexcept(noexcept(std::declval<decltype(labels)>().end())) { return labels.end(); }

  private:
    std::vector<std::shared_ptr<ViewArincLabel>> labels;
};

class DrawerQChart {
  public:
    explicit DrawerQChart(_ArincChartView *view, QWidget *viewport)
      : arincView{ view }
      , viewPort{ viewport }
    {
        // test
        chart  = new QChart{};
        chview = new QChartView{ chart, viewPort };
        chart->setParent(chview);
        chview->resize(QSize{ 600, 600 });
        // chart->setTitle("Labels");
        chart->legend()->setVisible(false);

        chart->setTheme(QChart::ChartTheme::ChartThemeBrownSand);
        // end test
    }

  protected:
    void PointOnChartBeenSelected(QPoint);
    void PointOnChartBeenDeselected(QPoint);

  private:
    QWidget         *viewPort  = nullptr;
    _ArincChartView *arincView = nullptr;

    QChart     *chart  = nullptr;
    QChartView *chview = nullptr;
    QValueAxis *vaxis  = nullptr;
    QValueAxis *haxis  = nullptr;
};

template<typename ImageType>
class LabelDrawer {
  public:
    virtual ~LabelDrawer() = default;

    virtual void             Init()                       = 0;
    virtual void             SetParent(_ArincChartView *) = 0;
    virtual void             SetViewPort(QWidget *viewport) { viewPort = viewport; }
    virtual void             AddLabel(LabelNumT label, const ImageType &msg_marker) = 0;
    virtual void             AppendLabelMsg(std::shared_ptr<ArincMsg> msg)          = 0;
    virtual void             SetAutoScrollToLatestMsg(bool set_to_auto = true)      = 0;
    virtual void             HandlePaintEvent(QPaintEvent *event){};
    virtual void             Resize(const QSize &newsize)             = 0;
    virtual void             OnLabelOnChartSelected(const QPointF &)  = 0;
    virtual bool             GetDataFromLabelOnChart(const QPointF &) = 0;
    virtual int              GetIdxOfSelectedMessage()                = 0;
    virtual const ImageType &GetLabelMarker(int label)                = 0;
    virtual bool             IsSomeLabelSelected()                    = 0;

  protected:
    // void resizeEvent(QResizeEvent *evt) override;
    // void wheelEvent(QWheelEvent *evt) override;
    // void _AdjustAxisToScroll(QValueAxis *axis, ScrollBar *scroll, int value);
    // void _AdjustScrollToAxis(ScrollBar *scroll, qreal min, qreal max);

    QWidget *viewPort = nullptr;

    // signals:
    //   void MsgOnChartBeenSelected(uint64_t msgN);

    // public slots:
    //   void SetLabelVisibility(int label, Qt::CheckState checkstate);

  private:
    // bool eventFilter(QObject *obj, QEvent *evt) override;
};

template<typename ImageType>
class QChartLabelDrawer : public LabelDrawer<ImageType> {
  public:
    ~QChartLabelDrawer(){};
    void AddLabel(LabelNumT label, const ImageType &msg_marker) override { }
    void AppendLabelMsg(std::shared_ptr<ArincMsg> msg) override { }
    void SetAutoScrollToLatestMsg(bool set_to_auto = true) override { }
    void Init() override
    {
        // test
        chart  = new QChart{};
        chview = new QChartView{ chart, LabelDrawer<ImageType>::viewPort };
        chart->setParent(chview);
        chview->resize(
          QSize{ LabelDrawer<ImageType>::viewPort->size().width(), LabelDrawer<ImageType>::viewPort->size().height() });
        // chart->setTitle("Labels");
        chart->legend()->setVisible(false);

        chart->setTheme(QChart::ChartTheme::ChartThemeBrownSand);
        // end test
    }
    void             SetParent(_ArincChartView *new_parent){ parent = new_parent };
    void             Resize(const QSize &newsize) override { chview->resize(newsize); }
    int              GetIdxOfSelectedMessage() override { return idxOfSelectedMsg; }
    void             OnLabelOnChartSelected(const QPointF &) override;
    void             SetAutoScrollToLatestMsg(bool set_to_auto = true);
    bool             GetDataFromLabelOnChart(const QPointF &) override;
    const ImageType &GetLabelMarker(int label) override;
    bool             IsSomeLabelSelected() override;

  private:
    QChart          *chart  = nullptr;
    QChartView      *chview = nullptr;
    QValueAxis      *vaxis  = nullptr;
    QValueAxis      *haxis  = nullptr;
    _ArincChartView *parent = nullptr;
};

class _ArincChartView : public QAbstractItemView {
    Q_OBJECT
  public:
    template<typename ImageType>
    explicit _ArincChartView(std::unique_ptr<LabelDrawer<ImageType>> drawer, ::QWidget *parent = nullptr)
      : chart{ std::move(drawer) }
      , QAbstractItemView{ parent }
    {
        chart->SetViewPort(viewport());
        chart->Init();
    }

    QRect       visualRect(const QModelIndex &index) const override;
    void        scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint &point) const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    int         horizontalOffset() const override;
    int         verticalOffset() const override;
    bool        isIndexHidden(const QModelIndex &index) const override;
    void        setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion     visualRegionForSelection(const QItemSelection &selection) const override;

    void dataChanged(const QModelIndex &topLeft,
                     const QModelIndex &bottomRight,
                     const QList<int>  &roles = QList<int>()) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void setModel(QAbstractItemModel *model) override;

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    [[nodiscard]] auto GetVisibleArea() const;
    [[nodiscard]] auto GetVisibleItems() const;
    auto               DrawLabelMarkerAt(QPointF point) const;
    auto               DrawLabelsAtArea(QRectF area) const;

    auto ConvertTimeDifferenceToXPos(auto msecs_from_start) const noexcept { return msecs_from_start; }

  public slots:
    void ArincMsgInserted(std::shared_ptr<ArincMsg> msg);

  protected:
  private:
    QDateTime                            startOfOperation;
    ViewArincLabels                      labelsContainer;
    std::unique_ptr<LabelDrawer<QImage>> chart;
};
