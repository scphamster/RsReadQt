#pragma once

#include <QTreeWidgetItem>
#include <QDateTime>
#include <QPainter>

#include "arinc_model_configs.hpp"
#include "arinc_label.hpp"

class ArincData;
class ArincTreeData;
class QDateTime;
class _ArincLabel;
class ArincMsg;
class CompositeArincItem;

template<typename, size_t>
class ChildContainer;

class ArincData {
  public:
    
    ArincData(ArincQModel::ChannelNumT belongs_to_channel = -1,
              const _ArincLabel       &lbl                = _ArincLabel{},
              const QDateTime         &firstTime          = QDateTime::currentDateTime(),
              const QDateTime         &lastTime           = QDateTime::currentDateTime(),
              HitCounterT              counter            = 0,
              Qt::CheckState           ifBeep             = Qt::Unchecked,
              Qt::CheckState           ifVisible          = Qt::Checked);

    explicit ArincData(const ArincData &other);
    ArincData &operator=(const ArincData &other);
    explicit ArincData(ArincData &&other);
    ArincData &operator=(ArincData &&other);
    ~ArincData();

    void SetFirstOccurrence(const QDateTime &when);
    void SetLastOccurrence(const QDateTime &when);
    void SetHitCount(HitCounterT quantity) noexcept;
    void IncrementHitCount(HitCounterT increment_value = 1) noexcept;
    void AppendMessage(std::shared_ptr<ArincMsg> msg);
    template<ArincLabelCompatible LblT>
    void SetLabel(const LblT &lbl)
    {
        label.Set(lbl);
    }

    template<typename RType = _ArincLabel>
    RType GetLabel() const noexcept
    {
        return label;
    }

    template<>
    [[nodiscard]] LabelTxtT GetLabel<LabelTxtT>() const noexcept
    {
        return label.GetString();
    }

    template<>
    [[nodiscard]] LabelNumT GetLabel<LabelNumT>() const noexcept
    {
        return label.GetNumber();
    }

    [[nodiscard]] ArincQModel::ChannelNumT                      GetChannelAffinity() const noexcept;
    [[nodiscard]] QDateTime                                     GetFirstOccurrence() const;
    [[nodiscard]] QDateTime                                     GetLastOccurrence() const;
    [[nodiscard]] HitCounterT                                   GetHitCount() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<ArincMsg>> &GetMessages();

  private:
    _ArincLabel                            label;
    ArincQModel::ChannelNumT               channel = -1;
    QDateTime                              firstOccurrence;
    QDateTime                              lastOccurrence;
    HitCounterT                            hitCount = 0;
    std::vector<std::shared_ptr<ArincMsg>> messages;

    bool someMsgIsSelected = false;
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

class CompositeArincItem {
  public:
    CompositeArincItem();

    explicit CompositeArincItem(std::shared_ptr<ArincData> data)
      : arincData{ data }
      , treeData{ new ArincTreeData{} }
    {
        SetupTreeDataFromArincData();
    }

    CompositeArincItem(const CompositeArincItem &)            = default;
    CompositeArincItem &operator=(const CompositeArincItem &) = default;
    CompositeArincItem(CompositeArincItem &&)                 = default;
    CompositeArincItem &operator=(CompositeArincItem &&)      = default;
    ~CompositeArincItem() { delete treeData; }

    template<ArincLabelCompatible LblT>
    void SetLabel(LblT lbl)
    {
        arincData->SetLabel(lbl);
        treeData->SetLabel(arincData->GetLabel<LabelTxtT>());
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
    void SetHitCount(HitCounterT quantity)
    {
        arincData->SetHitCount(quantity);
        treeData->SetHitCount(quantity);
    }
    void SetBeeperState(Qt::CheckState newstate) { treeData->SetBeeperState(newstate); }
    void SetVisibilityState(Qt::CheckState newstate) { treeData->SetVisibilityState(newstate); }
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

    template<typename RetType = _ArincLabel>
    auto GetLabel()
    {
        return arincData->GetLabel<RetType>();
    }

  protected:
    void SetupTreeDataFromArincData()
    {
        treeData->SetLabel(arincData->GetLabel<LabelTxtT>());
        treeData->SetFirstOccurrence(arincData->GetFirstOccurrence());
        treeData->SetLastOccurrence(arincData->GetLastOccurrence());
        treeData->SetHitCount(arincData->GetHitCount());
        treeData->SetBeeperState(Qt::Unchecked);
        treeData->SetVisibilityState(Qt::Checked);

        using Flag = Qt::ItemFlag;
        treeData->setFlags(static_cast<Qt::ItemFlag>(ArincTreeData::standardFlags));
    }

  private:
    std::shared_ptr<ArincData> arincData;
    ArincTreeData             *treeData = nullptr;
};
