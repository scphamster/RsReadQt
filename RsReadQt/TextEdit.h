#pragma once
#include <qtextedit.h>
#include <qevent.h>
#include <qplaintextedit.h>
// TestPurpouse
#include <vector>
#include <algorithm>
//! TestPurpouse

class TextEdit : public QPlainTextEdit {
    Q_OBJECT
  public:
    TextEdit(QWidget *parent = nullptr);

    std::vector<QEvent::Type> evTypes = std::vector<QEvent::Type>(50);

    //bool event(QEvent *ev) override
    //{
    //    if (std::find_if(evTypes.begin(), evTypes.end(), [&ev](auto &item) { return (item == ev->type()); }) == evTypes.end()) {
    //        evTypes.push_back(ev->type());
    //    }

    //    return true;
    //}

    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (ev->type() == QEvent::GraphicsSceneWheel) {
            return false;
        }

        return QObject::eventFilter(obj, ev);
    }

    void wheelEvent(QWheelEvent *ev) override { }

    void dragLeaveEvent(QDragLeaveEvent *ev) override { }

    void mousePressEvent(QMouseEvent *ev) override { }

    void mouseMoveEvent(QMouseEvent* ev) override
    {
        constexpr auto sizeGripWidth = 20;
        auto           halfWidth     = size().width() / 2;
        if (((ev->pos().x() - halfWidth) < sizeGripWidth) || ((halfWidth - ev->pos().x()) < sizeGripWidth)) {
            return;
        }
    }
};
