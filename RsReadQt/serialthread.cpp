#include "serialthread.h"

#include <QTime>
#include <QTextBlock>
#include <qtextedit.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <QFile>
#include <QFileDialog>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <qvalueaxis.h>
#include <QLineSeries>
#include <QPixMap>
#include <QPainter>
#include <QList>

#include <QAbstractAxis>
#include <QValueAxis>
#include <QXYSeries>
#include <QScatterSeries>
#include <QSplineSeries>

#include <windows.h>

ReadingThread::ReadingThread(std::shared_ptr<void> databidgeData, deque_s<std::shared_ptr<::dataPacket>> &data, QObject *parent)
  : serDevice(std::static_pointer_cast<SerialConfigs>(databidgeData), data)
  , QThread(parent)
{ }

// ReadingThread::~ReadingThread()
//{
//     serDevice.Stop();
// }

void
ReadingThread::quit()
{
    serDevice.Stop();
    QThread::quit();
    // ReadingThread::~ReadingThread();
}

void
ReadingThread::run()
{
    assert(serDevice.Open() == 1);

    while (serDevice.IsOpen()) {
        if (serDevice.Read()) {
            emit notificationDataArrived();
        }
    }
}

uint8_t
DTMsgElement::reverseBits(uint8_t data)
{
    data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
    data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
    data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
    return data;
}

OutputThread::OutputThread(deque_s<std::shared_ptr<::dataPacket>> &data,
                           QPlainTextEdit                         *lhTxt,
                           QPlainTextEdit                         *rhTxt,
                           QPlainTextEdit                         *decodedTxt,
                           std::shared_ptr<void>                   dataBridgeConfigs,
                           DataChart                              *chart)
  : dataToOutput{ data }
  , rawOutputTxt{ lhTxt }
  , asciiOutputTxt{ rhTxt }
  , decodedOutputTxt(decodedTxt)
  , diagram(chart)
{
    decodeConfigsFile = std::static_pointer_cast<SerialConfigs>(dataBridgeConfigs)->GetConfigsFileName();
    assert(decodeConfigsFile != QString{});
    GetDecodeConfigsFromFile();

    if (diagram->isInitialized)
        return;

    diagram->chart->setTitle("Labels");

    diagram->yaxis = new QValueAxis;
    diagram->xaxis = new QValueAxis;

    diagram->chart->addAxis(diagram->yaxis, Qt::AlignLeft);
    diagram->chart->addAxis(diagram->xaxis, Qt::AlignBottom);

    auto valuesRange = 50;

    diagram->yaxis->setRange(0, valuesRange);
    diagram->xaxis->setRange(0, valuesRange);

    //diagram->series = new QLineSeries;
    //diagram->series->setPen(QPen(QColor(Qt::GlobalColor::black)));
    //diagram->series->setMarkerSize(60);

    //auto marker = new QImage(30, 60, QImage::Format::Format_ARGB32);
    //auto painte = new QPainter(marker);

    //painte->setBrush(QBrush(QColor(250, 0, 0, 250)));
    //painte->drawRect(0, 0, 30, 60);
    //painte->rotate(-10);
    //painte->setBrush(QBrush(QColor(255, 255, 255, 255)));
    //painte->setPen(QColor(255, 255, 255, 255));
    //painte->drawText(QPoint(0, 50), "Dupa123456");
    //painte->rotate(30);

    //diagram->series->setLightMarker(*marker);
    //diagram->chart->addSeries(diagram->series);
    //diagram->series->attachAxis(diagram->xaxis);
    //diagram->series->attachAxis(diagram->yaxis);
    
    diagram->isInitialized = true;
}

OutputThread::~OutputThread()
{
    // delete diagram->xaxis;
    // delete diagram->yaxis;
    // delete diagram->series;
}

void
OutputThread::AddLabelToDiagram(int labelIdx)
{
    if (diagram->labelsSeries.contains(labelIdx))
        return;

    auto &series = diagram->labelsSeries[labelIdx];

    

    auto labelMarker = new QImage(80, 80, QImage::Format::Format_ARGB32);
    auto painter = new QPainter(labelMarker);

    labelMarker->fill(QColor(0, 0, 0, 0));
    painter->setBrush(QBrush(QColor(rand() % 255, rand() % 255, rand() % 255, 255)));
    painter->setBackground(QBrush(QColor(255,255,255,0)));
    painter->drawRect(QRect(0, 0, 10, 80));

    series.setMarkerSize(80);
    series.setLightMarker(*labelMarker);
    //series->setPen(QPen(QColor(Qt::GlobalColor::black)));
    //series->setBrush(QBrush(*labelMarker));

    diagram->chart->addSeries(&diagram->labelsSeries[labelIdx]);

    series.attachAxis(diagram->yaxis);
    series.attachAxis(diagram->xaxis);

}

