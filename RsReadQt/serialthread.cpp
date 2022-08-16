#include "serialthread.h"

#include <algorithm>

#include <QMainWindow>

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
#include <qvalueaxis.h>
#include <QLineSeries>
#include <QPixMap>
#include <QPainter>
#include <QList>

#include <QScrollBar>
#include <QAbstractAxis>
#include <QValueAxis>
#include <QXYSeries>
#include <QScatterSeries>
#include <QSplineSeries>

//#include <windows.h>

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

    if (!makePause)
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

OutputThread::OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                           std::shared_ptr<void>                   dataBridgeConfigs,
                           QTabWidget                             *tabs,
                           QChart                                 *ch,
                           QChartView                             *chvw,
                           QMainWindow                            *parent)
  : dataToOutput{ data }
  , tabWgt{ tabs }
  , myParent(parent)
{
    diagram.chart     = ch;
    diagram.chview    = chvw;
    decodeConfigsFile = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName();

    diagram.chart->setLocalizeNumbers(true);

    assert(decodeConfigsFile != QString{});

    GetDecodeConfigsFromFile();

    diagram.startOfOperation = QDateTime::currentDateTime();

    listOfMessages = new QListWidget{ tabWgt };
    listOfMessages->setUniformItemSizes(true);
    tabWgt->addTab(listOfMessages, "raw");

    diagram.yaxis = new QValueAxis{ diagram.chart };
    diagram.yaxis->setTickCount(10);

    diagram.xaxis = new QValueAxis{ diagram.chart };

    diagram.chart->addAxis(diagram.yaxis, Qt::AlignLeft);
    diagram.chart->addAxis(diagram.xaxis, Qt::AlignBottom);
    diagram.chart->setAnimationDuration(100);

    auto valuesRange = 400;

    diagram.xaxis->setRange(0, 10);
    diagram.yaxis->setRange(0, valuesRange);

    diagram.chart->setTheme(QChart::ChartTheme::ChartThemeDark);
    diagram.chview->setRenderHint(QPainter::RenderHint::Antialiasing, true);

    // experimental
    //auto hscroll = new QScrollBar{ Qt::Orientation::Horizontal, diagram.chview };
    //auto vscroll = new QScrollBar{ Qt::Orientation::Vertical, diagram.chview };
    
    auto hscroll = new QScrollBar{ Qt::Orientation::Horizontal, diagram.chview };
    auto vscroll = new QScrollBar{ Qt::Orientation::Vertical, diagram.chview };
    
    hscroll->setRange(diagram.xaxis->min(), diagram.xaxis->max());
    vscroll->setRange(diagram.yaxis->min(), diagram.yaxis->max());
    hscroll->setVisible(true);
    hscroll->setGeometry(QRect(200, 200, 400, 220));



    //diagram.chview->setHorizontalScrollBar(hscroll);
    //diagram.chview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //diagram.chview->setVerticalScrollBar(vscroll);
    //diagram.chview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //diagram.chview->setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
    //~experimental

    connect(&diagram, &ArincLabelsChart::MsgOnChartBeenSelected, this, &OutputThread::ScrollAndSelectMsg);
    connect(vscroll, &QScrollBar::valueChanged, this, &OutputThread::testSlot);
    
    diagram.chview->installEventFilter(this);
}

OutputThread::~OutputThread()
{
    diagram.labelsSeries.clear();
}

