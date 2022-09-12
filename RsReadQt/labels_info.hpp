#pragma once
#include <QTreeWidget>

class LabelsInfo : public QTreeWidget {
    Q_OBJECT

  public:
    enum Column {
        Name = 0,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram
    };

    template<typename... Args>
    LabelsInfo(Args... args)
      : QTreeWidget(std::forward<Args>(args)...)
    { }

    void Update(auto msg, int counter, QImage &&);
    bool ShouldBeepOnArival(int label);

  public slots:
    void OnLabelInfoChanged(QTreeWidgetItem *item, int column);

  signals:
    void LabelVisibilityChoiceChanged(int label, Qt::CheckState checkstate);

  private:
    std::map<int, QTreeWidgetItem *> labels;
    bool                             skipDataChangeEvent = false;
};