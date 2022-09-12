#include <functional>
#include <QPainter>
#include <QScrollBar>
#include <QDateTime>

#include "_ArincChartView.h"
#include "arinc_label_model.hpp"
#include "arinc_label_item.hpp"
#include "child_container.hpp"
#include "proxy_arinc_label_item.hpp"
#include "arinc.hpp"
#include "line_series.hpp"

#pragma region _ArincChartView
QRect
_ArincChartView::visualRect(const QModelIndex &index) const
{
    return QRect();
}

void
_ArincChartView::scrollTo(const QModelIndex &index, ScrollHint hint)
{ }

QModelIndex
_ArincChartView::indexAt(const QPoint &point) const
{
    return QModelIndex();
}

QModelIndex
_ArincChartView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    return QModelIndex();
}

int
_ArincChartView::horizontalOffset() const
{
    return 0;
}

int
_ArincChartView::verticalOffset() const
{
    return 0;
}

bool
_ArincChartView::isIndexHidden(const QModelIndex &index) const
{
    return false;
}

void
_ArincChartView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{ }

QRegion
_ArincChartView::visualRegionForSelection(const QItemSelection &selection) const
{
    return QRegion();
}

void
_ArincChartView::dataChanged(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight,
                             const QList<int>  &roles /*= QList<int>()*/)
{
    // if index is at visibility checkbox proceed otherwise return;

    if (topLeft.column() == static_cast<Column>(ArincTreeData::ColumnRole::Hide)) {
        // find label number of index
        auto label_item = static_cast<ProxyArincLabelItem<1> *>(topLeft.internalPointer());
        if (label_item == nullptr)
            return;

        auto label_is_hiden_CheckboxStatus =
          label_item->GetData(static_cast<Column>(ArincTreeData::ColumnRole::Hide), Qt::ItemDataRole::CheckStateRole);

        constexpr bool invisible = false, visible = true;
        if (label_is_hiden_CheckboxStatus == Qt::Checked)
            chart->SetLabelVisibility(label_item->GetArincData()->GetLabel<LabelNumT>(), invisible);
        else
            chart->SetLabelVisibility(label_item->GetArincData()->GetLabel<LabelNumT>(), visible);
    }
}

void
_ArincChartView::rowsInserted(const QModelIndex &parent, int start, int end)
{ }

void
_ArincChartView::setModel(QAbstractItemModel *newmodel)
{
    if (newmodel == nullptr)
        return;

    connect(dynamic_cast<ArincLabelModel2 *>(newmodel),
            &ArincLabelModel2::ArincMsgInserted,
            this,
            &_ArincChartView::ArincMsgInserted);

    QAbstractItemView::setModel(newmodel);
    arincModel = dynamic_cast<ArincLabelModel2 *>(newmodel);
}

void
_ArincChartView::paintEvent(QPaintEvent *event)
{
    /*
    *
    //PAINTER SETUP
    QItemSelectionModel *selections = selectionModel();
    QStyleOptionViewItem option;
    initViewItemOption(&option);

    QBrush background = option.palette.base();
    QPen   foreground(option.palette.color(QPalette::WindowText));

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(event->rect(), background);
    painter.setPen(foreground);

    //DRAWING
    DrawLabelsAtArea() {
        //GET ALL ITEMS LAYING IN AREA
        auto labels_to_be_drawn = labelsContainer.GetVisibleItems(GetVisibleArea());

        for(const auto &label : labels_to_be_drawn) {
            for(const auto &item : label) {



                auto point = item.GetPoint();
                auto image = item.GetImage();
                DrawItem(point, image);
            }
        }

        //DRAW
        for (const auto &label : items_to_be_drawn) {
            auto point = ConvertToPoint(label, Xposiiton);
            auto marker = GetMarkerForLabel(label);
            for (const auto &item : label) {
                DrawLabelMarkerAt(item.first, marker);
            }
        }

    }
    */

    /*
    auto visible_items = GetVisibleItems();
    canvas.AdjustVisibleAreaForLatestItems(visible_items);
    canvas.DrawItems(visible_items);
    */
    // test 2
    // QPainter painter(viewport());
    // painter.setPen(Qt::blue);
    // painter.setFont(QFont("Arial", 60));
    // painter.drawText(rect(), Qt::AlignCenter, "dupa");
    // end test 2

    chart->HandlePaintEvent(event);
}

