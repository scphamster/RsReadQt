#pragma once

#include <memory>
#include "arinc_model_configs.hpp"

template<size_t NumberOfDimensions = 1>
class ProxyArincLabelItem {
  public:
    using Counter        = HitCounterT;
    using ChildContainer = ChildContainer<ProxyArincLabelItem, NumberOfDimensions>;

    enum {
        Undefined = -1
    };

    enum class BehaveLike {
        QTreeItem,
        ArincItem
    };

    enum class ArincRole {
        NoRole = -1,
        Name,
        FirstOccurrence,
        LastOccurrence,
        HitCount,
        MakeBeep,
        ShowOnDiagram,
        Hide,
        ArincMessages,
        ArincMsg,
        _SIZE
    };

    ProxyArincLabelItem(std::shared_ptr<ArincData> data_of_item,
                        ProxyArincLabelItem       *parent_of_this_item = nullptr,
                        const ImageType           &marker              = ImageType{})
      : _data{ std::make_shared<CompositeArincItem>(data_of_item) }
      , parentItem{ parent_of_this_item }
    {
        if (marker == ImageType{})
            CreateLabelMarker(data_of_item->GetLabel<LabelNumT>(), data_of_item->GetChannelAffinity());
        else
            _data->SetTreeData(static_cast<Column>(ArincTreeData::ColumnRole::LabelNum),
                               Qt::ItemDataRole::DecorationRole,
                               marker);
    }

    ProxyArincLabelItem(const ProxyArincLabelItem &)            = default;
    ProxyArincLabelItem &operator=(const ProxyArincLabelItem &) = default;
    ProxyArincLabelItem(ProxyArincLabelItem &&)                 = default;
    ProxyArincLabelItem &operator=(ProxyArincLabelItem &&)      = default;
    ~ProxyArincLabelItem()                                      = default;

    [[nodiscard]] auto GetParent() const noexcept { return parentItem; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] auto GetColumnCount() const noexcept
    {
        return _data->GetTreeColumnCount();
    }

    template<>
    [[nodiscard]] auto GetColumnCount<BehaveLike::ArincItem>() const noexcept
    {
        // todo: remake
        return 0;
    }

    [[nodiscard]] auto GetChildrenCount() { return children.GetChildrenCount(); }

    [[nodiscard]] auto GetRow() const noexcept { return locatedAtRow; }

