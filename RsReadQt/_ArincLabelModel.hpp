#pragma once

//#include <gsl/pointers>

#include <QString>
#include <QAbstractItemModel>

#include "arinc.hpp"

class _ArincLabel;

template<typename T>
concept ArincLabelCompatible =
  std::same_as<T, _ArincLabel::TxtT> or std::same_as<T, _ArincLabel::NumT> or std::same_as<T, _ArincLabel>;

class _ArincLabel {
  public:
    using NumT = int;
    using TxtT = QString;

    enum {
        Undefined       = -1,
        LabelNumberBase = 8
    };

    _ArincLabel() = default;

    explicit _ArincLabel(NumT labelNumber, int number_base = LabelNumberBase)
      : asNumber{ labelNumber }
      , asText{ QString::number(labelNumber, number_base) }
    { }

    explicit _ArincLabel(const TxtT &label, int number_base = LabelNumberBase)
      : asNumber{ label.toInt(nullptr, number_base) }
      , asText{ label }
    { }

    void Set(NumT numb, int number_base = LabelNumberBase)
    {
        asNumber = numb;
        asText   = TxtT::number(numb, number_base);
    }

    void Set(const TxtT &lbl, int number_base = LabelNumberBase)
    {
        asText   = lbl;
        asNumber = lbl.toInt(nullptr, number_base);
    }

    void Set(const _ArincLabel &lbl)
    {
        asNumber = lbl.GetNumber();
        asText   = lbl.GetString();
    }

    [[nodiscard]] NumT GetNumber() const noexcept { return asNumber; }
    [[nodiscard]] TxtT GetString() const noexcept { return asText; }

  private:
    NumT asNumber = Undefined;
    TxtT asText   = TxtT{ "Undefined" };
};

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
    void SetBeeperState(Qt::CheckState newstate) noexcept { isBeeping = newstate; }
    void SetVisibilityState(Qt::CheckState newstate) noexcept { isVisible = newstate; }
    void AppendMessage(std::shared_ptr<ArincMsg> msg) { messages.push_back(msg); }

    template<typename RType = _ArincLabel>
    RType GetLabel() const noexcept
    {
        return label;
    }

    template<>
    [[nodiscard]] _ArincLabel::TxtT GetLabel<_ArincLabel::TxtT>() const noexcept
    {
        return label.GetString();
    }

    template<>
    [[nodiscard]] _ArincLabel::NumT GetLabel<_ArincLabel::NumT>() const noexcept
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
    void SetBeeperState(Qt::CheckState newstate) { setCheckState(static_cast<int>(ColumnRole::MakeBeep), newstate); }
    void SetVisibilityState(Qt::CheckState newstate)
    {
        setCheckState(static_cast<int>(ColumnRole::ShowOnDiagram), newstate);
    }
    void SetHideState(Qt::CheckState newstate) { setCheckState(static_cast<int>(ColumnRole::Hide), newstate); }
};

class ArincItemData {
  public:
    using Column = size_t;
    using Row    = size_t;

    ArincItemData()
      : arincData{ std::make_shared<ArincData>() }
    {
        SetupTreeDataFromArincData();
    }

    explicit ArincItemData(std::shared_ptr<ArincData> data)
      : arincData{ data }
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
        treeData->SetLabel(arincData->GetLabel<_ArincLabel::TxtT>());
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
    void AppendMessage(std::shared_ptr<ArincMsg> msg) { arincData->AppendMessage(msg); }

    auto GetTreeData(Column column, Qt::ItemDataRole role) { return treeData->data(column, role); }
    auto GetTreeChildrenCount() { return treeData->childCount(); }
    auto GetTreeColumnCount() { return treeData->columnCount(); }
    auto GetTreeFlags() { return treeData->flags(); }
    auto SetTreeData(Column column, Qt::ItemDataRole role, const QVariant &value)
    {
        return treeData->setData(column, role, value);
    }
    auto InsertTreeChild(Row row, ArincTreeData *child)
    {
        treeData->insertChild(row, dynamic_cast<QTreeWidgetItem *>(child));
    }
    auto GetTreeParent() { return treeData->parent(); }
    auto GetTreeItem() { return treeData; }

    auto RemoveTreeChild(ArincTreeData *child) { treeData->removeChild(child); }

  private:
    void SetupTreeDataFromArincData()
    {
        if (treeData == nullptr)
            treeData = new ArincTreeData{};

        treeData->SetLabel(arincData->GetLabel<_ArincLabel::TxtT>());
        treeData->SetFirstOccurrence(arincData->GetFirstOccurrence());
        treeData->SetLastOccurrence(arincData->GetLastOccurrence());
        treeData->SetHitCount(arincData->GetHitCount());
        treeData->SetBeeperState(arincData->GetBeeperState());
        treeData->SetVisibilityState(arincData->GetVisibilityState());

        treeData->setFlags(Qt::ItemFlag::ItemIsEnabled bitor Qt::ItemFlag::ItemIsEditable bitor
                           Qt::ItemFlag::ItemIsSelectable);
    }
    std::shared_ptr<ArincData> arincData;
    ArincTreeData             *treeData = nullptr;
};

