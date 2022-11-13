#include "widgets/registerswidget.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>

#include "fonts.h"

namespace vicedebug {

namespace {

QLineEdit* createLineEdit(int len) {
    QLineEdit* e= new QLineEdit();
    e->setFont(Fonts::robotoMono());
    e->setMaxLength(len);
    e->setAlignment(Qt::AlignHCenter);
    e->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // No idea how much the border in a QLineEdit actually is... Let's assume
    // that an additional character is sufficient to cover that.
    int minW = e->fontMetrics().boundingRect("W").width() * (len+1);
    e->setFixedWidth(minW);

    e->setEnabled(false);

//    e->setText(QString("0").repeated(len));
    return e;    
}

}

RegistersWidget::RegistersWidget(Controller* controller, QWidget* parent) :
    QGroupBox("Registers", parent),
    controller_(controller)
{
    QGridLayout* layout = new QGridLayout();
    setLayout(layout);

    layout->addWidget(new QLabel("PC"), 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel("SP"), 0, 1, Qt::AlignHCenter);
    layout->addWidget(new QLabel("A"), 0, 2, Qt::AlignHCenter);
    layout->addWidget(new QLabel("X"), 0, 3, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Y"), 0, 4, Qt::AlignHCenter);
    layout->addWidget(new QLabel("NV-BDIZC"), 0, 5, Qt::AlignHCenter);

    pc_ = createLineEdit(4);
    sp_ = createLineEdit(4);
    a_ = createLineEdit(2);
    x_ = createLineEdit(2);
    y_ = createLineEdit(2);
    flags_ = createLineEdit(8);

    layout->addWidget(pc_, 1, 0, Qt::AlignHCenter);
    layout->addWidget(sp_, 1, 1, Qt::AlignHCenter);
    layout->addWidget(a_, 1, 2, Qt::AlignHCenter);
    layout->addWidget(x_, 1, 3, Qt::AlignHCenter);
    layout->addWidget(y_, 1, 4, Qt::AlignHCenter);
    layout->addWidget(flags_, 1, 5, Qt::AlignHCenter);

    connect(controller_, &Controller::publishEvent, this, &RegistersWidget::onEventFromController);


//    DisassemblyWidget

//    controllerEventListener_ = [this] (const Event& ev) {
//        this->onEventFromController(ev);
//    };
//    controller_->addListener(&controllerEventListener_);
}

RegistersWidget::~RegistersWidget() {
//    controller_->removeListener(&controllerEventListener_);
}

void RegistersWidget::enableControls(bool enable) {
    a_->setEnabled(enable);
    x_->setEnabled(enable);
    y_->setEnabled(enable);
    sp_->setEnabled(enable);
    pc_->setEnabled(enable);
    flags_->setEnabled(enable);
}

void RegistersWidget::onEventFromController(const Event& event) {
    qDebug() << "RegistersWidget::onEventFromController called";
    enableControls(event.connected);
    if (event.connected) {
        a_->setText(QString::asprintf("%02x",event.machineState.regA));
        x_->setText(QString::asprintf("%02x",event.machineState.regX));
        y_->setText(QString::asprintf("%02x",event.machineState.regY));
        pc_->setText(QString::asprintf("%02x",event.machineState.pc));
        sp_->setText(QString::asprintf("%02x",event.machineState.sp));

        QString flags = "";
        for (int m = 128; m > 0; m >>= 1) {
            if (m == 32) {
                flags += "-";
                continue;
            }
            flags += (event.machineState.flags & m) > 0 ? "1" : "0";
        }
        flags_->setText(flags);
    } else {
        a_->setText("");
        x_->setText("");
        y_->setText("");
        pc_->setText("");
        sp_->setText("");
    }
}

}

