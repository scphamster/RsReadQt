#include <QMainWindow>

#include "arinc_chart.hpp"

void
LineSeries::SetMarkerVisibility(bool make_visible)
{
    if (make_visible == isVisible)
        return;

    if (make_visible == false) {
        _lightMarker         = lightMarker();
        _selectedLightMarker = selectedLightMarker();

        setLightMarker(QImage{});
        setSelectedLightMarker(QImage{});

        isVisible = make_visible;
    }
    else {
        setLightMarker(_lightMarker);
        setSelectedLightMarker(_selectedLightMarker);
        isVisible = make_visible;
    }
}

ArincLabelsChart::ArincLabelsChart(QWidget *mainWindow)
  : QWidget(mainWindow)
{
    // TODO: add goto field under chart for quick navigation to given value on xaxis
    // TODO: add filtering functions 1) filtering 2) beep on arivall
    // TODO: add right click on label action

    chart  = new QChart{};
    chview = new QChartView{ chart, this };
    chart->setParent(chview);
    // chart->setTitle("Labels");
    chart->legend()->setVisible(false);

    chart->setTheme(QChart::ChartTheme::ChartThemeBrownSand);
    chview->setParent(this);
    chview->setRenderHint(QPainter::RenderHint::Antialiasing, true);

    hscroll = new ScrollBar{ Qt::Orientation::Horizontal, this };
    vscroll = new ScrollBar{ Qt::Orientation::Vertical, this };

    dynamic_cast<QMainWindow *>(mainWindow)->centralWidget()->layout()->addWidget(this);

    vaxis = new QValueAxis{ chart };
    haxis = new QValueAxis{ chart };
    vaxis->setTickCount(10);
    haxis->setTickCount(10);
    hscroll->setMinimum(0);
    vscroll->setMinimum(0);
    chart->addAxis(vaxis, Qt::AlignLeft);
    chart->addAxis(haxis, Qt::AlignBottom);

    autoRangeChBox = new QCheckBox{ tr("Automatic diagram scroll"), this };
    autoRangeChBox->setGeometry(QRect{ 20, 10, 200, 30 });
    autoRangeChBox->setCheckState(Qt::CheckState::Checked);
    // autoRangeChBox->setFont(QFont{})

    startOfOperation = QDateTime::currentDateTime();
    // TODO: make palette

    connect(vaxis, &QValueAxis::rangeChanged, this, [this](qreal min, qreal max) {
        _AdjustScrollToAxis(vscroll, min, max);
    });
    connect(haxis, &QValueAxis::rangeChanged, this, [this](qreal min, qreal max) {
        _AdjustScrollToAxis(hscroll, min, max);
    });
    connect(vscroll, &ScrollBar::valueChanged, this, [this](int value) { _AdjustAxisToScroll(vaxis, vscroll, value); });
    connect(hscroll, &ScrollBar::valueChanged, this, [this](int value) { _AdjustAxisToScroll(haxis, hscroll, value); });
    connect(autoRangeChBox, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::CheckState::Checked)
            manualAxisRange = false;
        else
            manualAxisRange = true;
    });

    chview->installEventFilter(this);
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

    if (not  labelsSeries.contains(label))
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

void
ArincLabelsChart::AddLabel(int channel, int labelIdx)
{
    if (labelsSeries.contains(labelIdx))
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

    labelsSeries[labelIdx].first = new LineSeries{ chart };
    auto &series                 = *labelsSeries[labelIdx].first;

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

    chart->addSeries(labelsSeries[labelIdx].first);

    series.attachAxis(vaxis);
    series.attachAxis(haxis);

    QObject::connect(&series, &QXYSeries::released, this, &ArincLabelsChart::OnLabelOnChartSelected);
}

void
ArincLabelsChart::resizeEvent(QResizeEvent *evt)
{
    constexpr auto scroll_width      = 10;
    constexpr auto left_right_indent = 8;

    chview->resize(evt->size());

    hscroll->setGeometry(
      QRect{ left_right_indent, size().height() - scroll_width, size().width() - 2 * left_right_indent, scroll_width });
    vscroll->setGeometry(
      QRect{ size().width() - scroll_width, left_right_indent, scroll_width, size().height() - 2 * left_right_indent });
}

