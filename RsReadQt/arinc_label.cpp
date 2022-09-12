#include "arinc_label.hpp"

_ArincLabel::_ArincLabel() = default;

_ArincLabel::_ArincLabel(LabelNumT labelNumber, int number_base /*= LabelNumberBase*/)
  : asNumber{ labelNumber }
  , asText{ QString::number(labelNumber, number_base) }
{ }

_ArincLabel::_ArincLabel(const LabelTxtT &label, int number_base /*= LabelNumberBase*/)
  : asNumber{ label.toInt(nullptr, number_base) }
  , asText{ label }
{ }

bool
_ArincLabel::operator==(const _ArincLabel &rhLabel)
{
    return (asNumber == rhLabel.asNumber and asText == rhLabel.asText);
}

void
_ArincLabel::Set(LabelNumT numb, int number_base /*= LabelNumberBase*/)
{
    asNumber = numb;
    asText   = LabelTxtT::number(numb, number_base);
}

void
_ArincLabel::Set(const LabelTxtT &lbl, int number_base /*= LabelNumberBase*/)
{
    asText   = lbl;
    asNumber = lbl.toInt(nullptr, number_base);
}

void
_ArincLabel::Set(const _ArincLabel &lbl)
{
    asNumber = lbl.GetNumber();
    asText   = lbl.GetString();
}

[[nodiscard]] LabelNumT
_ArincLabel::GetNumber() const noexcept
{
    return asNumber;
}
[[nodiscard]] LabelTxtT
_ArincLabel::GetString() const noexcept
{
    return asText;
}