void
_ArincChartView::resizeEvent(QResizeEvent *event)
{
    chart->Resize(event->size());
}

[[nodiscard]] const QDateTime &
_ArincChartView::GetStartOfOperation() const noexcept
{
    return arincModel->GetStartTime();
}

[[nodiscard]] ImageType
_ArincChartView::GetLabelImage(LabelNumT label) const noexcept(std::is_nothrow_constructible_v<ImageType, ImageType &>)
{
    return arincModel->GetLabelMarker(label);
}

auto
_ArincChartView::GetVisibleArea() const
{
    return viewport()->geometry();
}

void
_ArincChartView::ArincMsgInserted(std::shared_ptr<ArincMsg> msg)
{
    if (not labelsContainer.ContainsLabel(msg->labelRaw)) {
        auto img      = dynamic_cast<ArincLabelModel2 *>(model())->GetLabelMarker(msg->labelRaw);
        auto newlabel = std::make_shared<ViewArincLabel>(msg->labelRaw, img);   // produces error
        chart->AddLabel(msg->labelRaw, img);
    }
    auto xpos =
      ConvertTimeDifferenceToXPos(dynamic_cast<ArincLabelModel2 *>(model())->GetStartTime().msecsTo(msg->timeArrivalPC));

    labelsContainer.InsertMsg(msg, xpos);
    chart->AppendLabelMsg(msg);
}

[[nodiscard]] auto
ViewArincLabels::GetVisibleItemsAtArea(auto area)
{
    auto LabelIsWithinArea = [area](const auto &label) {
        if (label->GetLabelNumber() > area.bot and label->GetLabelNumber() < area.top) {
            return true;
        }
        else
            return false;
    };

    auto ItemIsWithinArea = [area](const auto &item) {
        if (item->GetPosition() > area.left and item->GetPosition() < area.right)
            return true;
        else
            return false;
    };

    auto visible_labels = ViewArincLabels{};

    for (const auto &label : labels) {
        if (LabelIsWithinArea(label)) {
            auto visible_label = std::make_shared<ViewArincLabel>();

            for (const auto &item : label) {
                if (ItemIsWithinArea(item)) {
                    visible_label->InsertMsg(item);
                }
            }

            if (visible_label->size() > 0)
                visible_labels.AppendLabel(visible_label);
        }
    }

    return visible_labels;
}

[[nodiscard]] auto
ViewArincLabels::GetLabel(LabelNumT label)
{
    auto label_it = std::find_if(labels.begin(), labels.end(), [&wanted_label = label](const auto &label) {
        if (wanted_label == label->GetLabelNumber())
            return true;
        else
            return false;
    });

    if (label_it == labels.end())
        return std::tuple{ false, std::shared_ptr<ViewArincLabel>{} };

    return std::tuple{ true, *label_it };
}

bool
ViewArincLabels::InsertMsg(auto msg, auto xpos)
{
    auto [is_found, label] = GetLabel(msg->labelRaw);

    if (not is_found)
        return false;

    label->InsertMsg(std::make_shared<ViewArincLabelItem>(msg, xpos));

    return true;
}
#pragma endregion _ArincChartView

#pragma region LabelDrawer
class QChartLabelDrawerData {
  public:
    using SecondsT                = double;
    using PosX                    = SecondsT;
    using PointF                  = QPointF;
    using MsgIdxT                 = int;
    using SuccessT                = bool;
    using OpResult_CallbackF      = std::pair<SuccessT, QChartLabelDrawer::DeselectionCallbackF>;
    using OpReslt_MsgIdx_ArincMsg = std::tuple<SuccessT, MsgIdxT, std::shared_ptr<ArincMsg>>;

    explicit QChartLabelDrawerData(LabelNumT, const ImageType &, QChart *, QChartLabelDrawer *new_parent = nullptr);

