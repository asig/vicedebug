#pragma once

#include <QGroupBox>
#include <QTableWidget>
#include <QToolButton>

#include "controller.h"

namespace vicedebug {

class BreakpointsWidget : public QGroupBox
{
    Q_OBJECT

public:
    BreakpointsWidget(Controller* controller, QWidget* parent);
    ~BreakpointsWidget();

public slots:
    void onEventFromController(const Event& event);

private:
//    EventListener controllerEventListener_;
    Controller* controller_;

    QTableWidget* table_;
    QToolButton* addBtn_;
    QToolButton* removeBtn_;
};

}
