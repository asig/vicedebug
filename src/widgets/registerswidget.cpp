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

// =========================================
// == RegsGroup
// =========================================

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

// =========================================
// == Regs6502Group
// =========================================

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
    sp_ = createLineEdit(4);
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

// =========================================
// == RegsZ80Group
// =========================================

RegsZ80Group::RegsZ80Group(QString title, QWidget* parent)
    : RegsGroup(title, parent)
{
    QGridLayout* layout = new QGridLayout();
    setLayout(layout);

    layout->addWidget(new QLabel("PC"), 0, 0, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("SP"), 0, 1, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("IX"), 0, 2, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("IY"), 0, 3, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("I"), 0, 4, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("R"), 0, 5, Qt::AlignBottom | Qt::AlignHCenter);

    layout->addWidget(new QLabel("AF"), 2, 0, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("BC"), 2, 1, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("DE"), 2, 2, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("HL"), 2, 3, Qt::AlignBottom | Qt::AlignHCenter);


    layout->addWidget(new QLabel("AF'"), 4, 0, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("BC'"), 4, 1, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("DE'"), 4, 2, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(new QLabel("HL'"), 4, 3, Qt::AlignBottom | Qt::AlignHCenter);

    pc_ = createLineEdit(4);
    sp_ = createLineEdit(4);
    af_ = createLineEdit(4);
    bc_ = createLineEdit(4);
    de_ = createLineEdit(4);
    hl_ = createLineEdit(4);

    ix_ = createLineEdit(4);
    iy_ = createLineEdit(4);
    i_ = createLineEdit(2);
    r_ = createLineEdit(2);

    afPrime_ = createLineEdit(4);
    bcPrime_ = createLineEdit(4);
    dePrime_ = createLineEdit(4);
    hlPrime_ = createLineEdit(4);

    layout->addWidget(pc_, 1, 0, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(sp_, 1, 1, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(ix_, 1, 2, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(iy_, 1, 3, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(i_, 1, 4, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(r_, 1, 5, Qt::AlignTop | Qt::AlignHCenter);

    layout->addWidget(af_, 3, 0, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(bc_, 3, 1, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(de_, 3, 2, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(hl_, 3, 3, Qt::AlignTop | Qt::AlignHCenter);

    layout->addWidget(afPrime_, 5, 0, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(bcPrime_, 5, 1, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(dePrime_, 5, 2, Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(hlPrime_, 5, 3, Qt::AlignTop | Qt::AlignHCenter);

    layout->setRowStretch(0,0);
    layout->setRowStretch(1,100);
}

void RegsZ80Group::clear() {
    pc_->setText("");
    sp_->setText("");
    af_->setText("");
    bc_->setText("");
    de_->setText("");
    hl_->setText("");
    ix_->setText("");
    iy_->setText("");
    i_->setText("");
    r_->setText("");
    afPrime_->setText("");
    bcPrime_->setText("");
    dePrime_->setText("");
    hlPrime_->setText("");
}

void RegsZ80Group::initFromRegs(const Registers& regs) {
    pc_->setText(QString::asprintf("%04x",regs[Registers::PC]));
    sp_->setText(QString::asprintf("%04x",regs[Registers::SP]));
    af_->setText(QString::asprintf("%04x",regs[Registers::AF]));
    bc_->setText(QString::asprintf("%04x",regs[Registers::BC]));
    de_->setText(QString::asprintf("%04x",regs[Registers::DE]));
    hl_->setText(QString::asprintf("%04x",regs[Registers::HL]));
    ix_->setText(QString::asprintf("%04x",regs[Registers::IX]));
    iy_->setText(QString::asprintf("%04x",regs[Registers::IY]));
    i_->setText(QString::asprintf("%02x",regs[Registers::I]));
    r_->setText(QString::asprintf("%02x",regs[Registers::R]));
    afPrime_->setText(QString::asprintf("%04x",regs[Registers::AFPrime]));
    bcPrime_->setText(QString::asprintf("%04x",regs[Registers::BCPrime]));
    dePrime_->setText(QString::asprintf("%04x",regs[Registers::DEPrime]));
    hlPrime_->setText(QString::asprintf("%04x",regs[Registers::HLPrime]));
}

bool RegsZ80Group::saveToRegs(Registers& regs) {
    bool ok;
    std::uint16_t newPc = (std::uint16_t)parseInt(pc_, regs[Registers::PC], 0xffff);
    std::uint16_t newSp = (std::uint16_t)parseInt(sp_, regs[Registers::SP], 0xffff);
    std::uint16_t newAf = (std::uint16_t)parseInt(af_, regs[Registers::AF], 0xffff);
    std::uint16_t newBc = (std::uint16_t)parseInt(bc_, regs[Registers::BC], 0xffff);
    std::uint16_t newDe = (std::uint16_t)parseInt(de_, regs[Registers::DE], 0xffff);
    std::uint16_t newHl = (std::uint16_t)parseInt(hl_, regs[Registers::HL], 0xffff);
    std::uint16_t newIx = (std::uint16_t)parseInt(ix_, regs[Registers::IX], 0xffff);
    std::uint16_t newIy = (std::uint16_t)parseInt(iy_, regs[Registers::IY], 0xffff);
    std::uint16_t newI =  (std::uint8_t)parseInt(i_, regs[Registers::I], 0xff);
    std::uint16_t newR =  (std::uint8_t)parseInt(r_, regs[Registers::R], 0xff);
    std::uint16_t newAfPrime = (std::uint16_t)parseInt(afPrime_, regs[Registers::AFPrime], 0xffff);
    std::uint16_t newBcPrime = (std::uint16_t)parseInt(bcPrime_, regs[Registers::BCPrime], 0xffff);
    std::uint16_t newDePrime = (std::uint16_t)parseInt(dePrime_, regs[Registers::DEPrime], 0xffff);
    std::uint16_t newHlPrime = (std::uint16_t)parseInt(hlPrime_, regs[Registers::HLPrime], 0xffff);

    bool dirty = false;
    if (newPc != regs[Registers::PC])  { regs[Registers::PC]  = newPc;  dirty = true; }
    if (newSp != regs[Registers::SP])  { regs[Registers::SP]  = newSp;  dirty = true; }
    if (newAf != regs[Registers::AF])  { regs[Registers::AF]  = newAf;  dirty = true; }
    if (newBc != regs[Registers::BC])  { regs[Registers::BC]  = newBc;  dirty = true; }
    if (newDe != regs[Registers::DE])  { regs[Registers::DE]  = newDe;  dirty = true; }
    if (newHl != regs[Registers::HL]) { regs[Registers::HL] = newHl; dirty = true; }
    if (newIx != regs[Registers::IX]) { regs[Registers::IX] = newIx; dirty = true; }
    if (newIy != regs[Registers::IY]) { regs[Registers::IY] = newIy; dirty = true; }
    if (newI != regs[Registers::I])  { regs[Registers::I]  = newI;  dirty = true; }
    if (newR != regs[Registers::R])  { regs[Registers::R]  = newR;  dirty = true; }
    if (newAfPrime != regs[Registers::AFPrime])  { regs[Registers::AFPrime] = newAfPrime;  dirty = true; }
    if (newBcPrime != regs[Registers::BCPrime]) { regs[Registers::BCPrime] = newBcPrime; dirty = true; }
    if (newDePrime != regs[Registers::DEPrime]) { regs[Registers::DEPrime] = newDePrime; dirty = true; }
    if (newHlPrime != regs[Registers::HLPrime]) { regs[Registers::HLPrime] = newHlPrime; dirty = true; }
    return dirty;
}

// =========================================
// == RegistersWidget
// =========================================

RegistersWidget::RegistersWidget(Controller* controller, QWidget* parent) :
    QWidget(parent),
    controller_(controller)
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    RegsGroup* rg = new Regs6502Group("6502 registers", this);
    regsGroupForCpu_[Cpu::MOS6502] = rg;
    activeRegsGroup_ = rg;
    layout->addWidget(rg);

    rg = new RegsZ80Group("Z80 registers", this);
    rg->setVisible(false);
    regsGroupForCpu_[Cpu::Z80] = rg;
    layout->addWidget(rg);

    connect(controller_, &Controller::connected, this, &RegistersWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &RegistersWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &RegistersWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &RegistersWidget::onExecutionResumed);
    connect(controller_, &Controller::registersChanged, this, &RegistersWidget::onRegistersChanged);

    for (const auto& [key, r] : regsGroupForCpu_) {
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
    activeRegsGroup_->setEnabled(enable);
}

void RegistersWidget::clearControls() {
    activeRegsGroup_->clear();
}

void RegistersWidget::fillControls() {
    activeRegsGroup_->initFromRegs(regs_);
}

void RegistersWidget::onFocusLost() {
    bool modified = activeRegsGroup_->saveToRegs(regs_);
    if (modified) {
        controller_->updateRegisters(regs_);
    } else {
        // Update controls anyway, just in case the user entered bad content that got reverted.
        fillControls();
    }
}

void RegistersWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    regs_ = machineState.regs;
    activeRegsGroup_ = regsGroupForCpu_[machineState.activeCpu];
    for(const auto& [key, g] : regsGroupForCpu_) {
        g->setVisible(g == activeRegsGroup_);
    }
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
    activeRegsGroup_ = regsGroupForCpu_[machineState.activeCpu];
    for(const auto& [key, g] : regsGroupForCpu_) {
        g->setVisible(g == activeRegsGroup_);
    }
    fillControls();
    enableControls(true);
}

void RegistersWidget::onRegistersChanged(const Registers& registers) {
    regs_ = registers;
    fillControls();
}

}

