#include <algorithm>

#include <QMainWindow>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>
#include <QTime>
#include <qtextedit.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <QTextBlock>
#include <QMessageBox>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QXYSeries>
#include <QLineSeries>
#include <QScatterSeries>
#include <QSplineSeries>
#include <QAbstractAxis>
#include <QValueAxis>
#include <QPixMap>
#include <QPainter>

#include <QList>
#include <QListWidget>
#include <QListView>

#include <QLayout>

#include "serialthread.h"
//#include "arinc.hpp"
#include "arinc_label_model.hpp"
#include <Windows.h>

// test
#include "labels_info.hpp"
#include "_ArincChartView.h"
#include "arinc.hpp"
#include "Serial.h"
#include "serial_private.hpp"

// endtest

Output::Output(deque_s<std::shared_ptr<::dataPacket>> &data,
               std::shared_ptr<void>                   dataBridgeConfigs,
               QTabWidget                             *tabs,
               QMainWindow                            *parent)
  : rawData{ data }
  , tabWgt{ tabs }
  , myParent(parent)
  , arinc{ std::make_unique<ArincDriver>(
      std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName()) }
{
    // arincChart = std::make_unique<ArincLabelsChart>(parent);
    connect(arincChart.get(), &ArincLabelsChart::MsgOnChartBeenSelected, this, &Output::ScrollAndSelectMsg);

    labels     = new ArincLabelModel{};
    arincModel = std::make_shared<ArincLabelModel2>();

    // test
    auto chview = new _ArincChartView{ std::make_unique<QChartLabelDrawer>(), parent };
    // chview->resize(700, 700);
    chview->setModel(arincModel.get());
    // chview->show();
    //  endtest

    // test2
    parent->centralWidget()->layout()->addWidget(chview);

    // endtest2

    CreateRawOutput();
    CreateLabelsInfo();
    CreateFilter();
}

Output::~Output() = default;

void
Output::CreateRawOutput()
{
    rawMessages = new QListWidget{ tabWgt };
    rawMessages->setUniformItemSizes(true);   // for better performance
    rawMessages->setAlternatingRowColors(true);

    tabWgt->addTab(rawMessages, "Raw");
}

void
Output::CreateLabelsInfo()
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
Output::CreateFilter()
{
    tabWgt->addTab(new LabelFilterView(arincModel), "filter");
}

void
Output::NormalizeRawData(const auto &data, QString &appendHere)
{
    arinc->NormalizeAndStoreMsg(data);

    const auto &decoded = arinc->LastMsg();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded->channel)
                        .arg(decoded->labelRaw)
                        .arg(decoded->SDI)
                        .arg(decoded->valueRaw)
                        .arg(decoded->SSM)
                        .arg(decoded->parity)
                        .arg(decoded->DTtimeRaw));

    arinc->SetLastReadedMsgNumber(decoded->msgNumber);
}

void
Output::ShowDiagram()
{
    const auto &data = arinc->LastMsg();
    arincChart->Append(data);
}

void
Output::ShowNewData(void)
{
    if (rawData.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    rawData.pop_front_wait(data);

    QString rawOutput;

    rawOutput.append(QString{ "#%1 " }.arg(data->msgIdx));
    rawOutput.append(data->msg_arrival_time.toString(" hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");

    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);

    rawMessages->addItem(rawOutput);

    // test
    // if (not arincChart->IsSomeLabelSelected()) {
    //    rawMessages->scrollToBottom();
    //}

    // ShowDiagram();

    // labelsInfo->Update(arinc->LastMsg(),
    //                    arinc->labels.at(arinc->LastMsg()->label).size(),
    //                    arincChart->GetLabelMarker(arinc->LastMsg()->labelRaw));
    // endtest

    arincModel->InsertNewMessage(arinc->LastMsg());

    if (labelsInfo->ShouldBeepOnArival(arinc->LastMsg()->labelRaw))
        Beep((arinc->LastMsg()->labelRaw + 100) * 4, 100);
}

bool
Output::SaveSession()
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
Output::ScrollAndSelectMsg(size_t item)
{
    rawMessages->scrollToItem(rawMessages->item(arincChart->GetIdxOfSelectedMessage()));
    rawMessages->setCurrentRow((arincChart->GetIdxOfSelectedMessage()));
}
