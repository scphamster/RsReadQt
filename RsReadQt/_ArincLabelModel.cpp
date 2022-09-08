#include "_ArincLabelModel.hpp"
#include "arinc.hpp"

_ArincLabelModel::_ArincLabelModel()
{
    rootItem = new ProxyArincLabelItem{ std::make_shared<ArincData>() };

    using Column    = ArincTreeData::ColumnRole;
    auto horizontal = Qt::Horizontal;

    setHeaderData(static_cast<int>(Column::LabelNum), horizontal, "Label");
    setHeaderData(static_cast<int>(Column::FirstOccurrence), horizontal, "FirstOccurrence");
    setHeaderData(static_cast<int>(Column::LastOccurrence), horizontal, "LastOccurrence");
    setHeaderData(static_cast<int>(Column::HitCount), horizontal, "HitCount");
    setHeaderData(static_cast<int>(Column::MakeBeep), horizontal, "MakeBeep");
    setHeaderData(static_cast<int>(Column::ShowOnDiagram), horizontal, "ShowOnDiagram");
    setHeaderData(static_cast<int>(Column::Hide), horizontal, "Hide");
        
    startOfOperation = QDateTime::currentDateTime();
}

QModelIndex
_ArincLabelModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (not hasIndex(row, column, parent))
        return QModelIndex{};

    // if (column >= static_cast<int>(_ArincLabelItem::ArincRole::_SIZE))
    // return QModelIndex{};

    auto parent_item = ItemFromIndex(parent);

    return createIndex(row, column, parent_item->GetChild(row, column));
}

QModelIndex
_ArincLabelModel::parent(const QModelIndex &child) const
{
    if (not child.isValid())
        return QModelIndex{};

    auto child_item = static_cast<ProxyArincLabelItem<1> *>(child.internalPointer());
    if (child_item == nullptr)
        return QModelIndex{};

    auto parent_item = child_item->GetParent();
    if (parent_item == rootItem || parent_item == nullptr)
        return QModelIndex{};

    return createIndex(parent_item->GetRow(), parent_item->GetColumn(), parent_item);
}

int
_ArincLabelModel::rowCount(const QModelIndex &parent /*= QModelIndex{}*/) const
{
    ProxyArincLabelItem<1> *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer()) == nullptr)
                        ? rootItem
                        : static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer());
    else
        parent_item = rootItem;

    return parent_item->GetChildrenCount();
}

int
_ArincLabelModel::columnCount(const QModelIndex &parent /*= QModelIndex{}*/) const
{
    ProxyArincLabelItem<1> *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer()) == nullptr)
                        ? rootItem
                        : static_cast<ProxyArincLabelItem<1> *>(parent.internalPointer());
    else
        parent_item = rootItem;
    return parent_item->GetColumnCount();
}

QVariant
_ArincLabelModel::data(const QModelIndex &index, int role) const
{
    return GetDataForStandardViews(index, role);

    // if ((role == Qt::ItemDataRole::DisplayRole or role == Qt::EditRole) and (index.isValid()))
    // else
    //     return QVariant{};
}

QVariant
_ArincLabelModel::GetDataForStandardViews(const QModelIndex &index, int role) const
{
    // for QTreeView only column is changing, row is always 0, item is indicated by internal pointer in "index"
    auto item = index.isValid() ? static_cast<ProxyArincLabelItem<1> *>(index.internalPointer()) : rootItem;
    if (item == nullptr)
        return 0;

    return item->GetData(index.column(), role);
}

bool
_ArincLabelModel::setData(const QModelIndex &index, const QVariant &value, int role)
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
_ArincLabelModel::setHeaderData(int             at_pos,
                                Qt::Orientation orientation,
                                const QVariant &value,
                                int             role /*= Qt::EditRole*/)
{
    header.setData(at_pos, role, value);
    emit headerDataChanged(orientation, at_pos, at_pos);
    return true;
}

QVariant
_ArincLabelModel::headerData(int at_pos, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    return header.data(at_pos, role);
}

Qt::ItemFlags
_ArincLabelModel::flags(const QModelIndex &index) const
{
    auto item = ItemFromIndex(index);

    return item->GetFlags();
}

void
_ArincLabelModel::Insert(size_t row, ProxyArincLabelItem<1> *newitem)
{
    if (newitem == nullptr)
        return;

    auto parent_item = (newitem->GetParent() == nullptr) ? rootItem : newitem->GetParent();

    if (parent_item->InsertChild(newitem, row)) {
        emit layoutAboutToBeChanged({}, QAbstractItemModel::LayoutChangeHint::NoLayoutChangeHint);

        auto index_which_was_changed = createIndex(newitem->GetRow(), newitem->GetColumn(), newitem);

        emit layoutChanged({}, QAbstractItemModel::NoLayoutChangeHint);
        emit dataChanged(index_which_was_changed, index_which_was_changed, QList<int>{ Qt::ItemDataRole::DisplayRole });
    }
}

// todo: bad behaviour, remake
void
_ArincLabelModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
    auto changed = rootItem->SortChildren(column, order);

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

    sortOrder      = order;
    sortedByColumn = column;
}

QImage
_ArincLabelModel::GetLabelMarker(LabelNumT label) const noexcept(std::is_nothrow_constructible_v<QImage>)
{
    return rootItem->GetLabelMarker(label);
}

void
_ArincLabelModel::InsertNewMessage(std::shared_ptr<ArincMsg> msg)
{
    if (not rootItem->Contains(msg->_label)) {
        Insert(0, new ProxyArincLabelItem<1>{ std::make_shared<ArincData>(msg->_label), rootItem });
    }

    auto row = rootItem->AppendMessage(msg, msg->_label);

    auto changed_index = createIndex(static_cast<int>(row), static_cast<int>(Parameter::ArincMsgs), nullptr);
    emit dataChanged(changed_index, changed_index, QList<int>{ Qt::ItemDataRole::DisplayRole });
    emit ArincMsgInserted(msg);

    if (sortedByColumn != Undefined)
        sort(sortedByColumn, static_cast<Qt::SortOrder>(sortOrder));
}
