#include <memory>
#include <algorithm>

#include <QDateTime>
#include <QTreeWidget>

#include "arinc_model_configs.hpp"
#include "arinc_label_model.hpp"
#include "arinc_label_item.hpp"
#include "child_container.hpp"
#include "proxy_arinc_label_item.hpp"
#include "arinc.hpp"

class ArincLabelModel2::Impl {
  public:
    ProxyArincLabelItem<1> *rootItem;

    QDateTime                             startOfOperation;
    QTreeWidgetItem                       header;
    Column                                sortedByColumn = Undefined;
    std::underlying_type_t<Qt::SortOrder> sortOrder      = Qt::SortOrder::AscendingOrder;
};

ArincLabelModel2::ArincLabelModel2()
  : impl{ std::make_unique<Impl>() }
{
    impl->rootItem = new ProxyArincLabelItem{ std::make_shared<ArincData>() };

    using Column    = ArincTreeData::ColumnRole;
    auto horizontal = Qt::Horizontal;

    setHeaderData(static_cast<int>(Column::LabelNum), horizontal, "Label");
    setHeaderData(static_cast<int>(Column::FirstOccurrence), horizontal, "FirstOccurrence");
    setHeaderData(static_cast<int>(Column::LastOccurrence), horizontal, "LastOccurrence");
    setHeaderData(static_cast<int>(Column::HitCount), horizontal, "HitCount");
    setHeaderData(static_cast<int>(Column::MakeBeep), horizontal, "MakeBeep");
    setHeaderData(static_cast<int>(Column::ShowOnDiagram), horizontal, "ShowOnDiagram");
    setHeaderData(static_cast<int>(Column::Hide), horizontal, "Hide");

    impl->startOfOperation = QDateTime::currentDateTime();
}

ArincLabelModel2::~ArincLabelModel2() = default;

QModelIndex
ArincLabelModel2::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (not hasIndex(row, column, parent))
        return QModelIndex{};

    auto parent_item = ItemFromIndex(parent);

    return createIndex(row, column, static_cast<void *>(parent_item->GetChild(row, column)));
}

QModelIndex
ArincLabelModel2::parent(const QModelIndex &child) const
{
    if (not child.isValid())
        return QModelIndex{};

    auto child_item = static_cast<ProxyArincLabelItem<1> *>(child.internalPointer());
    if (child_item == nullptr)
        return QModelIndex{};

    auto parent_item = child_item->GetParent();
    if (parent_item == impl->rootItem || parent_item == nullptr)
        return QModelIndex{};

    return createIndex(parent_item->GetRow(), parent_item->GetColumn(), parent_item);
}

int
ArincLabelModel2::rowCount(const QModelIndex &parent /*= QModelIndex{}*/) const
{
    ProxyArincLabelItem<1> *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer()) == nullptr)
                        ? impl->rootItem
                        : static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer());
    else
        parent_item = impl->rootItem;

    return parent_item->GetChildrenCount();
}

int
ArincLabelModel2::columnCount(const QModelIndex &parent /*= QModelIndex{}*/) const
{
    ProxyArincLabelItem<1> *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer()) == nullptr)
                        ?impl->rootItem
                        : static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer());
    else
        parent_item =impl->rootItem;
    return parent_item->GetColumnCount();
}

QVariant
ArincLabelModel2::data(const QModelIndex &index, int role) const
{
    return GetDataForStandardViews(index, role);

    // if ((role == Qt::ItemDataRole::DisplayRole or role == Qt::EditRole) and (index.isValid()))
    // else
    //     return QVariant{};
}

QVariant
ArincLabelModel2::GetDataForStandardViews(const QModelIndex &index, int role) const
{
    // for QTreeView only column is changing, row is always 0, item is indicated by internal pointer in "index"
    auto item = index.isValid() ? static_cast<ProxyArincLabelItem<1> *>(index.internalPointer()) :impl->rootItem;
    if (item == nullptr)
        return 0;

    return item->GetData(index.column(), role);
}

