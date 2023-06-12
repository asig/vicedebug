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
#include <QTimer>

#include "dialogs/breakpointdialog.h"

namespace vicedebug {

BreakpointsWidget::BreakpointsWidget(Controller* controller, SymTable* symtab, QWidget* parent) :
    QGroupBox("Breakpoints", parent), controller_(controller), symtab_(symtab)
{
    QStringList l;

    tree_ = new QTreeWidget();
    tree_->setColumnCount(3);
    tree_->setHeaderLabels({ "Enabled","Address","Type"} );
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &BreakpointsWidget::onTreeItemSelectionChanged);
    connect(tree_, &QTreeWidget::itemChanged, this, &BreakpointsWidget::onTreeItemChanged);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &BreakpointsWidget::onTreeItemDoubleClicked);

    addBtn_ = new QToolButton();
    addBtn_->setIcon(QIcon(":/images/codicons/add.svg"));
    connect(addBtn_, &QToolButton::clicked, this, &BreakpointsWidget::onAddClicked);

    removeBtn_ = new QToolButton();
    removeBtn_->setIcon(QIcon(":/images/codicons/remove.svg"));
    connect(removeBtn_, &QToolButton::clicked, this, &BreakpointsWidget::onRemoveClicked);

    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addWidget(addBtn_);
    vLayout->addWidget(removeBtn_);
    vLayout->addStretch(10);

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

BreakpointsWidget::~BreakpointsWidget() {
}

void BreakpointsWidget::addItem(int bpIdx) {
    Breakpoint bp = breakpoints_[bpIdx];

    QString rangeStr = QString::asprintf("%04x", bp.addrStart);
    if (bp.addrEnd != bp.addrStart) {
        rangeStr += " - " + QString::asprintf("%04x", bp.addrEnd);
    }

    QString typeStr = "";
    if (bp.op & Breakpoint::READ) {
        typeStr += "R";
    }
    if (bp.op & Breakpoint::WRITE) {
        typeStr += "W";
    }
    if (bp.op & Breakpoint::EXEC) {
        typeStr += "X";
    }

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setCheckState(0, bp.enabled ? Qt::Checked : Qt::Unchecked);
    item->setText(1, rangeStr);
    item->setText(2, typeStr);
    item->setData(0, Qt::UserRole, QVariant(bpIdx));

    tree_->insertTopLevelItem(tree_->topLevelItemCount(), item);
}

void BreakpointsWidget::enableControls(bool enable) {
    this->setEnabled(enable);
}

void BreakpointsWidget::clearTree() {
    tree_->clear();
}

void BreakpointsWidget::fillTree(const Breakpoints& breakpoints) {
    breakpoints_ = breakpoints;
    std::sort(breakpoints_.begin(), breakpoints_.end(), [](const Breakpoint& b1, const Breakpoint& b2) {
                                                   if (b1.addrStart == b2.addrStart) {
                                                       return b1.addrEnd < b2.addrEnd;
                                                   }
                                                   return b1.addrStart < b2.addrStart;
                                               });
    for (int i = 0; i < breakpoints_.size(); i++) {
        addItem(i);
    }
}

void BreakpointsWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    qDebug() << "BreakpointsWidget::onConnected called";
    enableControls(true);    
    fillTree(breakpoints);
}

void BreakpointsWidget::onDisconnected() {
    qDebug() << "BreakpointsWidget::onDisconnected called";
    enableControls(false);
    clearTree();
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
    clearTree();
    fillTree(breakpoints);
}

void BreakpointsWidget::onSymTabChanged(std::shared_ptr<SymTable>) {
    // IMPLEMENT ME!
}

void BreakpointsWidget::onTreeItemSelectionChanged() {
    auto selectedItems = tree_->selectedItems();
    removeBtn_->setEnabled(selectedItems.size() == 1);
}

void BreakpointsWidget::onTreeItemChanged(QTreeWidgetItem* item, int column) {
    qDebug() << "Entering onTreeItemChanged()";
    if (column != 0) {
        return;
    }

    int bpIdx = item->data(0, Qt::UserRole).toInt();
    std::uint32_t bpNumber = breakpoints_[bpIdx].number;
    bool checked = item->checkState(0);
    // Ensure that the breakpoints are only updated after the tree's itemChanged signal is fully handled.
    QTimer::singleShot(0, [this, bpNumber, checked] {
        controller_->enableBreakpoint(bpNumber, checked);
    });
    qDebug() << "Leaving onTreeItemChanged()";
}

void BreakpointsWidget::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
    qDebug() << "Entering onTreeItemDoubleClicked()";
    Breakpoint bp = breakpoints_[item->data(0, Qt::UserRole).toInt()];
    BreakpointDialog dlg(bp, symtab_, this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        Breakpoint modifiedBp = dlg.breakpoint();
        bool wasEnabled = bp.enabled;
        controller_->deleteBreakpoint(bp.number);
        controller_->createBreakpoint(modifiedBp.op, modifiedBp.addrStart, modifiedBp.addrEnd, wasEnabled);
    }
    qDebug() << "Leaving onTreeItemDoubleClicked()";
}

void BreakpointsWidget::onAddClicked() {
    BreakpointDialog dlg(symtab_, this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        Breakpoint bp = dlg.breakpoint();
        controller_->createBreakpoint(bp.op, bp.addrStart,bp.addrEnd, true);
    }
}

void BreakpointsWidget::onRemoveClicked() {
    auto selected = tree_->selectedItems();
    if (selected.size() != 1) {
        qDebug() << "BreakpointsWidget: 'Remove' button clicked, but selection size is " << selected.size() << "... WTF?";
        return;
    }

    Breakpoint bp = breakpoints_[selected[0]->data(0, Qt::UserRole).toInt()];
    controller_->deleteBreakpoint(bp.number);
}

}