void
OutputThread::ShowDiagram()
{
    auto data = messages.back();

    auto time    = QTime::currentTime();
    auto rawTime = time.minute() * 6000 + time.second() * 100 + time.msec() / 10;

    if (!diagram->labelsSeries.contains(data.labelRaw))
        AddLabelToDiagram(data.labelRaw);
    
    diagram->labelsSeries[data.labelRaw].append(QPointF(rawTime, data.labelRaw));

    if (diagram->yaxis->max() < data.labelRaw) {
        diagram->yaxis->setMax(data.labelRaw + 50);
    }

    if (diagram->xaxis->max() < rawTime) {
        diagram->xaxis->setMax(rawTime + 500);
        diagram->xaxis->setMin(rawTime - 1000);
    }

    Beep(data.labelRaw * 4, 50);

    //diagram->series->append(QPointF(rawTime, data.valueRaw));

    //if (diagram->yaxis->max() < data.valueRaw) {
    //    diagram->yaxis->setMax(data.valueRaw + (diagram->yaxis->max() - diagram->yaxis->min() / 5));
    //}

    //if (diagram->xaxis->max() < rawTime) {
    //    diagram->xaxis->setMax(rawTime + 5);
    //    diagram->xaxis->setMin(rawTime - 10);
    //}
}

void
OutputThread::ShowNormalizedRawData(auto data)
{
    NormalizeAndStoreMsg(data);
    QString decodedOut;
    auto    decoded = messages.back();

    decodedOut.append("Channel ");
    decodedOut.append(QString::number(decoded.channel));

    decodedOut.append(" Label ");
    decodedOut.append(QString::number(decoded.labelRaw));

    decodedOut.append(" Data ");
    decodedOut.append(QString::number(decoded.valueRaw));

    decodedOut.append(" Time ");
    decodedOut.append(QString::number(decoded.timeRaw));

    decodedOutputTxt->appendPlainText(decodedOut);
}

void
OutputThread::ShowNewData(void)
{
    if (dataToOutput.empty() == true) {
        return;
    }

    auto data = std::make_shared<dataPacket>();
    dataToOutput.pop_front_wait(data);

    ShowNormalizedRawData(data);
    ShowDiagram();

    QString rawOutput;
    QString asciiOutput;

    rawOutput.append("\n");
    rawOutput.append(" #");
    rawOutput.append(QString::number(data->msg_counter));
    rawOutput.append(QTime::currentTime().toString("  hh:mm:ss:zzz "));
    rawOutput.append("RawData: ");
    // TODO: use vector range for loop
    for (int i = 0; (i < data->bytes_in_buffer) && (i < sizeof(data->data)); i++) {
        rawOutput.append(QString::number((data->data[i])));
        rawOutput.append(" ");
    }

    asciiOutput.append(QString("\nMsg number: "));
    asciiOutput.append(QString::number(data->msg_counter));
    asciiOutput.append(QString(":\n"));
    asciiOutput.append(QByteArray(data->data._Unchecked_begin(), data->bytes_in_buffer));

    rawOutputTxt->appendPlainText(rawOutput);
    asciiOutputTxt->appendPlainText(asciiOutput);

    auto asciiLineCount = asciiOutputTxt->document()->lineCount();
    auto rawLineCount   = rawOutputTxt->document()->lineCount();
    auto lineDiff       = asciiLineCount - rawLineCount;
    if (lineDiff != 0) {
        auto equalizer = QString("");

        if (lineDiff > 1) {
            lineDiff--;   // insertion to TextEdit makes newline itself
            equalizer.append(QString(qAbs(lineDiff), '\n'));
        }

        lineDiff > 0 ? rawOutputTxt->appendPlainText(equalizer) : asciiOutputTxt->appendPlainText(equalizer);
    }
}

