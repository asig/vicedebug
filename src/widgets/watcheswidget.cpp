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

#include "widgets/watcheswidget.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace vicedebug {

WatchesWidget::WatchesWidget(Controller* controller, QWidget* parent) :
    QGroupBox("Watches", parent), controller_(controller)
{
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

    connect(controller_, &Controller::connected, this, &WatchesWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &WatchesWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &WatchesWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &WatchesWidget::onExecutionResumed);
    connect(controller_, &Controller::memoryChanged, this, &WatchesWidget::onMemoryChanged);

    enableControls(false);   
}

WatchesWidget::~WatchesWidget()
{
}

void WatchesWidget::enableControls(bool enable) {
    tree_->setEnabled(enable);
    addBtn_->setEnabled(enable);
    removeBtn_->setEnabled(enable);
}

void WatchesWidget::clearControls() {
    tree_->clear();
}

void WatchesWidget::fillControls(const MachineState& machineState) {
    // FIXME fill UI
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setCheckState(0, Qt::Unchecked);
    item->setText(1, "foo");
    item->setText(2, "bar");
    tree_->insertTopLevelItem(0, item);
}

void WatchesWidget::onConnected(const MachineState& machineState, const Breakpoints& breakpoints) {
    enableControls(true);
    fillControls(machineState);
}

void WatchesWidget::onDisconnected() {
    clearControls();
    enableControls(false);
}

void WatchesWidget::onExecutionResumed() {
    enableControls(false);
}

void WatchesWidget::onExecutionPaused(const MachineState& machineState) {
    enableControls(true);
}

void WatchesWidget::onMemoryChanged(std::uint16_t addr, std::vector<std::uint8_t> data) {
}



}
