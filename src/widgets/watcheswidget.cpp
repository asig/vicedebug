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
#include "resources.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace vicedebug {

WatchesWidget::WatchesWidget(Controller* controller, SymTable* symtab, QWidget* parent) :
    symtab_(symtab), QGroupBox("Watches", parent), controller_(controller)
{
    QStringList l;

    tree_ = new QTreeWidget();
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({ "Bank", "Address","Type", "Value"} );
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &WatchesWidget::onTreeItemSelectionChanged);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &WatchesWidget::onTreeItemDoubleClicked);

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
    connect(controller_, &Controller::watchesChanged, this, &WatchesWidget::onWatchesChanged);

    enableControls(false);
}

WatchesWidget::~WatchesWidget()
{
}

void WatchesWidget::enableControls(bool enable) {
    this->setEnabled(enable);
}

void WatchesWidget::clearTree() {
    tree_->clear();
}

Bank WatchesWidget::bankById(std::uint32_t id) {
    for (auto b : banks_) {
        if (b.id == id) {
            return b;
        }
    }
    return Bank();
}

void WatchesWidget::updateTree() {
    // Modify existing tree items
    int i = 0;
    for (; i < tree_->topLevelItemCount() && i < watches_.size(); i++) {
        QTreeWidgetItem* item = tree_->topLevelItem(i);
        fillTreeItem(item, watches_[i], i);
    }

    // Delete unused tree items
    while (i < tree_->topLevelItemCount()) {
        tree_->takeTopLevelItem(i);
    }

    // Insert missing items
    for (; i < watches_.size(); i++) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        fillTreeItem(item, watches_[i], i);
        tree_->insertTopLevelItem(tree_->topLevelItemCount(), item);
    }
}

void WatchesWidget::fillTreeItem(QTreeWidgetItem* item, const Watch& w, int idx) {
    Bank bank = bankById(w.bankId);
    item->setText(0, bank.name.c_str());
    item->setText(1, QString::asprintf("%04x", w.addrStart));
    item->setText(2, w.viewTypeAsString());
    item->setText(3, w.asString(memory_));
    item->setFont(3, w.viewType == Watch::ViewType::CHARS ? Resources::c64Font() : Resources::robotoMonoFont());
    item->setData(0, Qt::UserRole, idx);
}

void WatchesWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    enableControls(true);
    memory_ = machineState.memory;
    banks_ = banks;
    updateTree();
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

void WatchesWidget::onWatchesChanged(const Watches& watches) {
    watches_ = watches;
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

void WatchesWidget::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
    int idx = item->data(0, Qt::UserRole).toInt();
    Watch w = watches_[idx];
    WatchDialog dlg(banks_, w, symtab_, this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        Watch modified = dlg.watch();
        controller_->modifyWatch(w.number, modified.viewType, modified.bankId, modified.addrStart, modified.len);
    }
}

void WatchesWidget::onAddClicked() {
    WatchDialog dlg(banks_, symtab_, this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        Watch w = dlg.watch();
        controller_->createWatch(w.viewType, w.bankId, w.addrStart, w.len);
    }
}

void WatchesWidget::onRemoveClicked() {
    auto selected = tree_->selectedItems();
    if (selected.size() != 1) {
        qDebug() << "WatchesWidget: 'Remove' button clicked, but selection size is " << selected.size() << "... WTF?";
        return;
    }

    int idx = tree_->indexOfTopLevelItem(selected[0]);
    controller_->deleteWatch(watches_[idx].number);
}

void WatchesWidget::onSymTabChanged() {
    // IMPLEMENT ME!
}

}
