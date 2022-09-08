#include <QPainter>
#include <QScrollBar>
#include <QDateTime>

#include "_ArincChartView.h"
#include "_ArincLabelModel.hpp"
#include "arinc.hpp"

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
{ }

void
_ArincChartView::rowsInserted(const QModelIndex &parent, int start, int end)
{ }

void
_ArincChartView::setModel(QAbstractItemModel *model)
{
    if (model == nullptr)
        return;

    connect(dynamic_cast<_ArincLabelModel *>(model),
            &_ArincLabelModel::ArincMsgInserted,
            this,
            &_ArincChartView::ArincMsgInserted);

    QAbstractItemView::setModel(model);
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
    //test 2
    //QPainter painter(viewport());
    //painter.setPen(Qt::blue);
    //painter.setFont(QFont("Arial", 60));
    //painter.drawText(rect(), Qt::AlignCenter, "dupa");
    //end test 2

    chart->HandlePaintEvent(event);
}

void
_ArincChartView::resizeEvent(QResizeEvent *event)
{ 
    chart->Resize(event->size());
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
        auto img      = dynamic_cast<_ArincLabelModel *>(model())->GetLabelMarker(msg->labelRaw);
        auto newlabel = std::make_shared<ViewArincLabel>(msg->labelRaw, img);   // produces error
    }
    auto xpos =
      ConvertTimeDifferenceToXPos(dynamic_cast<_ArincLabelModel *>(model())->GetStartTime().msecsTo(msg->timeArrivalPC));

    labelsContainer.AppendMsg(msg, xpos);
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
                    visible_label->AppendMsg(item);
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
ViewArincLabels::AppendMsg(auto msg, auto xpos)
{
    auto [is_found, label] = GetLabel(msg->labelRaw);

    if (not is_found)
        return false;

    label->AppendMsg(std::make_shared<ViewArincLabelItem>(msg, xpos));

    return true;
}
