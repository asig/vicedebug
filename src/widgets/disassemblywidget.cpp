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

#include "widgets/disassemblywidget.h"

#include "resources.h"
#include "disassembler_6502.h"
#include "disassembler_z80.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>
#include <QScrollArea>
#include <QLabel>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <iostream>

namespace vicedebug {

namespace {

const int kDecorationBorder =  8;
const char* kSeparator = "  ";

QColor kDecorationBg = QColor(Qt::lightGray);
QColor kDecorationBgDisabled = QColor(Qt::lightGray).lighter(120);

QColor kDisassemblyBg = QColor(Qt::white);
QColor kDisassemblyBgDisabled = QColor(Qt::lightGray).lighter(120);
QColor kDisassemblyBgSelected = QColor(Qt::yellow);

QColor kDisassemblyFg = QColor(Qt::black);
QColor kDisassemblyFgDisabled = QColor(Qt::lightGray).lighter(80);
QColor kDisassemblyFgSelected = QColor(Qt::black);
QColor kBreakpointFg = QColor(Qt::red);
QColor kBreakpointFgDisabled = QColor(Qt::lightGray).lighter(80);

}

DisassemblyWidget::DisassemblyWidget(Controller* controller, SymTable* symtab, QWidget* parent) :
    QWidget(parent), controller_(controller) {

    connect(symtab, &SymTable::symbolsChanged, this, &DisassemblyWidget::onSymTabChanged);

    connect(controller_, &Controller::connected, this, [this](const MachineState& machineState) {
        connected_ = true;
        bool multipleCpus = machineState.availableCpus.size() > 1;
        cpuCombo_->setVisible(multipleCpus);
        cpuLabel_->setVisible(multipleCpus);
        cpuCombo_->clear();
        int selected = -1;
        for (int i = 0; i < machineState.availableCpus.size(); i++) {
            Cpu cpu = machineState.availableCpus[i];
            if (cpu == machineState.activeCpu) {
                selected = i;
            }
            cpuCombo_->addItem(cpuName(cpu).c_str(), QVariant::fromValue((int)cpu));
        }
        cpuCombo_->setCurrentIndex(selected);
        setEnabled(true);

    } );
    connect(controller_, &Controller::disconnected, this, [this]() {
        connected_ = false;
        setEnabled(false);
    });
    connect(controller_, &Controller::executionPaused, this, [this](const MachineState& machineState) {
        for(int i = 0; i < cpuCombo_->count(); i++) {
            if (cpuCombo_->itemData(i).toInt() == (int)machineState.activeCpu) {
                cpuCombo_->setCurrentIndex(i);
                break;
            }
        }
        setEnabled(true);
    });
    connect(controller_, &Controller::executionResumed, this, [this]() { setEnabled(false); });

    // Set up disassembly content
    scrollArea_ = new QScrollArea(this);
    content_ = new DisassemblyContent(controller_, symtab, scrollArea_);
    connect(this, &DisassemblyWidget::cpuSelected, content_, &DisassemblyContent::onCpuChanged);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setWidget(content_);

    // Set up "toolbar"
    addressEdit_ = new QLineEdit();
    addressEdit_->setFont(Resources::robotoMonoFont());
    addressEdit_->setMaximumWidth(addressEdit_->fontMetrics().averageCharWidth()*8); // ~ 8 chars
    connect(addressEdit_, &QLineEdit::textEdited, [this](const QString& s) {
        std::optional<std::uint16_t> optAddr = parseAddress(s);
        goToAddressBtn_->setEnabled(optAddr.has_value());
    });
    auto goToAddrFct = [&]() {
        std::optional<std::uint16_t> optAddr = parseAddress(addressEdit_->text());
        if (optAddr.has_value()) {
           content_->goTo(optAddr.value());
           addressEdit_->clear();
           goToAddressBtn_->setEnabled(false);
        }
    };
    connect(addressEdit_, &QLineEdit::returnPressed, goToAddrFct);

    goToAddressBtn_ = new QPushButton("Go to");
    goToAddressBtn_->setEnabled(false);
    connect(goToAddressBtn_, &QPushButton::clicked, goToAddrFct);

    cpuLabel_ = new QLabel("CPU:");
    cpuCombo_ = new QComboBox();
    connect(cpuCombo_, &QComboBox::currentIndexChanged, [this](int index) {
        if (index >= 0) {
            Cpu cpu = (Cpu)cpuCombo_->itemData(index).toInt();
            emit cpuSelected(cpu);
        }
    });

    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->addWidget(addressEdit_);
    toolbar->addWidget(goToAddressBtn_);
    toolbar->addSpacing(20);
    toolbar->addWidget(cpuLabel_);
    toolbar->addWidget(cpuCombo_);
    toolbar->addStretch();

    // Set up final layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(toolbar);
    layout->addWidget(scrollArea_);

    setLayout(layout);

    setEnabled(false);
}

DisassemblyWidget::~DisassemblyWidget() {
}

void DisassemblyWidget::onSymTabChanged() {
    if (connected_) {
        content_->updateDisassembly();
        content_->update();
    }
}

// Move this into a util function
std::optional<std::uint16_t> DisassemblyWidget::parseAddress(QString s) {
    s = s.trimmed().toLower();
    if (s == "pc") {
        return content_->getPc();
    }
    int base = 16;
    if (s.startsWith("+")) {
        s = s.right(s.length()-1).trimmed();
        base = 10;
    }
    bool ok;
    int res = s.toInt(&ok, base);
    return ok ? std::optional<std::uint16_t>(res) : std::nullopt;
}

// ------------------------------------------------------------
//
// DisassemblyContent
//
// ------------------------------------------------------------

DisassemblyContent::DisassemblyContent(Controller* controller, SymTable* symtab, QScrollArea* parent) :
    QWidget(parent), symtab_(symtab), controller_(controller), mouseDown_(false), highlightedLine_(-1), scrollArea_(parent), pc_(0) {

    disassemblersPerCpu_[Cpu::MOS6502] = std::make_shared<Disassembler6502>(symtab);
    disassemblersPerCpu_[Cpu::Z80] = std::make_shared<DisassemblerZ80>(symtab);

    setFont(Resources::robotoMonoFont());

    // Compute the size of the widget:
    QFontMetrics fm(Resources::robotoMonoFont());
    lineH_ = fm.height();
    ascent_ = fm.ascent();

    decorationsW_ = fm.horizontalAdvance("    ");
    addressW_ = fm.horizontalAdvance("0000");
    separatorW_ = fm.horizontalAdvance(kSeparator);
    hexW_ = fm.horizontalAdvance("00 ");
    charW_ = fm.horizontalAdvance("0");

    connect(controller_, &Controller::connected, this, &DisassemblyContent::onConnected);
    connect(controller_, &Controller::disconnected, this, &DisassemblyContent::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &DisassemblyContent::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &DisassemblyContent::onExecutionResumed);
    connect(controller_, &Controller::breakpointsChanged, this, &DisassemblyContent::onBreakpointsChanged);
    connect(controller_, &Controller::registersChanged, this, &DisassemblyContent::onRegistersChanged);
    connect(controller_, &Controller::memoryChanged, this, &DisassemblyContent::onMemoryChanged);

    setMinimumWidth(decorationsW_ + separatorW_ + addressW_ + separatorW_ + 3 * hexW_ - charW_ + separatorW_ + 30 * charW_);

    enableControls(false);
}

QSize DisassemblyContent::sizeHint() const {
    return QSize(decorationsW_ + separatorW_ + addressW_ + separatorW_ + 3 * hexW_ - charW_ + separatorW_ + 30 * charW_, lines_.size()*lineH_);
}

void DisassemblyContent::mousePressEvent(QMouseEvent* event) {
    if (event->buttons() != Qt::LeftButton) {
        // Wrong mouse button, not interested
        event->ignore();
        return;
    }
    int x = event->position().x();
    if (x < 0 || x >= decorationsW_) {
        // Not in decorations area.
        event->ignore();
        return;
    }
    mouseDown_ = true;
    event->accept();
}

void DisassemblyContent::mouseReleaseEvent(QMouseEvent* event) {
    if (!mouseDown_) {
        // left button was never pressed, ignore.
        return;
    }

    int x = event->position().x();
    if (x < 0 || x >= decorationsW_) {
        // Not in decorations area.
        event->ignore();
        return;
    }

    int y = event->position().y();
    int lineIdx = y/lineH_;
    if (lineIdx >= lines_.size()) {
        // not a valid line
        event->ignore();
        return;
    }

    std::uint16_t addr = lines_[lineIdx].addr;
    auto it = addressToBreakpoint_.find(addr);
    if (it != addressToBreakpoint_.end()) {
        // We *do* have a breakpoint here! Remove it.
        auto bp = it->second;
        controller_->deleteBreakpoint(bp.number);
    } else {
        // No breakpoint, create one
        controller_->createBreakpoint(Breakpoint::EXEC, addr, addr, true);
    }

    mouseDown_ = false;
    event->accept();

    update();
}

void DisassemblyContent::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
//    painter.setFont(Fonts::robotoMono());
    painter.setBackgroundMode(Qt::OpaqueMode);

