#pragma once
#include <QLineSeries>

class LineSeries : public QLineSeries {
    Q_OBJECT

  public:
    template<typename... Args>
    explicit LineSeries(Args... args)
      : QLineSeries(std::forward<Args>(args)...)
    { }

    void SetMarkerVisibility(bool visible);
    bool MarkerIsVisible() { return isVisible; }

  private:
    QImage _lightMarker;
    QImage _selectedLightMarker;
    bool   isVisible = true;
};
