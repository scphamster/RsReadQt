#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>

#include <algorithm>

class LabelModel : public QStandardItemModel {
  public:
    enum Column {
        Name = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide,
        _SIZE
    };

    enum SpecialRows {
        All = -2,
        New = -1
    };

    const QList<QString> headerNames{ "Label", "First Occurrence", "Last occurrence", "Hit count", "Make beep", "Show",
                                      "Hide" };

    template<typename... Args>
        requires(sizeof...(Args) > 0)
    explicit LabelModel(Args... args)
      : QStandardItemModel(std::forward<Args>(args)...)
    { }

    LabelModel();

    void AddNewRow(int label = SpecialRows::New);
};

class LabelFilterDelegate : public QStyledItemDelegate {
  public:
    LabelFilterDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate{ parent }
    {
        connect(this, &QStyledItemDelegate::closeEditor, this, &LabelFilterDelegate::OnCloseEditor);
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.column() == LabelModel::Column::ShowOnDiagram) {
            auto ch_box = new QCheckBox{ parent };
            ch_box->setCheckState(Qt::CheckState::Checked);

            return ch_box;
        }

        return new QLineEdit{ parent };
    }

    //void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    //{
    //    painter->setBrush(QBrush{ QColor{ Qt::GlobalColor::darkBlue } });
    //    painter->drawRect(0, 0, 200, 20);
    //}

  public slots:
    void OnCloseEditor() { }
};

template<typename TView = QTreeView, typename TModel = QStandardItemModel, typename TDelegate = QStyledItemDelegate>
class LabelFilter {
  public:
    LabelFilter(QWidget *parent, TModel *model)
      : view{ new TView{ parent } }
    {
        view->setModel(model);

        view->hideColumn(LabelModel::Column::FirstOccurrence);
        view->hideColumn(LabelModel::Column::LastOccurrence);
        view->hideColumn(LabelModel::Column::HitCount);
        view->hideColumn(LabelModel::Column::MakeBeep);



        delegate = new TDelegate{};
        view->setItemDelegateForColumn(LabelModel::Column::Name, delegate);
    }

    TView     *GetView() { return view; }
    TDelegate *GetDelegate() { return delegate; }

  private:
    TView     *view     = nullptr;
    TDelegate *delegate = nullptr;
};

// class LabelTableLine {
//   public:
//     enum Column {
//         Name = 0,
//         ShowOnly,
//         Hide
//     };
//
//     LabelTableLine(const QString &name);
//     LabelTableLine(int label);
//
//     QTableWidgetItem *GetLabel() { return label_num; }
//     QTableWidgetItem *GetShowOnly() { return showOnlyCheck; }
//     QTableWidgetItem *GetHide() { return hideCheck; }
//
//     void SetLabel(int label) { label_num->setText(QString::number(label, 8)); }
//     void SetShowOnlyCheck(Qt::CheckState state) { showOnlyCheck->setCheckState(state); }
//     void SetHideCheck(Qt::CheckState state) { hideCheck->setCheckState(state); }
//
//   private:
//     QTableWidgetItem *label_num     = nullptr;
//     QTableWidgetItem *showOnlyCheck = nullptr;
//     QTableWidgetItem *hideCheck     = nullptr;
// };
//
// class LabelFilter : public QTreeView {
//     Q_OBJECT
//
//   public:
//     enum SpecialLines {
//         AllLabels = -2,
//         NewLine   = -1
//     };
//
//     LabelFilter(QWidget *parent = nullptr);
//     ~LabelFilter();
//
//   public slots:
//     void OnItemChanged(QTableWidgetItem *item);
//     void OnDoubleClick(QTableWidgetItem *item);
//     void OnSelectionChange();
//
//   private:
//     std::map<int, LabelTableLine *> labelLines;
//     int                             newRowNum = 0;
// };

// model
// delegate if needed