    void               SetParent(QChartLabelDrawer *new_parent) noexcept { myParent = new_parent; }
    OpResult_CallbackF SetMsgAtPointToSelectedState(PosX);
    void               InsertMsg(PointF at_point, std::shared_ptr<ArincMsg>);
    void               CreateSeries(const ImageType &img);

    OpReslt_MsgIdx_ArincMsg GetMsgFromPoint(PosX) const;
    void                    SetVisibility(bool make_visible = true);

  protected:
    void SetMsgAtIdxToSelectedState(MsgIdxT idx);

  private:
    LabelNumT                                               label    = -1;
    QChart                                                 *chart    = nullptr;
    QChartLabelDrawer                                      *myParent = nullptr;
    LineSeries                                             *series;
    std::vector<std::pair<PosX, std::shared_ptr<ArincMsg>>> messages;
};

QChartLabelDrawerData::QChartLabelDrawerData(LabelNumT          labelNum,
                                             const ImageType   &img,
                                             QChart            *newchart,
                                             QChartLabelDrawer *new_parent)
  : label{ labelNum }
  , chart{ newchart }
  , myParent{ new_parent }
{
    CreateSeries(img);

    chart->addSeries(series);
    series->attachAxis(chart->axisX());
    series->attachAxis(chart->axisY());
}
void
QChartLabelDrawerData::SetMsgAtIdxToSelectedState(MsgIdxT idx)
{
    series->selectPoint(idx);
}

QChartLabelDrawerData::OpResult_CallbackF
QChartLabelDrawerData::SetMsgAtPointToSelectedState(PosX at_point)
{
    constexpr bool failed = false, succeed = true;

    auto [msg_is_found, msg_idx, msg] = GetMsgFromPoint(at_point);

    if (not msg_is_found)
        return { failed, []() { ; } };

    SetMsgAtIdxToSelectedState(msg_idx);

    return { succeed, [series = this->series, msg]() {
                series->deselectAllPoints();
                msg->SetSelection(false);
            } };
}

void
QChartLabelDrawerData::InsertMsg(PointF at_point, std::shared_ptr<ArincMsg> msg)
{
    assert(at_point.y() == label);

    series->append(at_point);
    messages.push_back({ at_point.x(), msg });
}

// TODO: make configurations of image sizes packed to json file
void
QChartLabelDrawerData::CreateSeries(const ImageType &img)
{
    series = new LineSeries{ chart };

    series->setLightMarker(img);
    series->setMarkerSize(40);
    series->SetMarkerVisibility(true);

    series->setPen(QPen{ QColor{ 0, 0, 0, 0 } });

    auto     selected_marker = QImage{ img };
    QPainter painter{ &selected_marker };

    painter.setBrush(QBrush{ QColor{ 0, 0, 0, 0 } });
    constexpr double testwidth  = 50;
    constexpr double testradius = 25;
    painter.setPen(QPen{ QBrush{ QColor{ 255, 0, 0, 255 } }, testwidth });
    painter.drawRoundedRect(QRect{ QPoint{ 0, 0 }, QSize{ img.width(), img.height() } }, testradius, testradius);

    series->setSelectedLightMarker(std::move(selected_marker));

    QObject::connect(series, &QXYSeries::pressed, myParent, &QChartLabelDrawer::OnMsgOnChartBeenSelected);
}

QChartLabelDrawerData::OpReslt_MsgIdx_ArincMsg
QChartLabelDrawerData::GetMsgFromPoint(PosX at_point) const
{
    constexpr bool failed = false, succeed = true;
    auto           msg_idx = MsgIdxT{ 0 };

    for (const auto &pos_and_msg : messages) {
        if (pos_and_msg.first == at_point) {
            pos_and_msg.second->SetSelection(true);
            return std::make_tuple(succeed, msg_idx, pos_and_msg.second);
        }

        msg_idx++;
    }

    return std::make_tuple(failed, 0, std::shared_ptr<ArincMsg>{});
}

void
QChartLabelDrawerData::SetVisibility(bool make_visible)
{
    series->SetMarkerVisibility(make_visible);
}

QChartLabelDrawer::QChartLabelDrawer()  = default;
QChartLabelDrawer::~QChartLabelDrawer() = default;

