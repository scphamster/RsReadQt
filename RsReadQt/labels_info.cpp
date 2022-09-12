#include "labels_info.hpp"

void
LabelsInfo::OnLabelInfoChanged(QTreeWidgetItem *item, int column)
{
    if (skipDataChangeEvent) {
        return;
    }

    auto lbl_num = item->text(LabelsInfo::Column::Name).toInt(nullptr, 8);

    if (column == LabelsInfo::Column::ShowOnDiagram) {
        emit LabelVisibilityChoiceChanged(lbl_num, item->checkState(5));
        return;
    }
}

void
LabelsInfo::Update(auto msg, int counter, QImage &&img)
{
    if (not labels.contains(msg->labelRaw)) {
        auto label = new QTreeWidgetItem{};

        label->setText(LabelsInfo::Column::Name, QString::number(msg->labelRaw, 8));
        label->setData(
          LabelsInfo::Column::Name,
          Qt::ItemDataRole::DecorationRole,
          img.scaled(20, 20, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        label->setText(LabelsInfo::Column::FirstOccurrence, msg->timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setText(LabelsInfo::Column::LastOccurrence, msg->timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setData(LabelsInfo::Column::HitCount, Qt::ItemDataRole::EditRole, 1);
        label->setCheckState(LabelsInfo::Column::MakeBeep, Qt::CheckState::Unchecked);
        label->setCheckState(LabelsInfo::Column::ShowOnDiagram, Qt::Checked);

        labels[msg->labelRaw] = label;

        addTopLevelItem(label);
    }
    else {
        skipDataChangeEvent = true;

        auto label = labels.at(msg->labelRaw);
        label->setText(LabelsInfo::Column::LastOccurrence, msg->timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setData(LabelsInfo::Column::HitCount, Qt::ItemDataRole::EditRole, counter);

        skipDataChangeEvent = false;
    }
}

bool
LabelsInfo::ShouldBeepOnArival(int label)
{
    if (not labels.contains(label))
        return false;

    return (labels.at(label)->checkState(LabelsInfo::Column::MakeBeep) == Qt::Checked) ? true : false;
}