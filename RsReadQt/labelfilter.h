#pragma once

#include <QWidget>
#include <QTableWidget>
#include "ui_labelfilter.h"

class LabelTableLine {
  public:
    enum Column {
        Name = 0,
        ShowOnly,
        Hide
    };

    LabelTableLine(const QString &name);
    LabelTableLine(int label);

    QTableWidgetItem *GetLabel() { return label_num; }
    QTableWidgetItem *GetShowOnly() { return showOnlyCheck; }
    QTableWidgetItem *GetHide() { return hideCheck; }

    void SetLabel(int label) { label_num->setText(QString::number(label, 8)); }
    void SetShowOnlyCheck(Qt::CheckState state) { showOnlyCheck->setCheckState(state); }
    void SetHideCheck(Qt::CheckState state) { hideCheck->setCheckState(state); }

  private:
    QTableWidgetItem *label_num     = nullptr;
    QTableWidgetItem *showOnlyCheck = nullptr;
    QTableWidgetItem *hideCheck     = nullptr;
};

class LabelFilter : public QTableWidget {
    Q_OBJECT

  public:
    enum SpecialLines {
        AllLabels = -2,
        NewLine   = -1
    };

    LabelFilter(QWidget *parent = nullptr);
    ~LabelFilter();

  public slots:
    void OnItemChanged(QTableWidgetItem *item);
    void OnDoubleClick(QTableWidgetItem *item);
    void OnSelectionChange();

  private:
    std::map<int, LabelTableLine *> labelLines;
    int                             newRowNum = 0;
};