class _ArincLabelItem {
  public:
    using Row     = size_t;
    using Column  = ArincItemData::Column;
    using Counter = ArincData::HitCounterT;

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

    _ArincLabelItem(Row _row, Column _column, std::shared_ptr<ArincData> newdata, _ArincLabelItem *_parent)
      : row{ _row }
      , compositeItem{ std::make_shared<ArincItemData>(newdata) }
      , column{ _column }
      , parentItem{ _parent }
    { }
    _ArincLabelItem(const _ArincLabelItem &)            = default;
    _ArincLabelItem &operator=(const _ArincLabelItem &) = default;
    _ArincLabelItem(_ArincLabelItem &&)                 = default;
    _ArincLabelItem &operator=(_ArincLabelItem &&)      = default;
    ~_ArincLabelItem()
    {
        for (auto child : children) {
            delete child;
        }
    }

    [[nodiscard]] auto GetParent() const noexcept { return parentItem; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] auto GetColumnCount() const noexcept
    {
        return compositeItem->GetTreeColumnCount();
    }

    template<>
    [[nodiscard]] auto GetColumnCount<BehaveLike::ArincItem>() const noexcept
    {
        // todo: remake
        return 0;
    }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] auto GetChildrenCount()
    {
        return compositeItem->GetTreeChildrenCount();
    }

    template<>
    [[nodiscard]] auto GetChildrenCount<BehaveLike::ArincItem>()
    {
        return children.size();
    }

    [[nodiscard]] auto GetRow() const noexcept { return row; }

    [[nodiscard]] auto GetColumn() const noexcept { return column; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] Qt::ItemFlags GetFlags()
    {
        return compositeItem->GetTreeFlags();
    }

    template<BehaveLike BehaviourType = BehaveLike::QTreeItem,
             typename RType           = QVariant,
             typename ColumnT         = Column,
             typename RoleT           = Qt::ItemDataRole>
    [[nodiscard]] RType GetData(ColumnT column, RoleT role)
    {
        return compositeItem->GetTreeData(column, static_cast<Qt::ItemDataRole>(role));
    }

    template<>
    [[nodiscard]] std::shared_ptr<ArincItemData>
    GetData<BehaveLike::ArincItem, std::shared_ptr<ArincItemData>, Column, ArincRole>(Column column, ArincRole role)
    {
        return 0;
        // return data.GetArincData();
    }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] bool SetData(Column column, int role, const QVariant &value)
    {
        return compositeItem->SetTreeData(column, role, value);
    }

    template<>
    [[nodiscard]] bool SetData(Column column, int role, const QVariant &value)
    {
        return false;
        // todo: implement
    }

    [[nodiscard]] auto GetParent() noexcept { return parentItem; }

    void InsertChild(Row row, _ArincLabelItem *child)
    {
        if (children.size() < (row + 1))
            children.resize(row + 1, nullptr);

        if (children.at(row) != nullptr)
            delete children.at(row);

        children.at(row) = child;
        compositeItem->InsertTreeChild(row, child->compositeItem->GetTreeItem());
    }

    [[nodiscard]] _ArincLabelItem *GetChild(Row at_row, Column at_column)
    {
        if (at_column > 0)
            return nullptr;

        if (at_row > children.size())
            return nullptr;

        auto child = children.at(at_row);
        if (child == nullptr)
            return nullptr;

        return child;
    }

  private:
    std::shared_ptr<ArincItemData> compositeItem;
    std::vector<_ArincLabelItem *> children;
    _ArincLabelItem               *parentItem;
    Row                            row    = -1;
    Column                         column = -1;
};

class _ArincLabelModel : public QAbstractItemModel {
    Q_OBJECT
  public:
    using Row       = _ArincLabelItem::Row;
    using Column    = _ArincLabelItem::Column;
    using ArincRole = _ArincLabelItem::ArincRole;

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
    [[nodiscard]] bool          setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    void Insert(_ArincLabelItem *newitem, int row, const QModelIndex &parent);

    [[nodiscard]] auto ItemFromIndex(const QModelIndex &idx) const
    {
        auto item = idx.isValid() ? static_cast<_ArincLabelItem *>(idx.internalPointer()) : rootItem;
        return (item == nullptr) ? rootItem : item;
    }

  private:
    _ArincLabelItem *rootItem;
};
