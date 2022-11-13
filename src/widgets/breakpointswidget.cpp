#include "widgets/breakpointswidget.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QStringList>

namespace vicedebug {

BreakpointsWidget::BreakpointsWidget(Controller* controller, QWidget* parent) :
    QGroupBox("Breakpoints", parent), controller_(controller)
{

    QStringList l;

    table_ = new QTableWidget();
    table_->setColumnCount(3);
    table_->setHorizontalHeaderLabels({ "Enabled","Address","Type"} );
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
//    table_->addScrollBarWidget()

    table_->insertRow(0);
    table_->insertRow(0);
    table_->insertRow(0);
    table_->insertRow(0);
    table_->insertRow(0);



    // Create a widget that will contain a checkbox
     QWidget *checkBoxWidget = new QWidget();
     QCheckBox *checkBox = new QCheckBox();      // We declare and initialize the checkbox
     QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget); // create a layer with reference to the widget
     layoutCheckBox->addWidget(checkBox);            // Set the checkbox in the layer
     layoutCheckBox->setAlignment(Qt::AlignCenter);  // Center the checkbox
     layoutCheckBox->setContentsMargins(0,0,0,0);    // Set the zero padding
     table_->setCellWidget(0,0, checkBoxWidget);






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

    connect(controller_, &Controller::publishEvent, this, &BreakpointsWidget::onEventFromController);
}

BreakpointsWidget::~BreakpointsWidget()
{
}

void BreakpointsWidget::onEventFromController(const Event& event) {
    qDebug() << "BreakpointsWidget::onEventFromController called";
}

}
