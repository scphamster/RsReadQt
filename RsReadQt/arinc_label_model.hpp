#pragma once

#include <type_traits>
#include <QAbstractItemModel>
#include "arinc_label_item.hpp"
#include "child_container.hpp"


class ArincMsg;
template<typename, size_t>
class ChildContainer;
template<size_t>
class ProxyArincLabelItem;

class ArincLabelModel2 : public QAbstractItemModel {
    Q_OBJECT
  public:
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

    enum {
        Undefined = -1
    };

    ArincLabelModel2();
    ~ArincLabelModel2();

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int         rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int         columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant GetDataForStandardViews(const QModelIndex &index, int role) const;
    [[nodiscard]] bool     setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    [[nodiscard]] QVariant headerData(int at_pos, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setHeaderData(int at_pos, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void Insert(size_t row, ProxyArincLabelItem<1> *newitem);
    void Insert(size_t row, const _ArincLabel &label);
    void Insert(size_t row, const QString &label);
    void Insert(size_t row, int label);

    QImage GetLabelMarker(LabelNumT label) const noexcept(std::is_nothrow_constructible_v<QImage>);

    [[nodiscard]] ProxyArincLabelItem<1> *ItemFromIndex(const QModelIndex &idx) const;

    void             InsertNewMessage(std::shared_ptr<ArincMsg> msg);
    const QDateTime &GetStartTime() const noexcept(std::is_nothrow_constructible_v<QDateTime>);

    void SetArincMsgSelection(LabelNumT label, std::shared_ptr<ArincMsg> msg);

  signals:
    void ArincMsgInserted(std::shared_ptr<ArincMsg>);

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
