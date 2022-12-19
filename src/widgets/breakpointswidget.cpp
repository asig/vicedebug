/*
 * Copyright (c) 2022 Andreas Signer <asigner@gmail.com>
 *
 * This file is part of vicedebug.
 *
 * vicedebug is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * vicedebug is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vicedebug.  If not, see <http://www.gnu.org/licenses/>.
 */

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

    tree_ = new QTreeWidget();
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({ "Enabled","Address","Type"} );
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);

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
    hLayout->addWidget(tree_);
    hLayout->addLayout(vLayout);

    setLayout(hLayout);

    connect(controller_, &Controller::connected, this, &BreakpointsWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &BreakpointsWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &BreakpointsWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &BreakpointsWidget::onExecutionResumed);
    connect(controller_, &Controller::breakpointsChanged, this, &BreakpointsWidget::onBreakpointsChanged);

    enableControls(false);

}

BreakpointsWidget::~BreakpointsWidget()
{
}

void BreakpointsWidget::addItem(bool enabled, std::uint8_t type, std::uint16_t addrStart, std::uint16_t addrEnd) {

    QString rangeStr = QString::asprintf("%04x", addrStart);
    if (addrEnd != addrStart) {
        rangeStr += " - " + QString::asprintf("%04x", addrEnd);
    }

    QString typeStr = "";
    if (type & Breakpoint::READ) {
        typeStr += "R";
    }
    if (type & Breakpoint::WRITE) {
        typeStr += "W";
    }
    if (type & Breakpoint::EXEC) {
        typeStr += "X";
    }

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setCheckState(0, enabled ? Qt::Checked : Qt::Unchecked);
    item->setText(1, rangeStr);
    item->setText(2, typeStr);

    tree_->insertTopLevelItem(tree_->topLevelItemCount(), item);
}

void BreakpointsWidget::enableControls(bool enable) {
    tree_->setEnabled(enable);
    addBtn_->setEnabled(enable);
    removeBtn_->setEnabled(enable);
}


void BreakpointsWidget::clearControls() {
    tree_->clear();
}

void BreakpointsWidget::fillControls(const Breakpoints& breakpoints) {
    Breakpoints copy = breakpoints;
    std::sort(copy.begin(), copy.end(), [](const Breakpoint& b1, const Breakpoint& b2) {
                                                   if (b1.addrStart == b2.addrStart) {
                                                       return b1.addrEnd < b2.addrEnd;
                                                   }
                                                   return b1.addrStart < b2.addrStart;
                                               });
    for (const Breakpoint& bp : copy) {
        addItem(bp.enabled, bp.op, bp.addrStart, bp.addrEnd);
    }
}

void BreakpointsWidget::onConnected(const MachineState& machineState, const Breakpoints& breakpoints) {
    qDebug() << "BreakpointsWidget::onConnected called";
    enableControls(true);    
    fillControls(breakpoints);
}

void BreakpointsWidget::onDisconnected() {
    qDebug() << "BreakpointsWidget::onDisconnected called";
    enableControls(false);
    clearControls();
}

void BreakpointsWidget::onExecutionResumed() {
    qDebug() << "BreakpointsWidget::onExecutionResumed called";
    enableControls(false);
}

void BreakpointsWidget::onExecutionPaused(const MachineState& machineState) {
    qDebug() << "BreakpointsWidget::onExecutionPaused called";
    enableControls(true);
}

void BreakpointsWidget::onBreakpointsChanged(const Breakpoints& breakpoints) {
    clearControls();
    fillControls(breakpoints);
}

}