bool
OutputThread::SaveSession()
{
    auto saveto_fileAddress = QFileDialog::getSaveFileName(nullptr, tr("Save session to file"), "LastSession.txt");

    QString messagesText;

    messagesText.append(decodedOutputTxt->toPlainText());
    QFile lastSessionFile(saveto_fileAddress);

    lastSessionFile.open(QIODeviceBase::OpenModeFlag::Append);
    lastSessionFile.write(messagesText.toLocal8Bit(), messagesText.length());
    lastSessionFile.close();

    return true;
}

void
DecodedData::GetDecodeConfigsFromFile()
{
    auto jsonFile = QFile(decodeConfigsFile);
    jsonFile.setPermissions(QFileDevice::Permission::ReadGroup);
    jsonFile.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    auto fileContents = jsonFile.readAll();
    jsonFile.close();

    DTMessageStructure.clear();

    QJsonParseError err;
    QJsonDocument   jsondoc{ QJsonDocument::fromJson(fileContents, &err) };

    auto obj           = jsondoc.object();
    auto dataStructure = obj["Data structure"].toObject();

    auto bytesInMsg    = dataStructure.value("Message length");
    auto msgElemsNames = dataStructure.value("Message parts").toArray();

    for (auto name : msgElemsNames) {
        DTMsgElement elem;
        auto         elemConfigs = dataStructure.value(name.toString()).toObject();

        elem.length = elemConfigs.value("Length").toInt();

        if (elem.length > 1) {
            if (elemConfigs.value("Byte order").toString() == "MSB") {
                elem.byteOrder = DTMsgElement::ByteOrder::MSB;
            }
            else {
                elem.byteOrder = DTMsgElement::ByteOrder::LSB;
            }
        }

        if (elemConfigs.value("Bit order").toString() == "MSB") {
            elem.bitOrder = DTMsgElement::BitOrder::MSB;
        }
        else {
            elem.bitOrder = DTMsgElement::BitOrder::LSB;
        }

        auto activeBits        = elemConfigs.value("Active bits").toArray();
        elem.activeBits.first  = activeBits.at(0).toInt();
        elem.activeBits.second = activeBits.at(1).toInt();

        elem.startbyte = elemConfigs.value("Byte index").toInt();

        if (elemConfigs.value("Format").toString() == "BIN")
            elem.dataFormat = DTMsgElement::DataFormat::BIN;
        else if (elemConfigs.value("Format").toString() == "DEC")
            elem.dataFormat = DTMsgElement::DataFormat::DEC;
        else if (elemConfigs.value("Format").toString() == "OCT")
            elem.dataFormat = DTMsgElement::DataFormat::OCT;
        else if (elemConfigs.value("Format").toString() == "HEX")
            elem.dataFormat = DTMsgElement::DataFormat::HEX;
        else if (elemConfigs.value("Format").toString() == "BCD")
            elem.dataFormat = DTMsgElement::DataFormat::BCD;

        auto elemName                = name.toString();
        DTMessageStructure[elemName] = std::move(elem);
    }
}

void
DecodedData::NormalizeAndStoreMsgItem(std::shared_ptr<dataPacket> data, DTMsgElement &elemStructure, auto &container)
{
    assert(elemStructure.length > 0);
    assert(elemStructure.activeBits.first < elemStructure.activeBits.second);
    assert(elemStructure.startbyte >= 0 && elemStructure.startbyte < data->bytes_in_buffer);

    container = 0;

    auto reordreBitsBytes = [&elemStructure, &container](auto &iterator) {
        for (int i = 0; i < elemStructure.length; i++) {
            uint8_t byte;

            if (elemStructure.bitOrder == DTMsgElement::BitOrder::MSB) {
                byte = DTMsgElement::reverseBits(*iterator);
            }
            else
                byte = *iterator;

            container |= byte << (i * 8);
            iterator++;
        }
    };

    if (elemStructure.byteOrder == DTMsgElement::ByteOrder::MSB && elemStructure.length > 1) {
        auto iterator =
          std::make_reverse_iterator(data->data.begin() + elemStructure.startbyte + (elemStructure.length - 1)) - 1;
        reordreBitsBytes(iterator);
    }
    else {
        auto iterator = data->data.begin() + elemStructure.startbyte;
        reordreBitsBytes(iterator);
    }
}

void
DecodedData::NormalizeAndStoreMsg(std::shared_ptr<dataPacket> rawData)
{
    ArincMessage msg;

    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Channel"], msg.channel);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Label"], msg.labelRaw);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Data"], msg.valueRaw);
    NormalizeAndStoreMsgItem(rawData, DTMessageStructure["Time"], msg.timeRaw);

    messages.push_back(msg);
}