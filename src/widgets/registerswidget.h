#pragma once

#include <QGroupBox>
#include <QTableWidget>
#include <QLineEdit>

#include "controller.h"

namespace vicedebug {

class RegistersWidget : public QGroupBox
{
    Q_OBJECT

public:
    RegistersWidget(Controller* controller, QWidget* parent);
    ~RegistersWidget();

public slots:
    void onEventFromController(const Event& event);

private:
    void enableControls(bool enable);

    Controller* controller_;

    QLineEdit* pc_;
    QLineEdit* sp_;
    QLineEdit* a_;
    QLineEdit* x_;
    QLineEdit* y_;
    QLineEdit* flags_;
};

}
