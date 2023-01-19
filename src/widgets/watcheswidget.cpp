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
#include "dialogs/watchdialog.h"
#include "fonts.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace vicedebug {

WatchesWidget::WatchesWidget(Controller* controller, QWidget* parent) :
    QGroupBox("Watches", parent), controller_(controller)
{
    QStringList l;

    tree_ = new QTreeWidget();
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({ "Address","Type", "Value"} );
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &WatchesWidget::onTreeItemSelectionChanged);
    connect(tree_, &QTreeWidget::doubleClicked, this, &WatchesWidget::onTreeItemDoubleClicked);

    addBtn_ = new QToolButton();
    addBtn_->setIcon(QIcon(":/images/codicons/add.svg"));
    connect(addBtn_, &QToolButton::clicked, this, &WatchesWidget::onAddClicked);

    removeBtn_ = new QToolButton();
    removeBtn_->setIcon(QIcon(":/images/codicons/remove.svg"));
    connect(removeBtn_, &QToolButton::clicked, this, &WatchesWidget::onRemoveClicked);

    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addWidget(addBtn_);
    vLayout->addWidget(removeBtn_);
    vLayout->addStretch(10);

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

void WatchesWidget::clearTree() {
    tree_->clear();
}

QString viewTypeAsStr(const Watch& w) {
    switch(w.viewType) {
    case Watch::ViewType::INT_HEX:
    case Watch::ViewType::INT:
        switch(w.len) {
        case 1: return "int8";
        case 2: return "int16";
        }
        return "???";
    case Watch::ViewType::UINT_HEX:
    case Watch::ViewType::UINT:
        switch(w.len) {
        case 1: return "uint8";
        case 2: return "uint16";
        }
        return "???";
    case Watch::ViewType::FLOAT:
        return "float";
    case Watch::ViewType::CHARS:
        return QString::asprintf("String(%d)", w.len);
    case Watch::ViewType::BYTES:
        return QString::asprintf("Bytes(%d)", w.len);
    }
    return "???";
}

void WatchesWidget::appendWatchToTree(const Watch& w) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, QString::asprintf("%04x", w.addrStart));
    item->setText(1, viewTypeAsStr(w));
    item->setText(2, w.asString(memory_)); // FIXME use
    item->setFont(2, Fonts::c64());
    tree_->insertTopLevelItem(tree_->topLevelItemCount(), item);
}

void WatchesWidget::fillTree() {
    for (const Watch& w : watches_) {
        appendWatchToTree(w);
    }
}

void WatchesWidget::updateTree() {
    for (int i = 0; i < watches_.size(); i++) {
        const Watch& w = watches_[i];
        QTreeWidgetItem* item = tree_->topLevelItem(i);
        item->setText(2, w.asString(memory_));
    }
}

void WatchesWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    enableControls(true);
    memory_ = machineState.memory;
    fillTree();
}

void WatchesWidget::onDisconnected() {
    clearTree();
    enableControls(false);
}

void WatchesWidget::onExecutionResumed() {
    enableControls(false);
}

void WatchesWidget::onMemoryChanged(std::uint16_t bankId, std::uint16_t address, const std::vector<std::uint8_t>& data) {
    std::vector<std::uint8_t>& mem = memory_.at(bankId);
    for (auto b : data) {
        mem[address++] = b;
    }
    updateTree();
}

void WatchesWidget::onExecutionPaused(const MachineState& machineState) {
    enableControls(true);
    memory_ = machineState.memory;
    updateTree();
}

void WatchesWidget::onTreeItemSelectionChanged() {
    auto selectedItems = tree_->selectedItems();
    removeBtn_->setEnabled(selectedItems.size() == 1);
}

void WatchesWidget::onTreeItemDoubleClicked() {
    qDebug() << "IMPLEMENT ME!!!!";
}

void WatchesWidget::onAddClicked() {
    WatchDialog dlg(this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        Watch w = dlg.watch();
        watches_.push_back(w);
        appendWatchToTree(w);
    }
}

void WatchesWidget::onRemoveClicked() {
    auto selected = tree_->selectedItems();
    if (selected.size() != 1) {
        qDebug() << "WatchesWidget: 'Remove' button clicked, but selection size is " << selected.size() << "... WTF?";
        return;
    }

    int idx = tree_->indexOfTopLevelItem(selected[0]);
    watches_.erase(watches_.begin() + idx);
    clearTree();
    fillTree();
}


}
