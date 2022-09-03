#pragma once

//#include <gsl/pointers>

#include <QString>
#include <QTreeWidgetItem>
#include <QAbstractItemModel>
#include <QDateTime>

#include <algorithm>
#include <type_traits>

class ArincMsg;

class _ArincLabel {
  public:
    using NumType = int;
    using TxtType = QString;

    enum {
        Undefined       = -1,
        LabelNumberBase = 8
    };

    _ArincLabel() = default;

    explicit _ArincLabel(NumType labelNumber, int number_base = LabelNumberBase)
      : asNumber{ labelNumber }
      , asText{ QString::number(labelNumber, number_base) }
    { }

    explicit _ArincLabel(const TxtType &label, int number_base = LabelNumberBase)
      : asNumber{ label.toInt(nullptr, number_base) }
      , asText{ label }
    { }

    bool operator==(const _ArincLabel &rhLabel) { return (asNumber == rhLabel.asNumber and asText == rhLabel.asText); }

    void Set(NumType numb, int number_base = LabelNumberBase)
    {
        asNumber = numb;
        asText   = TxtType::number(numb, number_base);
    }

    void Set(const TxtType &lbl, int number_base = LabelNumberBase)
    {
        asText   = lbl;
        asNumber = lbl.toInt(nullptr, number_base);
    }

    void Set(const _ArincLabel &lbl)
    {
        asNumber = lbl.GetNumber();
        asText   = lbl.GetString();
    }

    [[nodiscard]] NumType GetNumber() const noexcept { return asNumber; }
    [[nodiscard]] TxtType GetString() const noexcept { return asText; }

  private:
    NumType asNumber = Undefined;
    TxtType asText   = TxtType{ "Undefined" };
};

template<typename T>
concept ArincLabelCompatible =
  std::same_as<T, _ArincLabel::TxtType> or std::same_as<T, _ArincLabel::NumType> or std::same_as<T, _ArincLabel>;

class ArincData {
  public:
    using HitCounterT = size_t;

    ArincData(const _ArincLabel &lbl       = _ArincLabel{},
              const QDateTime   &firstTime = QDateTime::currentDateTime(),
              const QDateTime   &lastTime  = QDateTime::currentDateTime(),
              HitCounterT        counter   = 0,
              Qt::CheckState     ifBeep    = Qt::Unchecked,
              Qt::CheckState     ifVisible = Qt::Checked)
      : label{ lbl }
      , firstOccurrence{ firstTime }
      , lastOccurrence{ lastTime }
      , hitCount{ counter }
      , isBeeping{ ifBeep }
      , isVisible{ ifVisible }
    { }

    explicit ArincData(const ArincData &other)   = default;
    ArincData &operator=(const ArincData &other) = default;
    explicit ArincData(ArincData &&other)        = default;
    ArincData &operator=(ArincData &&other)      = default;
    ~ArincData()                                 = default;

    template<ArincLabelCompatible LblT>
    void SetLabel(const LblT &lbl)
    {
        label.Set(lbl);
    }
    void SetFirstOccurrence(const QDateTime &when) { firstOccurrence = when; }
    void SetLastOccurrence(const QDateTime &when) { lastOccurrence = when; }
    void SetHitCount(HitCounterT quantity) noexcept { hitCount = quantity; }
    void IncrementHitCount(HitCounterT increment_value = 1) noexcept { hitCount += increment_value; }
    void SetBeeperState(Qt::CheckState newstate) noexcept { isBeeping = newstate; }
    void SetVisibilityState(Qt::CheckState newstate) noexcept { isVisible = newstate; }
    void AppendMessage(std::shared_ptr<ArincMsg> msg) { messages.push_back(msg); }

    template<typename RType = _ArincLabel>
    RType GetLabel() const noexcept
    {
        return label;
    }

    template<>
    [[nodiscard]] _ArincLabel::TxtType GetLabel<_ArincLabel::TxtType>() const noexcept
    {
        return label.GetString();
    }

    template<>
    [[nodiscard]] _ArincLabel::NumType GetLabel<_ArincLabel::NumType>() const noexcept
    {
        return label.GetNumber();
    }

    [[nodiscard]] auto        GetFirstOccurrence() const { return firstOccurrence; }
    [[nodiscard]] auto        GetLastOccurrence() const { return lastOccurrence; }
    [[nodiscard]] auto        GetHitCount() const noexcept { return hitCount; }
    [[nodiscard]] auto        GetBeeperState() const noexcept { return isBeeping; }
    [[nodiscard]] auto        GetVisibilityState() const noexcept { return isVisible; }
    [[nodiscard]] const auto &GetMessages() { return messages; }

