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

#include "fonts.h"
#include "disassembler.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>
#include <QScrollArea>
#include <QScrollBar>

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

DisassemblyWidget::DisassemblyWidget(Controller* controller, QWidget* parent) :
    QScrollArea(parent) {

    setBackgroundRole(QPalette::Dark);
    content_ = new DisassemblyContent(controller, this);
    setWidgetResizable(true);
    setWidget(content_);
    setFont(Fonts::robotoMono());

    connect(controller, &Controller::connected, this, [this]() { this->setEnabled(true);} );
    connect(controller, &Controller::disconnected, this, [this]() { this->setEnabled(false);} );
    connect(controller, &Controller::executionPaused, this, [this]() { this->setEnabled(true);} );
    connect(controller, &Controller::executionResumed, this, [this]() { this->setEnabled(false);} );

}

DisassemblyWidget::~DisassemblyWidget() {
}

//void DisassemblyWidget::resizeEvent(QResizeEvent* event) {
//    QSize vpSize = viewport()->size();
//    QSize disassemblySize = widget()->size();
//    int newW = std::max(vpSize.width(), widget()->sizeHint().width());
//    disassemblySize.setWidth(newW);
//    widget()->resize(disassemblySize);
//}


DisassemblyContent::DisassemblyContent(Controller* controller, QScrollArea* parent) :
    QWidget(parent), controller_(controller), mouseDown_(false), highlightedLine_(-1), scrollArea_(parent)
{
    // Compute the size of the widget:
    QFontMetrics fm(Fonts::robotoMono());
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
        bp.enabled = !bp.enabled;
        controller_->enableBreakpoint(bp.number, bp.enabled);
    } else {
        // No breakpoint, create one
        controller_->createBreakpoint(Breakpoint::EXEC, addr, addr);
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

void DisassemblyContent::onConnected(const MachineState& machineState, const Breakpoints& breakpoints) {
    qDebug() << "DisassemblyWidget::onConnected called";
    memory_ = machineState.memory;
    pc_ = machineState.regs.pc;
    updateDisassembly();
    this->setMinimumHeight(lines_.size() * lineH_);
    this->setMaximumHeight(lines_.size() * lineH_);
    onBreakpointsChanged(breakpoints);
    enableControls(true);
}

void DisassemblyContent::onDisconnected() {
    qDebug() << "DisassemblyWidget::onDisconnected called";
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
    memory_ = machineState.memory;
    pc_ = machineState.regs.pc;
    updateDisassembly();

//    lines_
//    int pcBlock = 0;
//    for (auto it = lines_.begin(); it != lines_.end(); it++, pos++) {
//        text.append(QString::fromStdString(it->disassembly) + "\n");
//        if (it->addr == machineState.pc) {
//            pcBlock = pos;
//        }
//    }
//    setPlainText(text);
//    highlightLine(pcBlock);
//    QTextCursor cursor(document()->findBlockByLineNumber(pcBlock));
//    setTextCursor(cursor);
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
    highlightLine(addressToLine_[registers.pc]);
}

//void DisassemblyWidget::breakpointMousePressedEvent(QMouseEvent* event) {
//    if (event->buttons() != Qt::LeftButton) {
//        // Wrong mouse button, not interested
//        event->ignore();
//        return;
//    }
//    qDebug() << "mouse pressed: " << event->position() << " " << event->buttons();
//    mouseDown_ = true;
//    event->accept();
//}

//void DisassemblyWidget::breakpointMouseReleasedEvent(QMouseEvent* event) {
//    if (!mouseDown_) {
//        // left button was never pressed, ignore.
//        return;
//    }

//    QTextBlock block = firstVisibleBlock();
//    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
//    int lineHeight = qRound(blockBoundingRect(block).height());
//    int blockNr = block.blockNumber() + (top + event->position().y())/lineHeight;

//    qDebug() << "mouse released: " << event->position() << " --> blockNr " << blockNr ;

//    std::uint16_t addr = lines_[blockNr].addr;
//    auto it = breakpointsPerAddress_.find(addr);
//    if (it != breakpointsPerAddress_.end()) {
//        // We *do* have a breakpoint here! Update it.
//        auto bp = it->second;
//        bp.enabled = !bp.enabled;
//        controller_->enableBreakpoint(bp.number, bp.enabled);
//    } else {
//        // No breakpoint, create one
//        controller_->createBreakpoint(Breakpoint::EXEC, addr, addr);
//    }

//    mouseDown_ = false;
//    event->accept();

//    update();
//}

void DisassemblyContent::updateDisassembly() {
    Disassembler disassembler;

    auto before = disassembler.disassembleBackward(pc_, memory_, 65636);
    auto after = disassembler.disassembleForward(pc_, memory_, 65636);

    lines_.clear();
    lines_.reserve( before.size() + after.size() ); // preallocate memory
    lines_.insert( lines_.end(), before.begin(), before.end() );
    lines_.insert( lines_.end(), after.begin(), after.end() );
    for (int i = 0; i < lines_.size(); i++) {
        addressToLine_[lines_[i].addr] = i;
    }

    highlightLine(addressToLine_[pc_]);
}

void DisassemblyContent::highlightLine(int line) {
    highlightedLine_ = line;

    int y = highlightedLine_ * lineH_ + ascent_;
    int x = scrollArea_->horizontalScrollBar()->value();

    scrollArea_->ensureVisible(x, y, 0, 50);
}

void DisassemblyContent::onMemoryChanged(std::uint16_t addr, std::vector<std::uint8_t> data) {
    for (auto b : data) {
        memory_[addr++] = b;
    }
    updateDisassembly();
    update();
}






}
