#include "labelfilter.hpp"

LabelModel::LabelModel()
{
    for (int i = 0; const auto &string : headerNames) {
        setHorizontalHeaderItem(i++, new QStandardItem{ string });
    }

    AddNewRow();
}

void
LabelModel::AddNewRow(int label /* = SpecialRows::New */)
{
    QList<QStandardItem *> items;
    items.resize(Column::_SIZE);

    items[Name] = new QStandardItem{};
    items.at(Name)->setData(tr("New"), Qt::ItemDataRole::DisplayRole);
    items.at(Name)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable);

    items[LastOccurrence] = new QStandardItem{};
    items[HitCount]       = new QStandardItem{};
    items[MakeBeep]       = new QStandardItem{};
    items[MakeBeep]->setCheckState(Qt::CheckState::Checked);
    items[ShowOnDiagram]  = new QStandardItem{};
    items.at(ShowOnDiagram)->setCheckState(Qt::Unchecked);
    items[Hide]           = new QStandardItem{};
    items.at(Hide)->setCheckState(Qt::Unchecked);

    items[LastOccurrence]->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    items[HitCount]->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    items[MakeBeep]->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    items[ShowOnDiagram]->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
    items[Hide]->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );

    this->insertRow(rowCount(), std::move(items));
}

// LabelTableLine::LabelTableLine(const QString &name)
//{
//     label_num     = new QTableWidgetItem{ name };
//     showOnlyCheck = new QTableWidgetItem{ Qt::NoItemFlags };
//     hideCheck     = new QTableWidgetItem{ Qt::NoItemFlags };
//
//     showOnlyCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
//     hideCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
//
//     showOnlyCheck->setCheckState(Qt::CheckState::Unchecked);
//     hideCheck->setCheckState(Qt::Unchecked);
// }
//
// LabelTableLine::LabelTableLine(int label)
//   : label_num{ new QTableWidgetItem{ QString::number(label, 8) } }
//{
//     showOnlyCheck = new QTableWidgetItem{ Qt::NoItemFlags };
//     hideCheck     = new QTableWidgetItem{ Qt::NoItemFlags };
//
//     showOnlyCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
//     hideCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
//
//     showOnlyCheck->setCheckState(Qt::CheckState::Unchecked);
//     hideCheck->setCheckState(Qt::Unchecked);
// }
//
// LabelFilter::LabelFilter(QWidget *parent)
//   : QTreeView(1, 3, parent)
//{
//     auto col0Header = new QTableWidgetItem{ tr("Label") };
//     auto col1Header = new QTableWidgetItem{ tr("Show only") };
//     auto col2Header = new QTableWidgetItem{ tr("Hide") };
//
//     setHorizontalHeaderItem(0, col0Header);
//     setHorizontalHeaderItem(1, col1Header);
//     setHorizontalHeaderItem(2, col2Header);
//
//     labelLines[SpecialLines::NewLine] = new LabelTableLine{ "New" };
//
//     this->setItem(newRowNum, LabelTableLine::Column::Name, labelLines.at(SpecialLines::NewLine)->GetLabel());
//     this->setItem(newRowNum, LabelTableLine::Column::ShowOnly, labelLines.at(SpecialLines::NewLine)->GetShowOnly());
//     this->setItem(newRowNum, LabelTableLine::Column::Hide, labelLines.at(SpecialLines::NewLine)->GetHide());
//
//     newRowNum++;
//
//     connect(this, &QTableWidget::itemChanged, this, &LabelFilter::OnItemChanged);
//     connect(this, &QTableWidget::itemDoubleClicked, this, &LabelFilter::OnDoubleClick);
// }
//
// LabelFilter::~LabelFilter() { }
//
// void
// LabelFilter::OnItemChanged(QTableWidgetItem *item)
//{
//     auto column = item->column();
//     auto row    = item->row();
//     // auto label  = item->text();
// }
//
// void
// LabelFilter::OnDoubleClick(QTableWidgetItem *item)
//{
//     if (item->text() == "New") {
//         item->setText("");
//     }
// }
//
// void
// LabelFilter::OnSelectionChange()
//{
//     return;
// }
