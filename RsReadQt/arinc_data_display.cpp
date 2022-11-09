#include <algorithm>

#include <QMainWindow>
#include <QFileDialog>

#include <QListWidget>
#include <QLayout>

#include "arinc_label_model.hpp"
#include "arinc_data_display.h"

// test
#include "labels_info.hpp"
#include "_ArincChartView.h"
#include "arinc.hpp"
#include "serial_private.hpp"
#include "datatrack.hpp"
// endtest

ArincDataDisplay::ArincDataDisplay(deque_s<std::shared_ptr<::DTdataPacket>> &data,
               std::shared_ptr<void>                   dataBridgeConfigs,
               QTabWidget                             *tabs,
               QMainWindow                            *parent)
  : rawDTData{ data }
  , tabWgt{ tabs }
  , myParent(parent)
  , arincInterface{ std::make_unique<ArincPhysicalInterface>(
      std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetDTMsgConfigsFilename()) }
{
    // arincChart = std::make_unique<ArincLabelsChart>(parent);
    connect(arincChart.get(), &ArincLabelsChart::MsgOnChartBeenSelected, this, &ArincDataDisplay::ScrollAndSelectMsg);

    labels     = new ArincLabelModel{};
    arincModel = std::make_shared<ArincLabelModel2>();

    // test
    auto chview = new _ArincChartView{ std::make_unique<QChartLabelDrawer>(), parent };
    chview->setModel(arincModel.get());
    parent->centralWidget()->layout()->addWidget(chview);
    // endtest

    CreateRawOutput();
    CreateLabelsInfo();
    CreateFilter();
}

ArincDataDisplay::~ArincDataDisplay() = default;

void
ArincDataDisplay::CreateRawOutput()
{
    rawArincMessages = new QListWidget{ tabWgt };
    rawArincMessages->setUniformItemSizes(true);   // for better performance
    rawArincMessages->setAlternatingRowColors(true);

    tabWgt->addTab(rawArincMessages, "Raw");
}

void
ArincDataDisplay::CreateLabelsInfo()
{
    labelsInfo = new LabelsInfo{ tabWgt };
    labelsInfo->setSortingEnabled(true);
    labelsInfo->sortByColumn(LabelsInfo::Name, Qt::SortOrder::DescendingOrder);

    auto labels = new QTreeWidgetItem{};

    labels->setText(0, "Label");
    labels->setText(1, "First occurrence");
    labels->setText(2, "Last occurrence");
    labels->setText(3, "Hit count");
    labels->setText(4, "Make beep");
    labels->setText(5, "Show on diagram");

    labelsInfo->setHeaderItem(labels);
    labelsInfo->setAlternatingRowColors(true);

    tabWgt->addTab(labelsInfo, "Labels");

    connect(labelsInfo, &LabelsInfo::itemChanged, labelsInfo, &LabelsInfo::OnLabelInfoChanged);
    connect(labelsInfo,
            &LabelsInfo::LabelVisibilityChoiceChanged,
            arincChart.get(),
            &ArincLabelsChart::SetLabelVisibility);

    // test

    auto _labelsInfo = new LabelsInfoView{ arincModel, tabWgt };
    tabWgt->addTab(_labelsInfo, "labelinfo");
    // endtest
}

void
ArincDataDisplay::CreateFilter()
{
    tabWgt->addTab(new LabelFilterView(arincModel), "filter");
}

void
ArincDataDisplay::NormalizeRawData(const std::shared_ptr<DTdataPacket> &data, QString &appendHere)
{
    arincInterface->NormalizeAndStoreMsg(data);

    const auto &decoded = arincInterface->LastMsg();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded->channel)
                        .arg(decoded->labelRaw)
                        .arg(decoded->SDI)
                        .arg(decoded->valueRaw)
                        .arg(decoded->SSM)
                        .arg(decoded->parity)
                        .arg(decoded->DTtimeRaw));

    arincInterface->SetLastReadedMsgNumber(decoded->msgNumber);
}

void
ArincDataDisplay::ShowDiagram()
{
    const auto &data = arincInterface->LastMsg();
    arincChart->Append(data);
}

void
ArincDataDisplay::ShowNewData(void)
{
    if (rawDTData.empty() == true) {
        return;
    }

    auto data = std::make_shared<DTdataPacket>();
    rawDTData.pop_front_wait(data);

    QString rawOutput;

    rawOutput.append(QString{ "#%1 " }.arg(data->msgIdx));
    rawOutput.append(data->msg_arrival_time.toString(" hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");

    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);

    rawArincMessages->addItem(rawOutput);

    // test
    // if (not arincChart->IsSomeLabelSelected()) {
    //    rawArincMessages->scrollToBottom();
    //}

    // ShowDiagram();

    // labelsInfo->Update(arincInterface->LastMsg(),
    //                    arincInterface->labels.at(arincInterface->LastMsg()->label).size(),
    //                    arincChart->GetLabelMarker(arincInterface->LastMsg()->labelRaw));
    // endtest

    arincModel->InsertNewMessage(arincInterface->LastMsg());

    if (labelsInfo->ShouldBeepOnArival(arincInterface->LastMsg()->labelRaw))
        Beep((arincInterface->LastMsg()->labelRaw + 100) * 4, 100);
}

bool
ArincDataDisplay::SaveSession()
{
    auto saveto_fileAddress = QFileDialog::getSaveFileName(nullptr, tr("Save session to file"), "LastSession.txt");

    QString messagesText;

    // test
    // messagesText.append(textOutput->toPlainText());
    QFile lastSessionFile(saveto_fileAddress);

    lastSessionFile.open(QIODeviceBase::OpenModeFlag::Append);
    lastSessionFile.write(messagesText.toLocal8Bit(), messagesText.length());
    lastSessionFile.close();

    return true;
}

void
ArincDataDisplay::ScrollAndSelectMsg(size_t item)
{
    rawArincMessages->scrollToItem(rawArincMessages->item(arincChart->GetIdxOfSelectedMessage()));
    rawArincMessages->setCurrentRow((arincChart->GetIdxOfSelectedMessage()));
}
