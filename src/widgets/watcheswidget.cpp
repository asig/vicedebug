#include "widgets/watcheswidget.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace vicedebug {

WatchesWidget::WatchesWidget(Controller* controller, QWidget* parent) :
    QGroupBox("Watches", parent), controller_(controller)
{
    table_ = new QTableWidget();

    addBtn_ = new QToolButton();
    addBtn_->setIcon(QIcon(":/images/codicons/add.svg"));

    removeBtn_ = new QToolButton();
    removeBtn_->setIcon(QIcon(":/images/codicons/remove.svg"));

    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->addWidget(addBtn_);
    vLayout->addWidget(removeBtn_);
    vLayout->addStretch(10);

//    vLayout->addSpacerItem(vLayout->spacerItem());

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(table_);
    hLayout->addLayout(vLayout);

    setLayout(hLayout);

    connect(controller_, &Controller::publishEvent, this, &WatchesWidget::onEventFromController);

}

WatchesWidget::~WatchesWidget()
{
}

void WatchesWidget::onEventFromController(const Event& event) {
    qDebug() << "WatchesWidget::onEventFromController called";
}


}