void
OutputThread::AddLabelToDiagram(int msgNo, int channel, int labelIdx)
{
    if (diagram.labelsSeries.contains(labelIdx))
        return;

    constexpr int imgsize               = 200;
    constexpr int marker_size           = 40;
    constexpr int fontsize              = imgsize / 4;
    constexpr int channel_num_line_h    = imgsize / 2;
    constexpr int label_idx_line_h      = imgsize - imgsize / 10;
    constexpr int text_left_padding     = imgsize / 20;
    constexpr int sel_marker_edge_width = imgsize / 4;
    constexpr int rect_rounding_radius  = imgsize / 8;
    constexpr int marker_opacity        = 220;

    diagram.labelsSeries[labelIdx].first = new QLineSeries{ diagram.chart };
    auto &series                         = *diagram.labelsSeries[labelIdx].first;

    auto channel_text = QString{ "Ch: %1" }.arg(channel);
    auto label_text   = QString{ "%1" }.arg(labelIdx, -3, 8);
    auto text_font    = QFont{ "Monospace", fontsize };
    text_font.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    auto channel_txt_pos  = QPoint{ text_left_padding, channel_num_line_h };
    auto label_txt_pos    = QPoint{ text_left_padding, label_idx_line_h };
    auto background_brush = QBrush{ QColor{ 255, 255, 255, 0 } };

    auto marker_img = new QImage{ imgsize, imgsize, QImage::Format::Format_ARGB32 };
    marker_img->fill(QColor(0, 0, 0, 0));

    auto painter = QPainter{ marker_img };
    painter.setBackground(background_brush);

    auto gradient_filler = QLinearGradient{ QPointF{ 0, 0 }, QPointF{ imgsize, imgsize } };
    auto grad_color1     = QColor{ rand() % 200, rand() % 255, rand() % 255, marker_opacity };

    auto makeGradient = [](int value, int difference = 50) {
        value += difference;
        if (value > 255)
            value -= difference;

        return value % 255;
    };

    auto grad_color2 = QColor(makeGradient(grad_color1.red()),
                              makeGradient(grad_color1.green()),
                              makeGradient(grad_color1.blue()),
                              marker_opacity);
    gradient_filler.setColorAt(0, grad_color1);
    gradient_filler.setColorAt(1, grad_color2);
    painter.setBrush(QBrush{ gradient_filler });
    painter.setPen(QColor{ 255, 255, 255, 0 });
    painter.drawRoundedRect(QRect(0, 0, imgsize, imgsize), rect_rounding_radius, rect_rounding_radius);

    auto contrasting_text_color =
      QColor{ 255 - grad_color1.red(), 255 - grad_color1.green(), 255 - grad_color1.blue(), 255 };

    painter.setPen(contrasting_text_color);
    painter.setFont(text_font);
    painter.drawText(channel_txt_pos, channel_text);
    painter.drawText(label_txt_pos, label_text);
    series.setMarkerSize(marker_size);
    series.setLightMarker(*marker_img);

    auto sel_marker_img = new QImage(imgsize, imgsize, QImage::Format::Format_ARGB32);
    sel_marker_img->fill(QColor(0, 0, 0, 0));
    auto painter2 = QPainter{ sel_marker_img };
    painter2.setBrush(painter.brush());
    painter2.setPen(QPen{ QBrush{ QColor{ 255, 0, 0, 255 } }, sel_marker_edge_width });
    painter2.setBackground(QBrush{ QColor{ 255, 255, 255, 0 } });
    painter2.drawRoundedRect(QRect(0, 0, imgsize, imgsize), rect_rounding_radius, rect_rounding_radius);
    painter2.setFont(text_font);
    painter2.setPen(contrasting_text_color);
    painter2.drawText(channel_txt_pos, channel_text);
    painter2.drawText(label_txt_pos, label_text);
    series.setSelectedLightMarker(*sel_marker_img);
    series.setPen(QPen(QColor(0, 0, 0, 0)));
    // diagram.chart->

    diagram.chart->addSeries(diagram.labelsSeries[labelIdx].first);

    series.attachAxis(diagram.yaxis);
    series.attachAxis(diagram.xaxis);

    QObject::connect(&series, &QXYSeries::released, &diagram, &ArincLabelsChart::OnLabelOnChartSelected);
}

void
OutputThread::ShowDiagram()
{
    const auto &data             = messages.back();
    qreal       secondsFromStart = (-1) * static_cast<qreal>(data.timeArrivalPC.msecsTo(diagram.startOfOperation)) / 1000;

    if (!diagram.labelsSeries.contains(data.labelRaw))
        AddLabelToDiagram(data.msgNumber, data.channel, data.labelRaw);

    diagram.labelsSeries[data.labelRaw].first->append(QPointF(secondsFromStart, data.labelRaw));
    diagram.labelsSeries[data.labelRaw].second.push_back(std::pair<qreal, const ArincMsg &>(secondsFromStart, data));

    if (diagram.yaxis->max() < data.labelRaw) {
        diagram.yaxis->setMax(data.labelRaw + 50);
    }

    if (diagram.xaxis->max() < secondsFromStart) {
        diagram.xaxis->setMax(secondsFromStart + 5);
        diagram.xaxis->setMin(secondsFromStart - 10);
    }

    // Beep(data.labelRaw * 4, 50);
}

void
OutputThread::NormalizeRawData(const auto &data, QString &appendHere)
{
    NormalizeAndStoreMsg(data);

    const auto &decoded = messages.back();

    appendHere.append(QString{ "Channel %1, Label %2, SDI %3, Data %4, SSM %5, Parity %6, Time %7" }
                        .arg(decoded.channel)
                        .arg(decoded.labelRaw)
                        .arg(decoded.SDI)
                        .arg(decoded.valueRaw)
                        .arg(decoded.SSM)
                        .arg(decoded.parity)
                        .arg(decoded.DTtimeRaw));

    lastMsgReadedNum = decoded.msgNumber;
}