  private:
    _ArincLabel                            label;
    QDateTime                              firstOccurrence;
    QDateTime                              lastOccurrence;
    HitCounterT                            hitCount  = 0;
    Qt::CheckState                         isBeeping = Qt::Unchecked;
    Qt::CheckState                         isVisible = Qt::Checked;
    std::vector<std::shared_ptr<ArincMsg>> messages;
};

// todo make protected/private
class ArincTreeData : public QTreeWidgetItem {
  public:
    enum class ColumnRole {
        LabelNum = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide
    };

    using Flag = Qt::ItemFlag;
    static const std::underlying_type_t<Qt::ItemFlag> standardFlags =
      Flag::ItemIsEnabled bitor Flag::ItemIsSelectable bitor Flag::ItemIsUserCheckable bitor Flag::ItemIsEditable;

    ArincTreeData()
    {
        setText(static_cast<int>(ColumnRole::LabelNum), QString{ "Undefined" });
        setText(static_cast<int>(ColumnRole::FirstOccurrence), QDateTime::currentDateTime().toString("hh:mm:ss:zzz"));
        setText(static_cast<int>(ColumnRole::LastOccurrence), QDateTime::currentDateTime().toString("hh:mm:ss:zzz"));
        setText(static_cast<int>(ColumnRole::HitCount), QString::number(0));
        setCheckState(static_cast<int>(ColumnRole::MakeBeep), Qt::Unchecked);
        setCheckState(static_cast<int>(ColumnRole::ShowOnDiagram), Qt::Checked);
        setData(static_cast<int>(ColumnRole::Hide), Qt::ItemDataRole::CheckStateRole, Qt::CheckState::Unchecked);
    }

    void SetLabel(const QString &lbl) { setText(static_cast<int>(ColumnRole::LabelNum), lbl); }
    void SetFirstOccurrence(const QDateTime &when)
    {
        setText(static_cast<int>(ColumnRole::FirstOccurrence), when.toString("hh:mm:ss:zzz"));
    }
    void SetLastOccurrence(const QDateTime &when)
    {
        setText(static_cast<int>(ColumnRole::LastOccurrence), when.toString("hh:mm:ss:zzz"));
    }
    void SetHitCount(auto quantity) { setText(static_cast<int>(ColumnRole::HitCount), QString::number(quantity)); }
    void IncrementHitCount(int increment_value = 1)
    {
        auto counter_value =
          data(static_cast<int>(ColumnRole::HitCount), Qt::ItemDataRole::DisplayRole).toInt() + increment_value;
        setData(static_cast<int>(ColumnRole::HitCount), Qt::ItemDataRole::DisplayRole, counter_value);
    }
    void SetBeeperState(Qt::CheckState newstate) { setCheckState(static_cast<int>(ColumnRole::MakeBeep), newstate); }
    void SetVisibilityState(Qt::CheckState newstate)
    {
        setCheckState(static_cast<int>(ColumnRole::ShowOnDiagram), newstate);
    }
    void SetHideState(Qt::CheckState newstate) { setCheckState(static_cast<int>(ColumnRole::Hide), newstate); }
};

class ArincItemData {
  public:
    using Column = int;
    using Row    = int;

    ArincItemData()
      : arincData{ std::make_shared<ArincData>() }
      , treeData{ new ArincTreeData{} }
    {
        SetupTreeDataFromArincData();
    }

    explicit ArincItemData(std::shared_ptr<ArincData> data)
      : arincData{ data }
      , treeData{ new ArincTreeData{} }
    {
        SetupTreeDataFromArincData();
    }

    ArincItemData(const ArincItemData &)            = default;
    ArincItemData &operator=(const ArincItemData &) = default;
    ArincItemData(ArincItemData &&)                 = default;
    ArincItemData &operator=(ArincItemData &&)      = default;
    ~ArincItemData() { delete treeData; }