    int firstLine = event->rect().top()/lineH_;
    int lastLine = event->rect().bottom()/lineH_;

    for (int y = firstLine*lineH_ + ascent_, i = firstLine; i <= lastLine; ++i, y += lineH_) {
        paintLine(painter, event->rect(), i);
    }

}

void DisassemblyContent::paintLine(QPainter& painter, const QRect& updateRect, int lineIdx) {
    // Pick colors
    QColor decorationBg;
    QColor disassemblyBg;
    QColor disassemblyFg;
    QColor breakpointFg;
    if (isEnabled()) {
        bool isSelected = lineIdx == highlightedLine_;
        decorationBg = kDecorationBg;
        disassemblyBg = isSelected ? kDisassemblyBgSelected : kDisassemblyBg;
        disassemblyFg = isSelected ? kDisassemblyFgSelected : kDisassemblyFg;
        breakpointFg = kBreakpointFg;
    } else {
        decorationBg = kDecorationBgDisabled;
        disassemblyBg = kDisassemblyBgDisabled;
        disassemblyFg = kDisassemblyFgDisabled;
        breakpointFg = kBreakpointFgDisabled;
    }

    QRect decoR(0, lineIdx * lineH_, decorationsW_, lineH_);
    painter.fillRect(decoR, decorationBg);

    QRect lineR(decorationsW_, lineIdx * lineH_, updateRect.right(), lineH_);
    painter.fillRect(lineR, disassemblyBg);

    if (lineIdx >= lines_.size()) {
        return;
    }

    const Disassembler::Line& line = lines_[lineIdx];

    // Decorations
    auto it = addressToBreakpoint_.find(line.addr);
    if (it != addressToBreakpoint_.end()) {
        bool breakpointEnabled = it->second.enabled;
        int cx = (decoR.left()+decoR.right())/2;
        int cy = (decoR.top()+decoR.bottom())/2;
        int radius = lineH_/2 - 3;

        painter.setPen(breakpointFg);
        painter.setBrush(breakpointEnabled ? QBrush(breakpointFg) : Qt::NoBrush);
        painter.drawEllipse(QPoint{cx,cy},radius,radius);
    }

    // Disassembly

    // ... address
    painter.setPen(disassemblyFg);
    painter.setBackground(disassemblyBg);
    QString addr = QString::asprintf("%04X", line.addr);
    int x = lineR.left();
    int y = lineR.top() + ascent_;
    painter.drawText(x + separatorW_/2, y, addr);

    // ... bytes
    for (int i = 0; i < line.bytes.size(); i++) {
        QString text = QString::asprintf("%02X", line.bytes[i]);
        painter.drawText(x + separatorW_/2 + addressW_ + separatorW_ + i * hexW_, y, text);
    }

    // ... disassembly
    painter.drawText(x + separatorW_/2 + addressW_ + separatorW_ + 3 * hexW_ - charW_ + separatorW_ , y, line.disassembly.c_str());
}