void
OutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data);

    QString rawOutput;

    rawOutput.append(QString{ "#%1  " }.arg(data->msg_counter));
    rawOutput.append(data->msg_arrival_time.toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    // TODO: use vector range for loop
    for (const auto &data : data->data) {
        rawOutput.append(QString::number(static_cast<uchar>(data)));
        rawOutput.append(" ");
    }

    NormalizeRawData(data, rawOutput);
    listOfMessages->addItem(rawOutput);

    if (diagram.idxOfSelectedMsg == ArincLabelsChart::ItemSelection::NOTSELECTED) {
        listOfMessages->scrollToBottom();
    }

    ShowDiagram();
}

void
OutputThread::ScrollAndSelectMsg(uint64_t item)
{
    listOfMessages->scrollToItem(listOfMessages->item(diagram.idxOfSelectedMsg));
    listOfMessages->setCurrentRow((diagram.idxOfSelectedMsg));
}

void
OutputThread::testSlot(int)
{ 
    
}

bool
OutputThread::SaveSession()
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

bool
OutputThread::eventFilter(QObject *obj, QEvent *evt)
{
    if (evt->type() == QEvent::Type::MouseButtonPress) {
        diagram.idxOfSelectedMsg = ArincLabelsChart::ItemSelection::NOTSELECTED;

        if (diagram.selectedMsgSeriesAffinity != nullptr) {
            diagram.selectedMsgSeriesAffinity->deselectAllPoints();
            diagram.selectedMsgSeriesAffinity = nullptr;
        }

        return true;
    }

    if (evt->type() == QEvent::Type::ScrollPrepare) {
        int i = 1;
        return true;
    }

    auto ev = dynamic_cast<QScrollEvent *>(evt);

    return false;
}

void
Arinc::GetDecodeConfigsFromFile()
{
    auto jsonfile = QFile{ decodeConfigsFile };
    jsonfile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonfile.readAll();
    jsonfile.close();

    DTMsgAnatomy.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    if (err.error != QJsonParseError::NoError) {
        QMessageBox errorMsg{ QMessageBox::Warning,
                              "Json file Error",
                              "Error occured during json file \" " + decodeConfigsFile +
                                "\" opening, Error: " + err.errorString() };

        assert(0);
        exit(-1);
    }

    auto mainObj   = jsondoc.object();
    auto configObj = mainObj["Configurations"].toObject();

    auto msgChunkNames         = configObj["Message parts names"].toArray();
    auto msgChunkNumber        = configObj["Message parts number"].toInt();
    auto msgLenBytes           = configObj["Message length bytes"].toInt();
    auto msgLenBits_calculated = msgLenBytes * 8 - 1;

    auto chunks = configObj["Parts"].toObject();

    DTMsgAnatomy.clear();

    for (auto chunkName : msgChunkNames) {
        auto configs = chunks[chunkName.toString()].toObject();

        DTWordField chunk;
        chunk.activeBits.first  = configs["Bit start"].toInt();
        chunk.activeBits.second = configs["Bit end"].toInt();

        if (chunk.activeBits.first > chunk.activeBits.second) {
            QMessageBox errorMsg{
                QMessageBox::Critical,
                "Json file Error",
                "Message part with name " + chunkName.toString() + "has starting bit number higher than ending bit number"

            };

            anatomyIsConfigured = false;
            return;
        }

        if (chunk.activeBits.second > msgLenBits_calculated || chunk.activeBits.first > msgLenBits_calculated) {
            QMessageBox errorMsg{
                QMessageBox::Critical,
                "Json file Error",
                "Message part with name: " + chunkName.toString() +
                  " has active bits number exceeding number of bits calculated from \"Mesaage length bytes\""
            };

            anatomyIsConfigured = false;
            return;
        }

        chunk.bitOrder =
          configs["Reverse bits?"].toBool() ? DTWordField::BitOrder::REVERSE : DTWordField::BitOrder::NORMAL;
        chunk.byteOrder =
          configs["Reverse bytes?"].toBool() ? DTWordField::ByteOrder::REVERSE : DTWordField::ByteOrder::NORMAL;

        auto format = configs["Encoding"].toString();

        if (format == "BIN")
            chunk.dataFormat = DTWordField::DataFormat::BIN;
        else if (format == "DEC")
            chunk.dataFormat = DTWordField::DataFormat::DEC;
        else if (format == "OCT")
            chunk.dataFormat = DTWordField::DataFormat::OCT;
        else if (format == "HEX")
            chunk.dataFormat = DTWordField::DataFormat::HEX;
        else if (format == "BCD")
            chunk.dataFormat = DTWordField::DataFormat::BCD;
        else {
            chunk.dataFormat = DTWordField::DataFormat::UNDEFINED;

            QMessageBox error{ QMessageBox::Critical,
                               "Json file Error",
                               "Message part with name: " + chunkName.toString() +
                                 " has undefined encoding. Valid types are: BIN, DEC, OCT, HEX, BCD" };
        }

        DTMsgAnatomy[chunkName.toString()] = std::move(chunk);
    }

    anatomyIsConfigured = true;
}