    template<ArincLabelCompatible LblT>
    void SetLabel(LblT lbl)
    {
        arincData->SetLabel(lbl);
        treeData->SetLabel(arincData->GetLabel<_ArincLabel::TxtType>());
    }
    void SetFirstOccurrence(const QDateTime &when)
    {
        arincData->SetFirstOccurrence(when);
        treeData->SetFirstOccurrence(when);
    }
    void SetLastOccurrence(const QDateTime &when)
    {
        arincData->SetLastOccurrence(when);
        treeData->SetLastOccurrence(when);
    }
    void SetHitCount(ArincData::HitCounterT quantity)
    {
        arincData->SetHitCount(quantity);
        treeData->SetHitCount(quantity);
    }
    void SetBeeperState(Qt::CheckState newstate)
    {
        arincData->SetBeeperState(newstate);
        treeData->SetBeeperState(newstate);
    }
    void SetVisibilityState(Qt::CheckState newstate)
    {
        arincData->SetVisibilityState(newstate);
        treeData->SetVisibilityState(newstate);
    }
    void SetArincData(std::shared_ptr<ArincData> newdata)
    {
        arincData = newdata;
        SetupTreeDataFromArincData();
    }

    void AppendMessage(std::shared_ptr<ArincMsg> msg)
    {
        arincData->AppendMessage(msg);
        treeData->IncrementHitCount();
    }

    auto GetTreeData(Column column, Qt::ItemDataRole role) { return treeData->data(column, role); }
    auto GetTreeChildrenCount() { return treeData->childCount(); }
    auto GetTreeColumnCount() { return treeData->columnCount(); }
    auto GetTreeFlags() { return treeData->flags(); }
    void SetTreeData(Column column, Qt::ItemDataRole role, const QVariant &value)
    {
        treeData->setData(column, role, value);
    }
    auto InsertTreeChild(Row row, ArincTreeData *child)
    {
        treeData->insertChild(row, dynamic_cast<QTreeWidgetItem *>(child));
    }
    auto GetTreeParent() { return treeData->parent(); }
    auto GetTreeItem() { return treeData; }

    auto RemoveTreeChild(ArincTreeData *child) { treeData->removeChild(child); }

    auto GetArincData() { return arincData; }
    auto GetLabel() { return arincData->GetLabel(); }

  private:
    void SetupTreeDataFromArincData()
    {
        treeData->SetLabel(arincData->GetLabel<_ArincLabel::TxtType>());
        treeData->SetFirstOccurrence(arincData->GetFirstOccurrence());
        treeData->SetLastOccurrence(arincData->GetLastOccurrence());
        treeData->SetHitCount(arincData->GetHitCount());
        treeData->SetBeeperState(arincData->GetBeeperState());
        treeData->SetVisibilityState(arincData->GetVisibilityState());

        using Flag = Qt::ItemFlag;
        treeData->setFlags(static_cast<Qt::ItemFlag>(ArincTreeData::standardFlags));
    }

    std::shared_ptr<ArincData> arincData;
    ArincTreeData             *treeData = nullptr;
};

// todo: add sorting
template<typename ChildT, size_t DimensionsInUse = 1>
class ChildContainer {
  public:
    ~ChildContainer()
    {
        for (auto child : children)
            delete child;
    }

    [[nodiscard]] auto GetChild(size_t position)
    {
        if (children.size() - 1 < position)
            return static_cast<ChildT *>(nullptr);

        return children.at(position);
    }

    template<typename T, typename FinderFunctor>
    [[nodiscard]] auto GetChildWithParam(const T &param, FinderFunctor finder)
    {
        auto child = std::find_if(children.begin(), children.end(), finder);

        if (child == children.end())
            return static_cast<ChildT *>(nullptr);
        else
            return *child;
    }

    // todo: make children be packed with no gaps, contiguously
    [[nodiscard]] auto InsertChild(size_t position, ChildT *newchild)
    {
        if (newchild == nullptr)
            return false;

        if (dimensionMaxPosition + 1 < position)
            position = dimensionMaxPosition + 1;

        dimensionMaxPosition++;
        children.resize(dimensionMaxPosition + 1);

        // push all children one row down after "position"
        if (children.size() > 1 and (position < children.size() - 1)) {
            auto insert_item_at     = children.rbegin() + (children.size() - 1 - position);
            auto new_child_position = children.size() - 1;
            auto one_child_ahead    = children.rbegin() + 1;

            std::for_each(children.rbegin(), insert_item_at, [this, &new_child_position, &one_child_ahead](auto &child) {
                child = *one_child_ahead;
                child->SetRow(new_child_position);
                new_child_position--;
                one_child_ahead++;
            });
        }

        children.at(position) = newchild;
        newchild->SetRow(position);

        return true;
    }

    [[nodiscard]] auto ChildAtPosExists(size_t at_pos) const noexcept { return children.contains(at_pos); }

    [[nodiscard]] auto GetChildrenCount() const noexcept { return children.size(); }

    [[nodiscard]] auto GetDoNotExceedDimensionsSizes(int dimension) const noexcept { return dimensionMaxPosition; }