    [[nodiscard]] auto GetColumn() const noexcept { return locatedAtColumn; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    [[nodiscard]] Qt::ItemFlags GetFlags()
    {
        return _data->GetTreeFlags();
    }

    template<BehaveLike BehaviourType = BehaveLike::QTreeItem,
             typename RType           = QVariant,
             typename ColumnT         = Column,
             typename RoleT           = Qt::ItemDataRole>
    [[nodiscard]] RType GetData(ColumnT column, RoleT role)
    {
        return _data->GetTreeData(column, static_cast<Qt::ItemDataRole>(role));
    }

    template<>
    [[nodiscard]] std::shared_ptr<CompositeArincItem>
    GetData<BehaveLike::ArincItem, std::shared_ptr<CompositeArincItem>, Column, ArincRole>(Column column, ArincRole role)
    {
        return 0;
        // return data.GetArincData();
    }

    [[nodiscard]] std::shared_ptr<ArincData> GetArincData() const
      noexcept(std::is_nothrow_constructible_v<std::shared_ptr<ArincData>, std::shared_ptr<ArincData>>)
    {
        return _data->GetArincData();
    }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    void SetData(Column column, int role, const QVariant &value)
    {
        _data->SetTreeData(column, static_cast<Qt::ItemDataRole>(role), value);
    }

    template<>
    void SetData<BehaveLike::ArincItem>(Column column, int role, const QVariant &value)
    {
        // compositeItem->SetArincData()
    }

    void SetItemsRow(size_t newrow) noexcept
    {
        if (newrow > 0)
            locatedAtRow = newrow;
    }

    [[nodiscard]] auto GetParent() noexcept { return parentItem; }
    [[nodiscard]] auto InsertChild(ProxyArincLabelItem *child, Row row = 0) { return children.InsertChild(row, child); }
    [[nodiscard]] auto GetChild(Row at_row, Column at_column) { return children.GetChild(at_row); }

    void SetRow(auto new_row) noexcept { locatedAtRow = new_row; }

    template<BehaveLike BehaviourT = BehaveLike::QTreeItem>
    decltype(auto) SortChildren(Column column, Qt::SortOrder order)
    {
        return children.Sort(column, order, [column, order](const auto &first_child, const auto &second_child) {
            bool answer;

            decltype(first_child->_data->GetArincData()) ardata1;
            decltype(first_child->_data->GetArincData()) ardata2;

            if (order == Qt::SortOrder::AscendingOrder) {
                ardata1 = first_child->_data->GetArincData();
                ardata2 = second_child->_data->GetArincData();
            }
            else {
                ardata2 = first_child->_data->GetArincData();
                ardata1 = second_child->_data->GetArincData();
            }

            using Col = ArincTreeData::ColumnRole;
            switch (column) {
            case static_cast<Column>(Col::LabelNum):
                answer = ardata1->GetLabel<LabelNumT>() < ardata2->GetLabel<LabelNumT>();
                break;
            case static_cast<Column>(Col::FirstOccurrence):
                answer = ardata1->GetFirstOccurrence() < ardata2->GetFirstOccurrence();
                break;
            case static_cast<Column>(Col::LastOccurrence):
                answer = ardata1->GetLastOccurrence() < ardata2->GetLastOccurrence();
                break;
            case static_cast<Column>(Col::HitCount): answer = ardata1->GetHitCount() < ardata2->GetHitCount(); break;

            default: answer = false;
            }

            return answer;
        });
    }

    auto Contains(const _ArincLabel &label) const
    {
        return children.Contains(label, [&label](const auto &child) {
            if (child->_data->GetLabel() == label)
                return true;
            else
                return false;
        });
    }

    void CreateLabelMarker(LabelNumT label, int channel)
    {
        constexpr int imgsize               = 200;
        constexpr int marker_size           = 40;
        constexpr int fontsize              = imgsize / 4;
        constexpr int channel_num_line_h    = imgsize / 2;
        constexpr int label_idx_line_h      = imgsize - imgsize / 10;
        constexpr int text_left_padding     = imgsize / 20;
        constexpr int sel_marker_edge_width = imgsize / 4;
        constexpr int rect_rounding_radius  = imgsize / 8;
        constexpr int marker_opacity        = 220;

        auto channel_text = QString{ "Ch: %1" }.arg(channel);
        auto label_text   = QString{ "%1" }.arg(label, -3, 8);
        auto text_font    = QFont{ "Monospace", fontsize };
        text_font.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
        auto channel_txt_pos  = QPoint{ text_left_padding, channel_num_line_h };
        auto label_txt_pos    = QPoint{ text_left_padding, label_idx_line_h };
        auto background_brush = QBrush{ QColor{ 255, 255, 255, 0 } };

        // nonselected marker image creation
        auto marker_img = QImage{ imgsize, imgsize, QImage::Format::Format_ARGB32 };
        marker_img.fill(QColor(0, 0, 0, 0));

        auto painter = QPainter{ &marker_img };
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

        _data->SetTreeData(static_cast<Column>(ArincTreeData::ColumnRole::LabelNum),
                           Qt::ItemDataRole::DecorationRole,
                           QVariant{ std::move(marker_img) });
    }

    ImageType GetLabelMarker(LabelNumT label)
    {
        auto child = children.FindChild(label, [label](const auto &child) {
            if (child->_data->GetLabel<LabelNumT>() == label)
                return true;
            else
                return false;
        });

        if (child == nullptr)
            return QImage{};

        return qvariant_cast<QImage>(
          child->GetData<BehaveLike::QTreeItem>(static_cast<int>(ArincTreeData::ColumnRole::LabelNum),
                                                Qt::ItemDataRole::DecorationRole));
    }

    auto AppendMessage(std::shared_ptr<ArincMsg> msg, const _ArincLabel &for_label)
    {
        auto child = children.GetChildWithParam(for_label, [label = for_label](const auto &child) {
            if (child->_data->GetLabel() == label)
                return true;
            else
                return false;
        });

        if (child == nullptr)
            return static_cast<Row>(Undefined);

        child->_data->AppendMessage(msg);
        return child->GetRow();
    }

  private:
    std::shared_ptr<CompositeArincItem> _data;
    ChildContainer                      children;

    ProxyArincLabelItem *parentItem;
    Row                  locatedAtRow    = 0;
    Column               locatedAtColumn = 0;
};