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

#include "widgets/registerswidget.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>

#include "resources.h"

namespace vicedebug {

namespace {

uint16_t parseInt(QLineEdit* edit, int oldVal, int maxVal) {
    bool ok;
    std::uint16_t val = edit->text().toUInt(&ok, 16);
    if (!ok || val < 0 || val > maxVal) {
        val = oldVal;
    }
    return val;
}

}
RegsGroup::RegsGroup(QString title, QWidget* parent)
    : QGroupBox(title, parent)
{
}

QLineEdit* RegsGroup::createLineEdit(int len) {
    QLineEdit* e= new QLineEdit();
    e->setFont(Resources::robotoMonoFont());
    e->setMaxLength(len);
    e->setAlignment(Qt::AlignHCenter);
    e->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // No idea how much the border in a QLineEdit actually is... Let's assume
    // that an additional character is sufficient to cover that.
    int minW = e->fontMetrics().boundingRect("W").width() * (len+1);
    e->setFixedWidth(minW);

    focusWatchers_.push_back(std::make_unique<FocusWatcher>(e));

    return e;
}

Regs6502Group::Regs6502Group(QString title, QWidget* parent)
    : RegsGroup(title, parent)
{
    QGridLayout* layout = new QGridLayout();
    setLayout(layout);

    layout->addWidget(new QLabel("PC"), 0, 0, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("SP"), 0, 1, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("A"), 0, 2, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("X"), 0, 3, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("Y"), 0, 4, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("NV-BDIZC"), 0, 5, Qt::AlignBottom | Qt::AlignHCenter);

    pc_ = createLineEdit(4);
    sp_ = createLineEdit(2);
    a_ = createLineEdit(2);
    x_ = createLineEdit(2);
    y_ = createLineEdit(2);
    flags_ = createLineEdit(8);

    layout->addWidget(pc_, 1, 0, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(sp_, 1, 1, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(a_, 1, 2, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(x_, 1, 3, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(y_, 1, 4, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(flags_, 1, 5, Qt::AlignTop | Qt::AlignHCenter);

    layout->setRowStretch(0,0);
    layout->setRowStretch(1,100);
}

void Regs6502Group::clear() {
    a_->setText("");
    x_->setText("");
    y_->setText("");
    pc_->setText("");
    sp_->setText("");
}

void Regs6502Group::initFromRegs(const Registers& regs) {
    a_->setText(QString::asprintf("%02x",regs[Registers::A]));
    x_->setText(QString::asprintf("%02x",regs[Registers::X]));
    y_->setText(QString::asprintf("%02x",regs[Registers::Y]));
    sp_->setText(QString::asprintf("%02x",regs[Registers::SP]));
    pc_->setText(QString::asprintf("%04x",regs[Registers::PC]));
    auto flags = regs[Registers::Flags];
    QString flagsStr = "";
    for (int m = 128; m > 0; m >>= 1) {
        if (m == 32) {
            flagsStr += "-";
            continue;
        }
        flagsStr += (flags & m) > 0 ? "1" : "0";
    }
    flags_->setText(flagsStr);
}

bool Regs6502Group::saveToRegs(Registers& regs) {
    bool ok;
    std::uint8_t newA = (std::uint8_t)parseInt(a_, regs[Registers::A], 0xff);
    std::uint8_t newX = (std::uint8_t)parseInt(x_, regs[Registers::X], 0xff);
    std::uint8_t newY = (std::uint8_t)parseInt(y_, regs[Registers::Y], 0xff);
    std::uint8_t newSP = (std::uint8_t)parseInt(sp_, regs[Registers::SP], 0xff);
    std::uint16_t newPC = (std::uint16_t)parseInt(pc_, regs[Registers::PC], 0xffff);
    std::uint8_t newFlags = 0;
    int mask = 128;
    auto flagsStr = flags_->text().toStdString();
    if (flagsStr.length() == 8 && flagsStr[2] == '-') {
        // Potentially valid string.
        for (int i = 0; i < 8; i++) {
            if (i != 2 && flagsStr[i] == '1') {
                newFlags = newFlags | mask;
            }
            mask >>= 1;
        }
    } else {
        // Not valid string
        newFlags = regs[Registers::Flags];
    }

    bool dirty = false;
    if (newA != regs[Registers::A]) { regs[Registers::A] = newA; dirty = true; }
    if (newX != regs[Registers::X]) { regs[Registers::X] = newX; dirty = true; }
    if (newY != regs[Registers::Y]) { regs[Registers::Y] = newY; dirty = true; }
    if (newSP != regs[Registers::SP]) { regs[Registers::SP] = newSP; dirty = true; }
    if (newPC != regs[Registers::PC]) { regs[Registers::PC] = newPC; dirty = true; }
    if (newFlags != regs[Registers::Flags]) { regs[Registers::Flags] = newFlags; dirty = true; }
    return dirty;
}

RegistersWidget::RegistersWidget(Controller* controller, QWidget* parent) :
    QWidget(parent),
    controller_(controller)
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    RegsGroup* rg = new Regs6502Group("6502 registers", this);
    regsGroups_.push_back(rg);
    layout->addWidget(rg);

    connect(controller_, &Controller::connected, this, &RegistersWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &RegistersWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &RegistersWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &RegistersWidget::onExecutionResumed);
    connect(controller_, &Controller::registersChanged, this, &RegistersWidget::onRegistersChanged);

    for (auto r : regsGroups_) {
        for (const auto& f : r->getFocusWatchers()) {
            connect(f.get(), &FocusWatcher::focusLost, this, &RegistersWidget::onFocusLost);
        }
    }

    enableControls(false);
    clearControls();
}

RegistersWidget::~RegistersWidget() {
}

void RegistersWidget::enableControls(bool enable) {
    for(auto r : regsGroups_) {
        r->setEnabled(enable);
    }
}

void RegistersWidget::clearControls() {
    for(auto r : regsGroups_) {
        r->clear();
    }
}

void RegistersWidget::fillControls() {
    for(auto r : regsGroups_) {
        r->initFromRegs(regs_);
    }
}

void RegistersWidget::onFocusLost() {
    bool modified = false;
    for(auto r : regsGroups_) {
        if (r->saveToRegs(regs_)) {
            modified = true;
        }
    }

    if (modified) {
        controller_->updateRegisters(regs_);
    } else {
        // Update controls anyway, just in case the user entered bad content that got reverted.
        fillControls();
    }
}

void RegistersWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    regs_ = machineState.regs;
    fillControls();
    enableControls(true);
}

void RegistersWidget::onDisconnected() {
    clearControls();
    enableControls(false);
}

void RegistersWidget::onExecutionResumed() {
    enableControls(false);
}

void RegistersWidget::onExecutionPaused(const MachineState& machineState) {
    regs_ = machineState.regs;
    fillControls();
    enableControls(true);
}

void RegistersWidget::onRegistersChanged(const Registers& registers) {
    regs_ = registers;
    fillControls();
}

}

