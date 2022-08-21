#include "labelfilter.h"

LabelTableLine::LabelTableLine(const QString &name)
{
    label_num     = new QTableWidgetItem{ name };
    showOnlyCheck = new QTableWidgetItem{ Qt::NoItemFlags };
    hideCheck     = new QTableWidgetItem{ Qt::NoItemFlags };

    showOnlyCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
    hideCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);

    showOnlyCheck->setCheckState(Qt::CheckState::Unchecked);
    hideCheck->setCheckState(Qt::Unchecked);
}

LabelTableLine::LabelTableLine(int label)
  : label_num{ new QTableWidgetItem{ QString::number(label, 8) } }
{
    showOnlyCheck = new QTableWidgetItem{ Qt::NoItemFlags };
    hideCheck     = new QTableWidgetItem{ Qt::NoItemFlags };

    showOnlyCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);
    hideCheck->setFlags(Qt::ItemFlag::ItemIsUserCheckable | Qt::ItemFlag::ItemIsEnabled);

    showOnlyCheck->setCheckState(Qt::CheckState::Unchecked);
    hideCheck->setCheckState(Qt::Unchecked);
}

LabelFilter::LabelFilter(QWidget *parent)
  : QTableWidget(1, 3, parent)
{
    auto col0Header = new QTableWidgetItem{ tr("Label") };
    auto col1Header = new QTableWidgetItem{ tr("Show only") };
    auto col2Header = new QTableWidgetItem{ tr("Hide") };

    setHorizontalHeaderItem(0, col0Header);
    setHorizontalHeaderItem(1, col1Header);
    setHorizontalHeaderItem(2, col2Header);

    labelLines[SpecialLines::NewLine] = new LabelTableLine{ "New" };

    this->setItem(newRowNum, LabelTableLine::Column::Name, labelLines.at(SpecialLines::NewLine)->GetLabel());
    this->setItem(newRowNum, LabelTableLine::Column::ShowOnly, labelLines.at(SpecialLines::NewLine)->GetShowOnly());
    this->setItem(newRowNum, LabelTableLine::Column::Hide, labelLines.at(SpecialLines::NewLine)->GetHide());

    newRowNum++;

    connect(this, &QTableWidget::itemChanged, this, &LabelFilter::OnItemChanged);
    connect(this, &QTableWidget::itemDoubleClicked, this, &LabelFilter::OnDoubleClick);
}

LabelFilter::~LabelFilter() { }

void
LabelFilter::OnItemChanged(QTableWidgetItem *item)
{
    auto column = item->column();
    auto row    = item->row();
    // auto label  = item->text();
}

void
LabelFilter::OnDoubleClick(QTableWidgetItem *item)
{
    if (item->text() == "New") {
        item->setText("");
    }
}

void
LabelFilter::OnSelectionChange()
{
    return;
}   