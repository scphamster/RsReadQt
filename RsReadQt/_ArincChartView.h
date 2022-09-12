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
#include "arinc_chart.hpp"

class ArincMsg;
class QDateTime;
class QMainWindow;
class QChartLabelDrawerData;
class QChartLabelDrawer;
class _ArincChartView;
class ArincLabelModel2;

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

    explicit ViewArincLabel(LabelNumT labelnum = static_cast<LabelNumT>(Undefined), ImageType marker = ImageType{})
      : labelNumber{ labelnum }
      , labelMarker{ marker }
    { }

    auto GetMarkerImage() const noexcept { return labelMarker; }
    auto GetLabelNumber() const noexcept { return labelNumber; }

    auto InsertMsg(auto msg) { items.push_back(msg); }

    decltype(auto) begin() const noexcept(noexcept(std::declval<decltype(items)>().begin())) { return items.begin(); }
    decltype(auto) end() const noexcept(noexcept(std::declval<decltype(items)>().end())) { return items.end(); }
    decltype(auto) begin() noexcept(noexcept(std::declval<decltype(items)>().begin())) { return items.begin(); }
    decltype(auto) end() noexcept(noexcept(std::declval<decltype(items)>().end())) { return items.end(); }
    decltype(auto) size() const noexcept(noexcept(std::declval<decltype(items)>().size())) { return items.size(); }

  private:
    LabelNumT                                        labelNumber = Undefined;
    ImageType                                        labelMarker;
    std::vector<std::shared_ptr<ViewArincLabelItem>> items;
};

class ViewArincLabels {
  public:
    auto               GetItemAtPoint(QPointF p);
    auto               InsertItemAtPoint(QPointF p);
    [[nodiscard]] auto GetVisibleItemsAtArea(auto area);
    [[nodiscard]] auto GetLabel(LabelNumT label);
    auto               AppendLabel(auto label) { labels.push_back(label); }
    bool InsertMsg(auto msg, auto xpos);   // check if label exists, if no -> create new // else append to label
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

class LabelDrawer : public QObject {
  public:
    virtual ~LabelDrawer() = default;

    virtual void                      Init()                                                 = 0;
    virtual void                      SetParent(_ArincChartView *)                           = 0;
    virtual void                      SetUltraParent(QWidget *parent)                        = 0;
    virtual void                      AddLabel(LabelNumT label, const ImageType &msg_marker) = 0;
    virtual void                      AppendLabelMsg(std::shared_ptr<ArincMsg> msg)          = 0;
    virtual void                      SetAutoScrollToLatestMsg(bool set_to_auto = true)      = 0;
    virtual void                      HandlePaintEvent(QPaintEvent *event){};
    virtual void                      Resize(const QSize &newsize)             = 0;
    virtual void                      OnLabelOnChartSelected(const QPointF &)  = 0;
    virtual bool                      GetDataFromLabelOnChart(const QPointF &) = 0;
    virtual int                       GetIdxOfSelectedMessage()                = 0;
    virtual ImageType                 GetLabelMarker(int label)                = 0;
    virtual bool                      IsSomeLabelSelected()                    = 0;
    [[nodiscard]] virtual QChartView *GetChView() const noexcept               = 0;
    virtual void                      SetLabelVisibility(LabelNumT, bool)      = 0;

  protected:
  private:
};

// TODO: implement add label and appendLabelMsg
class QChartLabelDrawer : public LabelDrawer {
    Q_OBJECT

  public:
    using PointF               = QPointF;
    using DeselectionCallbackF = std::function<void()>;

    QChartLabelDrawer();
    ~QChartLabelDrawer();
    void                      SetParent(_ArincChartView *new_parent) override { myParent = new_parent; }
    void                      SetUltraParent(QWidget *parent) override { myMainWindow = parent; }
    void                      AddLabel(LabelNumT label, const ImageType &msg_marker) override;
    void                      AppendLabelMsg(std::shared_ptr<ArincMsg> msg) override;
    void                      SetAutoScrollToLatestMsg(bool set_to_auto = true) override { }   // unimpl
    void                      Init() override;
    void                      Resize(const QSize &newsize) override { chview->resize(newsize); }
    int                       GetIdxOfSelectedMessage() override { return 0; }             // unimpl
    void                      OnLabelOnChartSelected(const QPointF &) override;            // unimpl
    bool                      GetDataFromLabelOnChart(const QPointF &) override;           // unimpl
    ImageType                 GetLabelMarker(int label) override { return ImageType{}; }   // unimpl
    bool                      IsSomeLabelSelected() override { return false; }             // unimpl
    [[nodiscard]] QChartView *GetChView() const noexcept override { return chview; }
    void                      SetLabelVisibility(LabelNumT, bool make_visible = true) override;
    void                      UnselectAll();

  public slots:
    void OnMsgOnChartBeenSelected(const PointF &at_point);

  protected:
    void AdjustAxles(PointF to_accommodate_point);

  private:
    bool eventFilter(QObject *obj, QEvent *evt) override;

    std::map<LabelNumT, std::unique_ptr<QChartLabelDrawerData>> labels;
    _ArincChartView                                            *myParent     = nullptr;
    QWidget                                                    *myMainWindow = nullptr;
    QChart                                                     *chart        = nullptr;
    QChartView                                                 *chview       = nullptr;
    QValueAxis                                                 *vaxis        = nullptr;
    QValueAxis                                                 *haxis        = nullptr;
    QWidget                                                    *viewPort     = nullptr;

    std::pair<bool, DeselectionCallbackF> handleOfSelectedMsg;
};

class _ArincChartView : public QAbstractItemView {
    Q_OBJECT
  public:
    _ArincChartView(std::unique_ptr<LabelDrawer> drawer, ::QWidget *parent = nullptr)
      : chart{ std::move(drawer) }
      , QAbstractItemView{ parent }
    {
        chart->SetParent(this);
        chart->SetUltraParent(parent);
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

    [[nodiscard]] QDateTime const &GetStartOfOperation() const noexcept;
    [[nodiscard]] ImageType        GetLabelImage(LabelNumT label) const
      noexcept(std::is_nothrow_constructible_v<ImageType, ImageType &>);

    auto ConvertTimeDifferenceToXPos(auto msecs_from_start) const noexcept { return msecs_from_start; }
    
    [[nodiscard]] QChartView *GetChView() const noexcept { return chart->GetChView(); }

  public slots:
    void ArincMsgInserted(std::shared_ptr<ArincMsg> msg);

  protected:
  private:
    std::unique_ptr<LabelDrawer> chart;
    ViewArincLabels              labelsContainer;
    ArincLabelModel2            *arincModel = nullptr;
};
