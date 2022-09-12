#pragma once
#include <type_traits>
#include <concepts>
#include <QString>

class QImage;
class QDateTime;

namespace ArincQModel
{
using ChannelNumT = int;
using LabelNumT   = int;
using LabelTxtT   = QString;
using ColumnNumT  = int;
using RowNumT     = int;
using ValueT      = uint64_t;
using ssmT        = uint8_t;
using ParityT     = uint8_t;
using TimeT       = QDateTime;
using DTRawTime   = uint64_t;
using sdiT        = uint8_t;
using MsgNumT     = uint64_t;
}

using Column = int;
using Row = int;
using LabelNumT = int;
using LabelTxtT = QString;
using HitCounterT = size_t;
using ImageType = QImage;

constexpr int    LabelImageSize                     = 200;
constexpr double SelectedLabelImageEdgeWidthPercent = 0.25;
constexpr double LabelImageFontSizePercent          = 0.25;

class _ArincLabel;

template<typename T>
concept Appendable = requires(T t)
{
    t.append(T());
};

template<typename PairType, typename MapType, typename ValueType>
concept IsGood = requires
{
    std::same_as<PairType, typename MapType::value_type>;
    std::same_as<ValueType, typename MapType::mapped_type>;
};

template<typename T>
concept ArincLabelCompatible = std::same_as<T, LabelTxtT> or std::same_as<T, LabelNumT> or std::same_as<T, _ArincLabel>;