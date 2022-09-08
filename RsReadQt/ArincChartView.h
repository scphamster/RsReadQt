#pragma once

#include <QAbstractItemView>

#include "labelfilter.hpp"

class ArincChartView : public QAbstractItemView {
    Q_OBJECT
  public:
    ArincChartView() = default;
    ArincChartView(ArincLabelModel *model, QWidget *parent = nullptr);
    
    
    // chart



    // items interactions

  private:
};