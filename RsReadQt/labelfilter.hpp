#pragma once

#include <QString>
#include <QDateTime>
#include <QList>

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>

#include <QTreeView>
#include <QTreeWidget>
#include <QTableWidget>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <algorithm>

#include "_ArincLabelModel.hpp"
#include "arinc.hpp"

class ArincMsgItem {
  public:
    enum {
        Undefined = -1
    };

    enum Parameter {
        Counter = 0,
        ArrivalTime,
        Data
    };

    explicit ArincMsgItem(int counter, const QDateTime &arrival_time, const ArincMsg &data)
      : number{ counter }
      , arrivalTime{ arrival_time }
      , arincMsg{ data }
    {
        params.insert(Parameter::Counter, new QStandardItem{});
        params.at(Counter)->setData(counter, Qt::ItemDataRole::DisplayRole);

        params.insert(Parameter::ArrivalTime, new QStandardItem{});
        params.at(ArrivalTime)->setText(arrival_time.toString("hh:mm:ss:zzz"));
    }

    decltype(auto) GetRow() const noexcept { return params; }

  private:
    int                    number = Undefined;
    const QDateTime       &arrivalTime;
    QList<QStandardItem *> params;
    const ArincMsg        &arincMsg;
};

class LabelItem {
  public:
    enum Parameter {
        Name = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide,
        _MAX
    };

    enum SpecialLabel {
        New = -2,
        All = -1
    };

    explicit LabelItem(int              label,
                       const QImage    &label_img        = QImage{},
                       const QDateTime &first_occurrence = QDateTime{},
                       int              hit_count        = 0,
                       bool             make_beep        = false,
                       Qt::CheckState   show             = Qt::CheckState::Checked,
                       Qt::CheckState   hide             = Qt::CheckState::Unchecked);

    // setters
    void SetLabel(int label);
    void SetFirstOccurrence(const QDateTime &at_moment);
    void SetLastOccurrence(const QDateTime &at_moment);
    void SetShowOnDiagram(int status)
    {
        params.at(Parameter::ShowOnDiagram)->setCheckState(static_cast<Qt::CheckState>(status));
    }
    void SetHide(int status) { params.at(Parameter::Hide)->setCheckState(static_cast<Qt::CheckState>(status)); }
    void SetShow(int status) { params.at(Parameter::ShowOnDiagram)->setCheckState(static_cast<Qt::CheckState>(status)); }
    void SetData(Parameter parameter, QVariant *data);

    // getters
    QList<QStandardItem *> &GetRow() { return params; }

  private:
    QList<QStandardItem *> params;
};

class ArincLabelModel : public QStandardItemModel {
    // Q_OBJECT
  public:
    enum Parameter {
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

    const QList<QString> headerNames{ "Label", "First Occurrence", "Last occurrence", "Hit count", "Make beep", "Show",
                                      "Hide" };

    template<typename... Args>
        requires(sizeof...(Args) > 0)
    explicit ArincLabelModel(Args... args)
      : QStandardItemModel(std::forward<Args>(args)...)
    { }

    ArincLabelModel();

  public slots:
    void OnDataChange() { }
    void SetAllHide(int state)
    {
        std::for_each(labels.begin(), labels.end(), [state](auto &label_item) { label_item.second->SetHide(state); });
    }

    void SetAllShow(int state)
    {
        std::for_each(labels.begin(), labels.end(), [state](auto &label_item) { label_item.second->SetShow(state); });
    }

    void SetHideNew(int state) { hideNewCheckState = static_cast<Qt::CheckState>(state); }

  private:
    std::map<int, LabelItem *> labels;
    Qt::CheckState             hideNewCheckState = Qt::CheckState::Unchecked;
};

class LabelFilterView : public QTreeView {
    // Q_OBJECT

  public:
    explicit LabelFilterView(std::shared_ptr<_ArincLabelModel> model, QWidget *parent = nullptr)
      : QTreeView{ parent }
    {
        _model = model;
        setModel(model.get());

        hideColumn(LabelItem::Parameter::FirstOccurrence);
        hideColumn(LabelItem::Parameter::LastOccurrence);
        // hideColumn(LabelItem::Parameter::HitCount);
        hideColumn(LabelItem::Parameter::MakeBeep);

        setSortingEnabled(true);
        setAlternatingRowColors(true);
        // sortByColumn(LabelItem::Parameter::Name, Qt::SortOrder::AscendingOrder);

        // hideAllChBox = new QCheckBox{ "Hide all labels", this->viewport() };
        // showAllChBox = new QCheckBox{ "Show all labels", this->viewport() };
        // hideNew      = new QCheckBox{ "Hide new labels", this->viewport() };

        // connect(hideAllChBox, &QCheckBox::stateChanged, model, &ArincLabelModel::SetAllHide);
        // connect(showAllChBox, &QCheckBox::stateChanged, model, &ArincLabelModel::SetAllShow);
        // connect(showAllChBox, &QCheckBox::stateChanged, model, &ArincLabelModel::SetHideNew);
    }

  public slots:
    // void OnHideAllClicked(state) { dynamic_cast<LabelModel *>(model())->setallhide(state); }

  protected:
    // void resizeEvent(QResizeEvent *evt) override;

    // todo: add trigger for checkboxes : click ->

  private:
    QCheckBox                        *hideAllChBox = nullptr;
    QCheckBox                        *showAllChBox = nullptr;
    QCheckBox                        *hideNew      = nullptr;
    std::shared_ptr<_ArincLabelModel> _model;
};

class LabelsInfoView : public QTreeView {
  public:
    using Column = ArincItemData::Column;

    explicit LabelsInfoView(auto model, QWidget *parent = nullptr)
      : QTreeView{ parent }
    {
        setModel(model.get());

        setSortingEnabled(true);
        setAlternatingRowColors(true);

        hideColumn(static_cast<Column>(ArincTreeData::ColumnRole::Hide));
    }
};