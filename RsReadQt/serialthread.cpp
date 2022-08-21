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

#include "serialthread.h"
#include "arinc.hpp"
#include <Windows.h>

ReadingThread::ReadingThread(std::shared_ptr<void>                   databidgeData,
                             deque_s<std::shared_ptr<::dataPacket>> &data,
                             QObject                                *parent)
  : serDevice(std::static_pointer_cast<SerialConfigs>(databidgeData), data)
  , QThread(parent)
{ }

void
ReadingThread::quit()
{
    serDevice.Stop();
    QThread::quit();
}

void
ReadingThread::Pause(bool makePause)
{
    isPaused = makePause;

    if (not makePause)
        serDevice.Flush();
}

void
ReadingThread::run()
{
    // TODO: add error checking to serial
    serDevice.Open();

    while (serDevice.IsOpen()) {
        if (isPaused)
            continue;

        if (serDevice.Read()) {
            emit notificationDataArrived();
        }
    }
}

Output::Output(deque_s<std::shared_ptr<::dataPacket>> &data,
               std::shared_ptr<void>                   dataBridgeConfigs,
               QTabWidget                             *tabs,
               QMainWindow                            *parent)
  : rawData{ data }
  , tabWgt{ tabs }
  , myParent(parent)
  , arinc{ std::make_unique<Arinc>(std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName()) }
{
    // arincChart = new ArincLabelsChart{ parent };
    arincChart = std::make_unique<ArincLabelsChart>(parent);

    rawMessages = new QListWidget{ tabWgt };
    rawMessages->setUniformItemSizes(true);   // for better performance
    tabWgt->addTab(rawMessages, "Raw");

    labelsInfo = new LabelsInfo{ tabWgt };
    labelsInfo->setSortingEnabled(true);
    labelsInfo->sortByColumn(0, Qt::SortOrder::DescendingOrder);

    auto labels = new QTreeWidgetItem{};
    labels->setText(0, "Label");
    labels->setText(1, "First occurrence");
    labels->setText(2, "Last occurrence");
    labels->setText(3, "Hit count");
    labels->setText(4, "Make beep");
    labels->setText(5, "Show on diagram");
    labelsInfo->setHeaderItem(labels);

    tabWgt->addTab(labelsInfo, "Labels");

    labelFilter = new LabelFilter{ tabWgt };
    tabWgt->addTab(labelFilter, "Filter");

    connect(arincChart.get(), &ArincLabelsChart::MsgOnChartBeenSelected, this, &Output::ScrollAndSelectMsg);
    connect(labelsInfo, &LabelsInfo::itemChanged, labelsInfo, &LabelsInfo::OnLabelInfoChanged);
    connect(labelsInfo,
            &LabelsInfo::LabelVisibilityChoiceChanged,
            arincChart.get(),
            &ArincLabelsChart::SetLabelVisibility);
}

Output::~Output() { }

void
Output::NormalizeRawData(const auto &data, QString &appendHere)
{
    arinc->NormalizeAndStoreMsg(data);

    const auto &decoded = arinc->messages.back();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded.channel)
                        .arg(decoded.labelRaw)
                        .arg(decoded.SDI)
                        .arg(decoded.valueRaw)
                        .arg(decoded.SSM)
                        .arg(decoded.parity)
                        .arg(decoded.DTtimeRaw));

    arinc->lastMsgReadedNum = decoded.msgNumber;
}

void
Output::ShowDiagram()
{
    const auto &data = arinc->messages.back();
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

    rawOutput.append(QString{ "#%1 " }.arg(data->msg_counter));
    rawOutput.append(data->msg_arrival_time.toString(" hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");

    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);

    rawMessages->addItem(rawOutput);

    if (not arincChart->IsSomeLabelSelected()) {
        rawMessages->scrollToBottom();
    }

    ShowDiagram();

    labelsInfo->Update(arinc->LastMsg(),
                       arinc->labels.at(arinc->LastMsg().label).size(),
                       arincChart->GetLabelMarker(arinc->LastMsg().labelRaw));

    if (labelsInfo->ShouldBeepOnArival(arinc->LastMsg().labelRaw))
        Beep((arinc->LastMsg().labelRaw + 100) * 4, 100);
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
Output::ScrollAndSelectMsg(uint64_t item)
{
    rawMessages->scrollToItem(rawMessages->item(arincChart->GetIdxOfSelectedMessage()));
    rawMessages->setCurrentRow((arincChart->GetIdxOfSelectedMessage()));
}

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
LabelsInfo::Update(const ArincMsg &msg, int counter, QImage &&img)
{
    if (not labels.contains(msg.labelRaw)) {
        auto label = new QTreeWidgetItem{};

        label->setText(LabelsInfo::Column::Name, QString::number(msg.labelRaw, 8));
        label->setData(
          LabelsInfo::Column::Name,
          Qt::ItemDataRole::DecorationRole,
          img.scaled(20, 20, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        label->setText(LabelsInfo::Column::FirstOccurrence, msg.timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setText(LabelsInfo::Column::LastOccurrence, msg.timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setData(LabelsInfo::Column::HitCount, Qt::ItemDataRole::EditRole, 1);
        label->setCheckState(LabelsInfo::Column::MakeBeep, Qt::CheckState::Unchecked);
        label->setCheckState(LabelsInfo::Column::ShowOnDiagram, Qt::Checked);

        labels[msg.labelRaw] = label;

        addTopLevelItem(label);
    }
    else {
        skipDataChangeEvent = true;

        auto label = labels.at(msg.labelRaw);
        label->setText(2, msg.timeArrivalPC.toString(" hh:mm:ss:zzz "));
        label->setData(3, Qt::ItemDataRole::EditRole, counter);

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