    // template<std::predicate SortingFunctor>
    // void Sort(auto column, auto order, SortingFunctor sorter)
    //{
    //     std::sort(children.begin(), children.end(), sorter);
    // }

    template<typename SortingFunctor>
    decltype(auto) Sort(auto column, auto order, SortingFunctor sorter)
    {
        std::pair<QList<std::pair<int, ChildT *>>, QList<std::pair<int, ChildT *>>> changedModelIndexesFromTo{};

        for (const auto child : children) {
            changedModelIndexesFromTo.first << std::pair<int, ChildT *>{ child->GetRow(), child };
        }
        std::stable_sort(children.begin(), children.end(), sorter);

        // auto temp             = *(children.end() - 1);
        //*(children.end() - 1) = *(children.begin());
        //*(children.begin())   = temp;

        for (size_t position{ 0 }; auto child : children) {
            child->SetRow(position);
            changedModelIndexesFromTo.second << std::pair<int, ChildT *>{ position, child };

            position++;
        }

        return changedModelIndexesFromTo;
    }

    template<typename T, typename EqComparatorFunctor>
    auto Contains(const T &wantedParam, EqComparatorFunctor comparator) const
    {
        return std::any_of(children.begin(), children.end(), comparator);
    }

  private:
    std::vector<ChildT *> children;
    size_t                dimensionMaxPosition = -1;
};

template<typename ChildT>
class ChildContainer<ChildT, 2> {
    [[nodiscard]] auto GetChild(size_t position_d1, size_t position_d2) { return 0; }

  private:
    std::map<size_t, std::map<size_t, ChildT *>> children;
    std::vector<size_t>                          dimensionSizes;
};

template<size_t NumberOfDimensions = 1>
class ProxyArincLabelItem {
  public:
    using Row            = size_t;
    using Column         = ArincItemData::Column;
    using Counter        = ArincData::HitCounterT;
    using ChildContainer = ChildContainer<ProxyArincLabelItem, NumberOfDimensions>;

    enum class BehaveLike {
        QTreeItem,
        ArincItem
    };

    enum class ArincRole {
        NoRole = -1,
        Name,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide,
        ArincMessages,
        ArincMsg,
        _SIZE
    };

    ProxyArincLabelItem(std::shared_ptr<ArincData> data_of_item, ProxyArincLabelItem *parent_of_this_item = nullptr)
      : universalItem{ std::make_shared<ArincItemData>(data_of_item) }
      , parentItem{ parent_of_this_item }
    { }

    ProxyArincLabelItem(const ProxyArincLabelItem &)            = default;
    ProxyArincLabelItem &operator=(const ProxyArincLabelItem &) = default;
    ProxyArincLabelItem(ProxyArincLabelItem &&)                 = default;
    ProxyArincLabelItem &operator=(ProxyArincLabelItem &&)      = default;
    ~ProxyArincLabelItem()                                      = default;

    [[nodiscard]] auto GetParent() const noexcept { return parentItem; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] auto GetColumnCount() const noexcept
    {
        return universalItem->GetTreeColumnCount();
    }

    template<>
    [[nodiscard]] auto GetColumnCount<BehaveLike::ArincItem>() const noexcept
    {
        // todo: remake
        return 0;
    }

    [[nodiscard]] auto GetChildrenCount() { return children.GetChildrenCount(); }

    [[nodiscard]] auto GetRow() const noexcept { return locatedAtRow; }

    [[nodiscard]] auto GetColumn() const noexcept { return locatedAtColumn; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] Qt::ItemFlags GetFlags()
    {
        return universalItem->GetTreeFlags();
    }

    template<BehaveLike BehaviourType = BehaveLike::QTreeItem,
             typename RType           = QVariant,
             typename ColumnT         = Column,
             typename RoleT           = Qt::ItemDataRole>
    [[nodiscard]] RType GetData(ColumnT column, RoleT role)
    {
        return universalItem->GetTreeData(column, static_cast<Qt::ItemDataRole>(role));
    }

    template<>
    [[nodiscard]] std::shared_ptr<ArincItemData>
    GetData<BehaveLike::ArincItem, std::shared_ptr<ArincItemData>, Column, ArincRole>(Column column, ArincRole role)
    {
        return 0;
        // return data.GetArincData();
    }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    void SetData(Column column, int role, const QVariant &value)
    {
        universalItem->SetTreeData(column, static_cast<Qt::ItemDataRole>(role), value);
    }

    template<>
    void SetData<BehaveLike::ArincItem>(Column column, int role, const QVariant &value)
    {
        // compositeItem->SetArincData()
    }