void
Arinc::NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTWordField &configs, auto &container)
{
    auto firstByteNum = configs.activeBits.first / 8;
    auto lastByteNum  = configs.activeBits.second / 8;
    int  incrementor;
    int  byteNumber = 0;

    if (configs.byteOrder == DTWordField::ByteOrder::NORMAL) {
        incrementor = 1;
    }
    else {
        incrementor = -1;

        auto temp    = firstByteNum;
        firstByteNum = lastByteNum;
        lastByteNum  = temp;
    }

    container = 0;
    for (int i = firstByteNum; true; i += incrementor) {
        auto byte    = static_cast<uint8_t>(data->data.at(i));
        auto byteNum = i - firstByteNum;

        if (configs.bitOrder == DTWordField::BitOrder::REVERSE) {
            byte = DTWordField::reverseBitsInByte(byte);
        }

        if (byteNumber > 7)
            while (1) { }

        container |= byte << (byteNumber * 8);

        byteNumber++;

        if (i == lastByteNum)
            break;
    }

    auto     activeBitsNum = configs.activeBits.second - configs.activeBits.first + 1;
    uint64_t mask          = (static_cast<uint64_t>(1 << activeBitsNum)) - 1;

    if (configs.byteOrder == DTWordField::ByteOrder::NORMAL) {
        auto firstByteFirstBitNum = configs.activeBits.first % 8;

        container >>= firstByteFirstBitNum;
        container &= mask;
    }
    else {
        uint64_t allButFirst = container & 0xFFffFFff00UL;

        auto lastByteUnusedBitsNum = configs.activeBits.first % 8;
        auto bytesNum              = firstByteNum - lastByteNum;

        uint64_t lastByte = container & (0xFF << (bytesNum * 8));
        lastByte >>= lastByteUnusedBitsNum;

        container &= ~(0xFF << (bytesNum * 8));
        container |= lastByte;

        auto firstByteUnusedBitsNum = 7 - configs.activeBits.second % 8;
        container >>= firstByteUnusedBitsNum;

        container &= mask;
    }
}

void
Arinc::NormalizeAndStoreMsg(std::shared_ptr<dataPacket> rawData)
{
    ArincMsg msg;
    if (decodeConfigsFile != QString{}) {
        msg.timeArrivalPC = rawData->msg_arrival_time;
        msg.msgNumber     = rawData->msg_counter;

        for (auto &msgChunk : DTMsgAnatomy) {
            if (msgChunk.second.activeBits.first / 8 >= rawData->bytes_in_buffer ||
                msgChunk.second.activeBits.second / 8 >= rawData->bytes_in_buffer) {
                QMessageBox err{ QMessageBox::Critical, "Data read error", "Not enough bytes in buffer" };
            }
        }

        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Channel"], msg.channel);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Label"], msg.labelRaw);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["SDI"], msg.SDI);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Data"], msg.valueRaw);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["SSM"], msg.SSM);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Parity"], msg.parity);
        NormalizeAndStoreMsgItem(rawData, DTMsgAnatomy["Time"], msg.DTtimeRaw);
    }

    messages.push_back(msg);
}

void
ArincLabelsChart::OnLabelOnChartSelected(const QPointF &point)
{
    GetDataFromLabelOnChart(point);
}

bool
ArincLabelsChart::GetDataFromLabelOnChart(const QPointF &atPoint)
{
    int label = static_cast<int>(atPoint.y());

    if (!labelsSeries.contains(label))
        return false;

    const auto &dataVector = labelsSeries.at(label).second;
    const auto  series     = labelsSeries.at(label).first;

    auto point_and_msg_itr = std::find_if(dataVector.rbegin(), dataVector.rend(), [&atPoint](const auto &pointAndData) {
        if (pointAndData.first == atPoint.x()) {
            return true;
        }
        else {
            return false;
        }
    });

    if (point_and_msg_itr != dataVector.rend()) {
        idxOfSelectedMsg = (point_and_msg_itr->second.msgNumber);

        const auto &points = series->points();

        // set point on chart which belongs to series "series" to selected state and remember that series for deselection
        // in future
        int  idx      = 0;
        auto iterator = std::find_if(points.begin(), points.end(), [&idx, &atPoint](auto &point) {
            if (point.x() == atPoint.x())
                return true;
            else
                idx++;

            return false;
        });

        if (iterator != points.end()) {
            if (selectedMsgSeriesAffinity != nullptr) {
                selectedMsgSeriesAffinity->deselectAllPoints();
            }

            series->setPointSelected(idx, true);

            selectedMsgSeriesAffinity = series;

            emit MsgOnChartBeenSelected(idxOfSelectedMsg);

            return true;
        }
    }
    return false;
}
