#pragma once
#include <QScrollBar>

class ScrollBar : public QScrollBar {
    Q_OBJECT

  public:
    template<typename... Args>
    explicit ScrollBar(Args... args)
      : QScrollBar(std::forward<decltype(args)>(args)...)
    { }

    bool ShouldSkipValueAdjustment() { return skipValueAdjustment; }
    bool ShouldSkipValueChangedEvt() { return skipValueChangedEvt; }
    void SetSkipValueAjustment(bool skip) { skipValueAdjustment = skip; }
    void SetSkipValueChangedEvt(bool skip) { skipValueChangedEvt = skip; }

  private:
    bool skipValueAdjustment = false;
    bool skipValueChangedEvt = false;
};