void
QChartLabelDrawer::AddLabel(LabelNumT label, const ImageType &msg_marker)
{
    labels.insert({ label, std::make_unique<QChartLabelDrawerData>(label, msg_marker, chart, this) });
}

void
QChartLabelDrawer::AppendLabelMsg(std::shared_ptr<ArincMsg> msg)
{
    if (not labels.contains(msg->labelRaw))
        AddLabel(msg->labelRaw, myParent->GetLabelImage(msg->labelRaw));

    auto insertion_point =
      PointF{ static_cast<qreal>(myParent->GetStartOfOperation().msecsTo(msg->GetTimeArival())) / 1000,
              static_cast<qreal>(msg->labelRaw) };

    AdjustAxles(insertion_point);
    labels.at(msg->labelRaw)->InsertMsg(insertion_point, msg);
}

void
QChartLabelDrawer::Init()
{
    viewPort = myParent->viewport();

    // test
    chart = new QChart{};
    // chview = new QChartView{ chart, dynamic_cast<QWidget *>(myMainWindow) };
    chview = new QChartView{ chart, dynamic_cast<QWidget *>(myParent) };
    chart->setParent(chview);
    chview->resize(QSize{ viewPort->size().width(), viewPort->size().height() });
    // chart->setTitle("Labels");
    chart->legend()->setVisible(false);

    chart->setTheme(QChart::ChartTheme::ChartThemeBrownSand);

    vaxis = new QValueAxis{ chart };
    haxis = new QValueAxis{ chart };
    vaxis->setTickCount(10);
    vaxis->setRange(0, 255);
    haxis->setTickCount(10);
    haxis->setRange(0, 20);
    chart->addAxis(vaxis, Qt::AlignLeft);
    chart->addAxis(haxis, Qt::AlignBottom);

    chview->installEventFilter(this);
    // end test
}

void
QChartLabelDrawer::OnLabelOnChartSelected(const QPointF &)
{
    // label, msg -> modelSetSelectionOfMsg(label, msg);
}

bool
QChartLabelDrawer::GetDataFromLabelOnChart(const QPointF &)
{
    return false;
}

void
QChartLabelDrawer::SetLabelVisibility(LabelNumT label, bool make_visible)
{
    if (not labels.contains(label))
        return;

    labels.at(label)->SetVisibility(make_visible);
}

void
QChartLabelDrawer::UnselectAll()
{
    if (handleOfSelectedMsg.first == true) {
        handleOfSelectedMsg.second();
        handleOfSelectedMsg.first = false;
    }
}

// TODO: add json file for behaviour configuration
void
QChartLabelDrawer::AdjustAxles(PointF to_accommodate_point)
{
    constexpr double test_autoscrollovershootPercent        = 0.4;
    constexpr double test_triggerautoscrollovershootPercent = 0.2;

    if (auto haxis_width = haxis->max() - haxis->min();
        to_accommodate_point.x() > (haxis->max() - haxis_width * test_triggerautoscrollovershootPercent)) {
        haxis->setMax(to_accommodate_point.x() + haxis_width * test_autoscrollovershootPercent);
        haxis->setMin(haxis->max() - haxis_width);
    }

    constexpr double test_verticalOverheadNeeded = 50;
    constexpr double test_autorerangeovershoot   = 60;

    if (to_accommodate_point.y() > (vaxis->max() - test_verticalOverheadNeeded)) {
        vaxis->setMax(to_accommodate_point.y() + test_autorerangeovershoot);
    }
}

bool
QChartLabelDrawer::eventFilter(QObject *obj, QEvent *evt)
{
    constexpr bool propagate_event_further = false, do_not_propagate_event = true;

    if (evt->type() == QEvent::Type::MouseButtonPress) {
        UnselectAll();
        return do_not_propagate_event;
    }

    return propagate_event_further;
}

void
QChartLabelDrawer::OnMsgOnChartBeenSelected(const PointF &at_point)
{
    auto label = static_cast<LabelNumT>(at_point.y());
    if (not labels.contains(label))
        return;

    UnselectAll();

    handleOfSelectedMsg = labels.at(label)->SetMsgAtPointToSelectedState(at_point.x());
}

#pragma endregion LabelDrawer