    void SetItemsRow(size_t newrow) noexcept
    {
        if (newrow > 0)
            locatedAtRow = newrow;
    }

    [[nodiscard]] auto GetParent() noexcept { return parentItem; }
    [[nodiscard]] auto InsertChild(ProxyArincLabelItem *child, Row row = 0) { return children.InsertChild(row, child); }
    [[nodiscard]] auto GetChild(Row at_row, Column at_column) { return children.GetChild(at_row); }

    void SetRow(auto new_row) noexcept { locatedAtRow = new_row; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    decltype(auto) SortChildren(Column column, Qt::SortOrder order)
    {
        return children.Sort(column, order, [column, order](const auto &first_child, const auto &second_child) {
            bool answer;

            decltype(first_child->universalItem->GetArincData()) ardata1;
            decltype(first_child->universalItem->GetArincData()) ardata2;

            if (order == Qt::SortOrder::AscendingOrder) {
                ardata1 = first_child->universalItem->GetArincData();
                ardata2 = second_child->universalItem->GetArincData();
            }
            else {
                ardata2 = first_child->universalItem->GetArincData();
                ardata1 = second_child->universalItem->GetArincData();
            }

            using Col = ArincTreeData::ColumnRole;
            switch (column) {
            case static_cast<Column>(Col::LabelNum):
                answer = ardata1->GetLabel<_ArincLabel::NumType>() < ardata2->GetLabel<_ArincLabel::NumType>();
                break;
            case static_cast<Column>(Col::FirstOccurrence):
                answer = ardata1->GetFirstOccurrence() < ardata2->GetFirstOccurrence();
                break;
            case static_cast<Column>(Col::LastOccurrence):
                answer = ardata1->GetLastOccurrence() < ardata2->GetLastOccurrence();
                break;
            case static_cast<Column>(Col::HitCount):
                answer = ardata1->GetHitCount() < ardata1->GetHitCount();
                break;
                // case Col::MakeBeep:
                //     answer = ardata1->GetBeeperState()
                // case Col::ShowOnDiagram :   answer = ardata1->GetVisibilityState()
                // case Col::Hide :            answer = ardata1->GetHitCount()

            default: answer = false;
            }

            return answer;
        });
    }

    auto Contains(const _ArincLabel &label) const
    {
        return children.Contains(label, [&label](const auto &child) {
            if (child->universalItem->GetLabel() == label)
                return true;
            else
                return false;
        });
    }

    void AppendMessage(std::shared_ptr<ArincMsg> msg, const _ArincLabel &forLabel)
    {
        auto child = children.GetChildWithParam(forLabel, [label = forLabel](const auto &child) {
            if (child->universalItem->GetLabel() == label)
                return true;
            else
                return false;
        });

        if (child == nullptr)
            return;

        child->universalItem->AppendMessage(msg);
    }

  private:
    std::shared_ptr<ArincItemData> universalItem;
    // std::vector<_ArincLabelItem *> children;
    ChildContainer children;

    ProxyArincLabelItem *parentItem;
    Row                  locatedAtRow    = 0;
    Column               locatedAtColumn = 0;
};

class _ArincLabelModel : public QAbstractItemModel {
    Q_OBJECT
  public:
    using Row       = ProxyArincLabelItem<1>::Row;
    using Column    = ProxyArincLabelItem<1>::Column;
    using ArincRole = ProxyArincLabelItem<1>::ArincRole;

    enum class Parameter {
        Name = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide,
        ArincMsgs,
        _SIZE
    };

    _ArincLabelModel();

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int         rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int         columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant GetDataForStandardViews(const QModelIndex &index, int role) const;
    // QVariant      GetDataForSpecialViews(const QModelIndex &index, int role) const;
    [[nodiscard]] bool     setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    [[nodiscard]] QVariant headerData(int at_pos, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setHeaderData(int at_pos, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void Insert(size_t row, ProxyArincLabelItem<1> *newitem);
    void Insert(size_t row, const _ArincLabel &label);
    void Insert(size_t row, const QString &label);
    void Insert(size_t row, int label);

    [[nodiscard]] auto ItemFromIndex(const QModelIndex &idx) const
    {
        auto item = idx.isValid() ? static_cast<ProxyArincLabelItem<1> *>(idx.internalPointer()) : rootItem;
        return (item == nullptr) ? rootItem : item;
    }

    void InsertNewMessage(std::shared_ptr<ArincMsg> msg);

  private:
    ProxyArincLabelItem<1> *rootItem;
    QTreeWidgetItem         header;
};
