#include "labelfilter.hpp"
#include "_ArincLabelModel.hpp"

LabelItem::LabelItem(int              label = SpecialLabel::New,
                     const QImage    &label_img,
                     const QDateTime &first_occurrence,
                     int              hit_count,
                     bool             make_beep,
                     Qt::CheckState   show,
                     Qt::CheckState   hide)
{
    params.insert(Parameter::Name, new QStandardItem{});
    // Todo: set value??
    if (label == SpecialLabel::New)
        params.at(Name)->setText(QObject::tr("New"));
    else if (label == SpecialLabel::All)
        params.at(Name)->setText(QObject::tr("All"));
    else
        params.at(Name)->setText(QString::number(label, 8));

    if (not  label_img.isNull())
        params.at(Name)->setData(
          label_img.scaled(20, 20, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation),
          Qt::ItemDataRole::DecorationRole);

    params.at(Name)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable);

    params.insert(Parameter::FirstOccurrence, new QStandardItem{});
    params.at(Parameter::FirstOccurrence)->setText(first_occurrence.toString("hh:mm:ss:zzz"));
    params.at(Parameter::FirstOccurrence)->setFlags(Qt::ItemFlag::ItemIsEnabled);

    params.insert(Parameter::LastOccurrence, new QStandardItem{});
    params.at(Parameter::LastOccurrence)->setText(first_occurrence.toString("hh:mm:ss:zzz"));
    params.at(Parameter::LastOccurrence)->setFlags(Qt::ItemFlag::ItemIsEnabled);

    params.insert(HitCount, new QStandardItem{});
    params.at(HitCount)->setData(0, Qt::ItemDataRole::DisplayRole);
    params.at(HitCount)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsSelectable);

    // auto msgitem = new ArincMsgItem{ 2, QDateTime::currentDateTime(), ArincMsg{} };
    // params.at(HitCount)->setChild(1, dynamic_cast<QStandardItem *>(msgitem));

    params.insert(MakeBeep, new QStandardItem{});
    params.at(MakeBeep)->setCheckState(Qt::CheckState::Checked);
    params.at(MakeBeep)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsUserCheckable);

    params.insert(ShowOnDiagram, new QStandardItem{});
    params.at(ShowOnDiagram)->setCheckState(Qt::Unchecked);
    params.at(ShowOnDiagram)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsUserCheckable);

    params.insert(Hide, new QStandardItem{});
    params.at(Hide)->setCheckState(Qt::Unchecked);
    params.at(Hide)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemIsUserCheckable);
}

ArincLabelModel::ArincLabelModel()
{
    for (int i = 0; const auto &string : headerNames) {
        auto h_item = new QStandardItem{ string };
        h_item->setFlags(Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled);

        // setHorizontalHeaderItem(i++, new QStandardItem{ string });
        setHorizontalHeaderItem(i++, h_item);
    }

    //test purpouse
    auto label  = new LabelItem{ 200, QImage{}, QDateTime::currentDateTime() };
    labels[200] = label;
    insertRow(0, label->GetRow());

    //setData(index(0, Parameter::ArincMsgs), std::vector<std::shared_ptr<ArincMsg>>{});

    emit dataChanged(index(0, 0), index(0, 0));

    labels[230] = new LabelItem{ 230 };
    insertRow(0, labels.at(230)->GetRow());
    //end test

    connect(this, &QStandardItemModel::dataChanged, this, &ArincLabelModel::OnDataChange);
}


//
//void
//LabelFilterView::resizeEvent(QResizeEvent *evt)
//{
//    constexpr auto chboxW = 120, chboxH = 20;
//    constexpr auto chbox1LPos = 15, chbox2LPos = chbox1LPos + chboxW, chbox3LPos = chbox2LPos + chboxW;
//
//    this->setFrameRect(QRect{ 0, 0, evt->size().width(), evt->size().height() - 30 });
//    hideAllChBox->setGeometry(QRect{ chbox1LPos, evt->size().height() - 30, chboxW, chboxH });
//    showAllChBox->setGeometry(QRect{ chbox2LPos, evt->size().height() - 30, chboxW, chboxH });
//    hideNew->setGeometry(QRect{ chbox3LPos, evt->size().height() - 30, chboxW, chboxH });
//}