bool
ArincLabelModel2::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // todo: make path for my view
    // this is the path for QTreeView

    // index contains item and column if column count is insufficient -> enlarge
    auto item = ItemFromIndex(index);

    item->SetData(index.column(), role, value);
    emit dataChanged(index, index, QList<int>{ role });
    return true;
}

bool
ArincLabelModel2::setHeaderData(int             at_pos,
                                Qt::Orientation orientation,
                                const QVariant &value,
                                int             role /*= Qt::EditRole*/)
{
    impl->header.setData(at_pos, role, value);
    emit headerDataChanged(orientation, at_pos, at_pos);
    return true;
}

QVariant
ArincLabelModel2::headerData(int at_pos, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    return impl->header.data(at_pos, role);
}

Qt::ItemFlags
ArincLabelModel2::flags(const QModelIndex &index) const
{
    auto item = ItemFromIndex(index);

    return item->GetFlags();
}

void
ArincLabelModel2::Insert(size_t row, ProxyArincLabelItem<1> *newitem)
{
    if (newitem == nullptr)
        return;

    auto parent_item = (newitem->GetParent() == nullptr) ?impl->rootItem : newitem->GetParent();

    if (parent_item->InsertChild(newitem, row)) {
        emit layoutAboutToBeChanged({}, QAbstractItemModel::LayoutChangeHint::NoLayoutChangeHint);

        auto index_which_was_changed = createIndex(newitem->GetRow(), newitem->GetColumn(), newitem);

        emit layoutChanged({}, QAbstractItemModel::NoLayoutChangeHint);
        emit dataChanged(index_which_was_changed, index_which_was_changed, QList<int>{ Qt::ItemDataRole::DisplayRole });
    }
}

// todo: bad behaviour, remake
void
ArincLabelModel2::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    auto changed =impl->rootItem->SortChildren(column, order);

    QList<QModelIndex> fromList;
    for (auto &row_item_pair : changed.first) {
        for (auto col = 0; col < columnCount(); col++) {
            fromList << createIndex(row_item_pair.first, col, row_item_pair.second);
        }
    }

    QList<QModelIndex> toList;
    for (auto &row_item_pair : changed.second) {
        for (auto col = 0; col < columnCount(); col++) {
            toList << createIndex(row_item_pair.first, col, row_item_pair.second);
        }
    }

    changePersistentIndexList(fromList, toList);   // no impact observed so far

    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);

    impl->sortOrder = order;
    impl->sortedByColumn = column;
}

QImage
ArincLabelModel2::GetLabelMarker(LabelNumT label) const noexcept(std::is_nothrow_constructible_v<QImage>)
{
    return impl->rootItem->GetLabelMarker(label);
}

[[nodiscard]] ProxyArincLabelItem<1> *
ArincLabelModel2::ItemFromIndex(const QModelIndex &idx) const
{
    auto item = idx.isValid() ? static_cast<ProxyArincLabelItem<1> *>(idx.internalPointer()) :impl->rootItem;
    return (item == nullptr) ?impl->rootItem : item;
}

void
ArincLabelModel2::InsertNewMessage(std::shared_ptr<ArincMsg> msg)
{
    if (not impl->rootItem->Contains(msg->GetLabel<_ArincLabel>())) {
        Insert(0, new ProxyArincLabelItem<1>{ std::make_shared<ArincData>(msg->channel, msg->_label),impl->rootItem });
    }

    auto row =impl->rootItem->AppendMessage(msg, msg->_label);

    auto changed_index = createIndex(static_cast<int>(row), static_cast<int>(Parameter::ArincMsgs), nullptr);
    emit dataChanged(changed_index, changed_index, QList<int>{ Qt::ItemDataRole::DisplayRole });
    emit ArincMsgInserted(msg);

    if (impl->sortedByColumn != Undefined)
        sort(impl->sortedByColumn, static_cast<Qt::SortOrder>(impl->sortOrder));
}

const QDateTime &
ArincLabelModel2::GetStartTime() const noexcept(std::is_nothrow_constructible_v<QDateTime>)
{
    return impl->startOfOperation;
}

void
ArincLabelModel2::SetArincMsgSelection(LabelNumT label, std::shared_ptr<ArincMsg> msg)
{ }