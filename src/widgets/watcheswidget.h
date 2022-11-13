#pragma once

#include <QGroupBox>
#include <QTableWidget>
#include <QToolButton>

#include "controller.h"

namespace vicedebug {

class WatchesWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit WatchesWidget(Controller* controller, QWidget* parent);
    ~WatchesWidget();

public slots:
    void onEventFromController(const Event& event);

private:
    Controller* controller_;

    QTableWidget* table_;
    QToolButton* addBtn_;
    QToolButton* removeBtn_;
};

}