void
ArincLabelsChart::wheelEvent(QWheelEvent *evt)
{
    constexpr qreal single_wheel_tick_value = 120;

    auto pagesize = hscroll->pageStep();
    pagesize -= static_cast<qreal>(evt->angleDelta().y()) / (single_wheel_tick_value / 2);

    if (pagesize < 1)
        pagesize = 1;

    if (pagesize > hmax) {
        pagesize = hmax;
    }

    auto midval     = haxis->min() + (haxis->max() - haxis->min()) / 2;
    auto left_bound = midval - static_cast<qreal>(pagesize) / 2;

    if (left_bound < hmin)
        left_bound = hmin;

    auto right_bound = static_cast<int>(left_bound + pagesize);

    haxis->setRange(static_cast<int>(left_bound), static_cast<int>(right_bound));
}

void
ArincLabelsChart::_AdjustScrollToAxis(ScrollBar *scroll, qreal min, qreal max)
{
    constexpr qreal page_step_coeff   = 1;
    constexpr qreal single_step_coeff = 0.1;

    if (scroll->pageStep() != (max - min) * page_step_coeff) {
        auto page_step   = (max - min) * page_step_coeff;
        auto single_step = (max - min) * single_step_coeff;

        if (page_step < 1)
            page_step = 1;

        if (single_step < 1)
            single_step = 1;

        scroll->setSingleStep(static_cast<int>(single_step));
        scroll->setPageStep(static_cast<int>(page_step));
    }

    if (scroll->ShouldSkipValueAdjustment()) {
        scroll->SetSkipValueAjustment(false);
        return;
    }

    scroll->SetSkipValueChangedEvt(true);
    scroll->setValue(static_cast<int>(min));
}

void
ArincLabelsChart::_AdjustAxisToScroll(QValueAxis *axis, ScrollBar *scroll, int value)
{
    constexpr qreal minimumValue = 0;

    if (scroll->ShouldSkipValueChangedEvt()) {
        scroll->SetSkipValueChangedEvt(false);
        return;
    }

    scroll->SetSkipValueAjustment(true);
    axis->setRange(static_cast<qreal>(value), static_cast<qreal>(value + scroll->pageStep()));
}

void
ArincLabelsChart::Append(const ArincMsg &msg)
{
    constexpr auto  vscroll_overshoot                   = 30;
    constexpr auto  hscroll_overshoot                   = 10;
    constexpr qreal auto_x_range_change_olddata_percent = 0.3;

    if (not  labelsSeries.contains(msg.labelRaw))
        AddLabel(msg.channel, msg.labelRaw);

    auto msg_time = (-1) * static_cast<qreal>(msg.timeArrivalPC.msecsTo(startOfOperation)) / 1000;

    labelsSeries[msg.labelRaw].first->append(QPointF(msg_time, msg.labelRaw));
    labelsSeries[msg.labelRaw].second.push_back(std::pair<qreal, const ArincMsg &>(msg_time, msg));

    hmax = msg_time + hscroll_overshoot;
    hscroll->setMaximum(static_cast<int>(hmax));

    if ((vaxis->max() - vscroll_overshoot) < msg.labelRaw) {
        vmax = static_cast<int>(msg.labelRaw) + vscroll_overshoot;
        vscroll->setMaximum(vmax);
    }

    if (manualAxisRange || !labelsSeries[msg.labelRaw].first->MarkerIsVisible()) {
        return;
    }

    if ((vaxis->max() - vscroll_overshoot) < msg.labelRaw) {
        vaxis->setMax(static_cast<qreal>(vscroll->maximum()));
    }

    if (haxis->max() < msg_time || haxis->min() > msg_time) {
        int x_begin = static_cast<int>(msg_time - hscroll->pageStep() * auto_x_range_change_olddata_percent);

        haxis->setRange(x_begin, x_begin + hscroll->pageStep());
    }
}

void
ArincLabelsChart::SetLabelVisibility(int label, Qt::CheckState checkstate)
{
    bool make_visible = (checkstate == Qt::Checked) ? true : false;

    labelsSeries[label].first->SetMarkerVisibility(make_visible);

    chart->update();   // glitched repaint of labels without this call
}

bool
ArincLabelsChart::eventFilter(QObject *obj, QEvent *evt)
{
    if (evt->type() == QEvent::Type::MouseButtonPress) {
        idxOfSelectedMsg = ArincLabelsChart::ItemSelection::NOTSELECTED;

        if (selectedMsgSeriesAffinity != nullptr) {
            selectedMsgSeriesAffinity->deselectAllPoints();
            selectedMsgSeriesAffinity = nullptr;
        }

        return true;
    }

    return false;
}