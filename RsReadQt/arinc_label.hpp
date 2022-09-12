#pragma once
#include "arinc_model_configs.hpp"

class _ArincLabel {
  public:
    enum {
        Undefined       = -1,
        LabelNumberBase = 8
    };

    _ArincLabel();

    _ArincLabel(LabelNumT labelNumber, int number_base = LabelNumberBase);

    _ArincLabel(const LabelTxtT &label, int number_base = LabelNumberBase);

    bool operator==(const _ArincLabel &rhLabel);

    void Set(LabelNumT numb, int number_base = LabelNumberBase);

    void Set(const LabelTxtT &lbl, int number_base = LabelNumberBase);

    void Set(const _ArincLabel &lbl);

    [[nodiscard]] LabelNumT GetNumber() const noexcept;
    [[nodiscard]] LabelTxtT GetString() const noexcept;

  private:
    LabelNumT asNumber = Undefined;
    LabelTxtT asText   = LabelTxtT{ "Undefined" };
};