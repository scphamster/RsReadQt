#pragma once

#include <QString>
#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>

#include <algorithm>

#include "arinc.hpp"

class LabelConfigsItem : protected QStandardItem {
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

    LabelConfigsItem(int              label,
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

class LabelConfigsModel : public QStandardItemModel {
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
        _SIZE
    };

    const QList<QString> headerNames{ "Label", "First Occurrence", "Last occurrence", "Hit count", "Make beep", "Show",
                                      "Hide" };

    template<typename... Args>
        requires(sizeof...(Args) > 0)
    explicit LabelConfigsModel(Args... args)
      : QStandardItemModel(std::forward<Args>(args)...)
    { }

    LabelConfigsModel();

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
    std::map<int, LabelConfigsItem *> labels;
    Qt::CheckState                  hideNewCheckState = Qt::CheckState::Unchecked;
};

class LabelFilterView : public QTreeView {
  public:
    explicit LabelFilterView(LabelConfigsModel *model, QWidget *parent = nullptr)
      : QTreeView{ parent }
    {
        setModel(model);

        hideColumn(LabelConfigsItem::Parameter::FirstOccurrence);
        hideColumn(LabelConfigsItem::Parameter::LastOccurrence);
        hideColumn(LabelConfigsItem::Parameter::HitCount);
        hideColumn(LabelConfigsItem::Parameter::MakeBeep);

        sortByColumn(LabelConfigsItem::Parameter::Name, Qt::SortOrder::AscendingOrder);

        hideAllChBox = new QCheckBox{ "Hide all labels", this->viewport() };
        showAllChBox = new QCheckBox{ "Show all labels", this->viewport() };
        hideNew      = new QCheckBox{ "Hide new labels", this->viewport() };

        connect(hideAllChBox, &QCheckBox::stateChanged, model, &LabelConfigsModel::SetAllHide);
        connect(showAllChBox, &QCheckBox::stateChanged, model, &LabelConfigsModel::SetAllShow);
        connect(showAllChBox, &QCheckBox::stateChanged, model, &LabelConfigsModel::SetHideNew);
    }

  public slots:
    // void OnHideAllClicked(state) { dynamic_cast<LabelModel *>(model())->setallhide(state); }

  protected:
    void resizeEvent(QResizeEvent *evt) override;

    // todo: add trigger for checkboxes : click ->

  private:
    QCheckBox *hideAllChBox = nullptr;
    QCheckBox *showAllChBox = nullptr;
    QCheckBox *hideNew      = nullptr;
};

class LabelInfoView : public QTreeView { 
    public:
    explicit LabelInfoView(QWidget *parent)
      : QTreeView{ parent }
    { }
};