DisassemblyContent::~DisassemblyContent() {
}

void DisassemblyContent::enableControls(bool enable) {
    setEnabled(enable);
}

void DisassemblyContent::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    memory_ = machineState.memory.at(machineState.cpuBankId);
    pc_ = machineState.regs[Registers::PC];
    disassembler_ = disassemblersPerCpu_[machineState.activeCpu];
    updateDisassembly();
    onBreakpointsChanged(breakpoints);
    enableControls(true);
}

void DisassemblyContent::onDisconnected() {
    addressToBreakpoint_.clear();
    addressToLine_.clear();
    lines_.resize(0);
    enableControls(false);
}

void DisassemblyContent::onExecutionResumed() {
    qDebug() << "DisassemblyWidget::onExecutionResumed called";
    enableControls(false);
}

void DisassemblyContent::onExecutionPaused(const MachineState& machineState) {
    qDebug() << "DisassemblyWidget::onExecutionPaused called";
    memory_ = machineState.memory.at(machineState.cpuBankId);
    pc_ = machineState.regs[Registers::PC];
    updateDisassembly();
    update();
    enableControls(true);
}

void DisassemblyContent::onBreakpointsChanged(const Breakpoints& breakpoints) {
    addressToBreakpoint_.clear();
    for (const Breakpoint& bp : breakpoints) {
        for (std::uint16_t addr = bp.addrStart; addr <= bp.addrEnd; ++addr) {
            addressToBreakpoint_[addr] = bp;
        }
    }
    update();
}

