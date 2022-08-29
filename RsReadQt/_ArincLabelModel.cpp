#include "_ArincLabelModel.hpp"

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

    // test
    Insert(0, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 100 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 150 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 80 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 70 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 40 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 30 }), rootItem });
    Insert(1, new ProxyArincLabelItem{ std::make_shared<ArincData>(_ArincLabel{ 51 }), rootItem });
    // endtest
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

Qt::ItemFlags
_ArincLabelModel::flags(const QModelIndex &index) const
{
    auto item = ItemFromIndex(index);

    return item->GetFlags();
}

void
_ArincLabelModel::Insert(size_t row, ProxyArincLabelItem<1> *newitem)
{
    auto parent_item = (newitem->GetParent() == nullptr) ? rootItem : newitem->GetParent();

    if (parent_item->InsertChild(newitem, row)) {
        auto index_which_was_changed = createIndex(newitem->GetRow(), newitem->GetColumn(), newitem);
        emit dataChanged(index_which_was_changed, index_which_was_changed, QList<int>{ Qt::ItemDataRole::DisplayRole });
    }
}

//todo: bad behaviour, remake
void
_ArincLabelModel::sort(int column, Qt::SortOrder order)
{
    rootItem->SortChildren(column, order);

    auto first_index = createIndex(0, 0, rootItem->GetChild(0, 0));
    auto last_index =
      createIndex(rootItem->GetChildrenCount() - 1, 0, rootItem->GetChild(rootItem->GetChildrenCount() - 1, 0));

    emit dataChanged(first_index, last_index, QList<int>{ Qt::ItemDataRole::DisplayRole });
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