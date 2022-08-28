#include "_ArincLabelModel.hpp"

_ArincLabelModel::_ArincLabelModel()
//: QAbstractItemModel{}
{
    rootItem = new _ArincLabelItem{ 0, 0, std::make_shared<ArincData>(), nullptr };
    Insert(new _ArincLabelItem{ 0, 0, std::make_shared<ArincData>(_ArincLabel{ 100 }), rootItem },
           0,
           createIndex(rootItem->GetRow(), rootItem->GetColumn(), rootItem));

    Insert(new _ArincLabelItem{ 1, 0, std::make_shared<ArincData>(_ArincLabel{ 150 }), rootItem },
           1,
           createIndex(rootItem->GetRow(), rootItem->GetColumn(), rootItem));
}

QModelIndex
_ArincLabelModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (not hasIndex(row, column, parent))
        return QModelIndex{};

    // if (column >= static_cast<int>(_ArincLabelItem::ArincRole::_SIZE))
    // return QModelIndex{};

    _ArincLabelItem *parent_item;

    if (parent == QModelIndex{})
        parent_item = rootItem;
    else
        parent_item = static_cast<_ArincLabelItem *>(parent.internalPointer());

    return createIndex(row, column, parent_item->GetChild(row, column));
}

QModelIndex
_ArincLabelModel::parent(const QModelIndex &child) const
{
    if (not child.isValid())
        return QModelIndex{};

    auto child_item = static_cast<_ArincLabelItem *>(child.internalPointer());
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
    _ArincLabelItem *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<_ArincLabelItem *>(parent.internalPointer()) == nullptr)
                                      ? rootItem
                                      : static_cast<_ArincLabelItem *>(parent.internalPointer());
    else
        parent_item = rootItem;

    return parent_item->GetChildrenCount();
}

int
_ArincLabelModel::columnCount(const QModelIndex &parent /*= QModelIndex{}*/) const
{
    _ArincLabelItem *parent_item;

    if (parent.isValid())
        parent_item = (static_cast<_ArincLabelItem *>(parent.internalPointer()) == nullptr)
                                      ? rootItem
                                      : static_cast<_ArincLabelItem *>(parent.internalPointer());
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
    auto item = ItemFromIndex(index);

    return item->GetData(index.column(), role);
}

bool
_ArincLabelModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // todo: make path for my view
    // this is the path for QTreeView

    // index contains item and column if column count is insufficient -> enlarge
    //
    auto item = ItemFromIndex(index);

    if (item->SetData(index.column(), role, value)) {
        emit dataChanged(index, index, QList<int>{ role });
        return true;
    }

    return false;
}

Qt::ItemFlags
_ArincLabelModel::flags(const QModelIndex &index) const
{
    if (not hasIndex(index.row(), index.column(), index))
        return Qt::ItemFlags{};

    auto item = ItemFromIndex(index);

    return item->GetFlags();
}

void
_ArincLabelModel::Insert(_ArincLabelItem *newitem, int row, const QModelIndex &parent)
{
    _ArincLabelItem *parent_item;

    if (parent.isValid())
        parent_item = static_cast<_ArincLabelItem *>(parent.internalPointer());
    else
        parent_item = rootItem;

    parent_item->InsertChild(row, newitem);

    auto index_which_was_changed = createIndex(row, 0, newitem);
    emit dataChanged(index_which_was_changed, index_which_was_changed, QList<int>{ Qt::ItemDataRole::DisplayRole });
}