void DisassemblyContent::onRegistersChanged(const Registers& registers) {
    highlightLine(addressToLine_[registers[Registers::PC]]);
}

void DisassemblyContent::onCpuChanged(Cpu cpu) {
    qDebug() << "DisassemblyWidget::onCpuChanged called";
    disassembler_ = disassemblersPerCpu_[cpu];
    updateDisassembly();
    update();
}

void DisassemblyContent::updateDisassembly() {    
    auto before = disassembler_->disassembleBackward(pc_, memory_, 65636, {});
    auto after = disassembler_->disassembleForward(pc_, memory_, 65636);

    lines_.clear();
    addressToLine_.clear();
    lines_.reserve( before.size() + after.size() ); // preallocate memory
    lines_.insert( lines_.end(), before.begin(), before.end() );
    lines_.insert( lines_.end(), after.begin(), after.end() );
    for (int i = 0; i < lines_.size(); i++) {
        addressToLine_[lines_[i].addr] = i;
    }

    this->setMinimumHeight(lines_.size() * lineH_);
    this->setMaximumHeight(lines_.size() * lineH_);

    goTo(pc_);
}

void DisassemblyContent::highlightLine(int line) {
    highlightedLine_ = line;

    int y = highlightedLine_ * lineH_ + ascent_;
    int x = scrollArea_->horizontalScrollBar()->value();

    scrollArea_->ensureVisible(x, y, 0, 50);
}

void DisassemblyContent::goTo(std::uint16_t addr) {
    auto it = addressToLine_.find(addr);
    while (addr > 0 && it == addressToLine_.end()) {
        addr--;
        it = addressToLine_.find(addr);
    }
    auto l = it->second;
    highlightLine(l);
//    int y = l * lineH_ + ascent_;
//    int x = scrollArea_->horizontalScrollBar()->value();
//    scrollArea_->ensureVisible(x, y, 0, 50);
}

void DisassemblyContent::onMemoryChanged(std::uint16_t bankId, std::uint16_t addr, std::vector<std::uint8_t> data) {
    for (auto b : data) {
        memory_[addr++] = b;
    }
    updateDisassembly();
    update();
}

}
