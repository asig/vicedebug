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

#include "widgets/memorywidget.h"

#include "resources.h"
#include "petscii.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>
#include <QLineEdit>
#include <QScrollBar>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolTip>
#include <QMenu>

#include <iostream>

namespace vicedebug {

namespace {

const int kBytesPerLine = 16;
const QString kSeparator("  ");

const QColor kBg = QColor(Qt::white);
const QColor kBgSelected = QColor(Qt::red);
const QColor kBgDisabled = QColor(Qt::lightGray).lighter(120);

const QColor kFg = QColor(Qt::black);
const QColor kFgSelected = QColor(Qt::yellow);
const QColor kFgDisabled = QColor(Qt::lightGray).lighter(80);

constexpr const std::uint32_t kPetsciiUCBase = 0xee00;
constexpr const std::uint32_t kPetsciiLCBase = 0xef00;

}

MemoryWidget::MemoryWidget(Controller* controller, QWidget* parent) :
    controller_(controller), QWidget(parent)
{
    connect(controller_, &Controller::connected, this, [this]() { setEnabled(true);} );
    connect(controller_, &Controller::disconnected, this, [this]() { setEnabled(false);} );
    connect(controller_, &Controller::executionPaused, this, [this]() { setEnabled(true);} );
    connect(controller_, &Controller::executionResumed, this, [this]() { setEnabled(false);} );

    // Set up memory content
    scrollArea_ = new QScrollArea(this);
    content_ = new MemoryContent(controller_, scrollArea_);
    connect(content_, &MemoryContent::memoryChanged, this, [this](std::uint16_t addr, std::uint8_t val) {
        controller_->writeMemory(selectedBank_, addr, val);
    });
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setWidget(content_);

    // Set up "toolbar"
    bankCombo_ = new QComboBox();
    connect(bankCombo_, &QComboBox::currentIndexChanged, [this](int index) {
        selectedBank_ = index;
        if (selectedBank_ < memory_.size()) {
            content_->setMemory(memory_.at(selectedBank_), selectedBank_ == 0 ? breakpoints_ : Breakpoints()); // So far, breakpoints are only supported for default bank...
        }
    });
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Bank:"));
    toolbar->addWidget(bankCombo_);
    toolbar->addStretch();

    // Set up final layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(toolbar);
    layout->addWidget(scrollArea_);

    setLayout(layout);

    connect(controller, &Controller::connected, this, &MemoryWidget::onConnected);
    connect(controller, &Controller::disconnected, this, &MemoryWidget::onDisconnected);
    connect(controller, &Controller::executionPaused, this, &MemoryWidget::onExecutionPaused);
    connect(controller, &Controller::executionResumed, this, &MemoryWidget::onExecutionResumed);
    connect(controller, &Controller::memoryChanged, this, &MemoryWidget::onMemoryChanged);
    connect(controller, &Controller::breakpointsChanged, this, &MemoryWidget::onBreakpointsChanged);

    setEnabled(false);
}

MemoryWidget::~MemoryWidget() {
}

void MemoryWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    bankCombo_->clear();
    for (Bank b : banks) {
        bankCombo_->addItem(b.name.c_str(), QVariant(b.id));
    }
    selectedBank_ = 0;
    bankCombo_->setCurrentIndex(0);
    memory_ = machineState.memory;
    breakpoints_ = breakpoints;
    content_->setMemory(memory_.at(selectedBank_), selectedBank_ == 0 ? breakpoints_ : Breakpoints()); // So far, breakpoints are only supported for default bank...
    setEnabled(true);
}

void MemoryWidget::onDisconnected() {
    memory_.clear();
    content_->setMemory({}, {});
    setEnabled(false);
}

void MemoryWidget::onExecutionResumed() {
    // enableControls() seems to change the scroll position when the user was editing contents.
    // As a workaround, remember and restore scroll positions.
    int vert = scrollArea_->verticalScrollBar()->value();
    int hor = scrollArea_->horizontalScrollBar()->value();

    setEnabled(false);

    scrollArea_->verticalScrollBar()->setValue(vert);
    scrollArea_->horizontalScrollBar()->setValue(hor);
}

void MemoryWidget::onExecutionPaused(const MachineState& machineState) {
    memory_ = machineState.memory;
    content_->setMemory(memory_.at(selectedBank_), selectedBank_ == 0 ? breakpoints_ : Breakpoints()); // So far, breakpoints are only supported for default bank...
    setEnabled(true);
    update();
}

void MemoryWidget::onMemoryChanged(std::uint16_t bankId, std::uint16_t addr, std::vector<std::uint8_t> data) {
    std::vector<std::uint8_t>& mem = memory_.at(bankId);
    for (auto b : data) {
        mem[addr++] = b;
    }
    content_->setMemory(memory_.at(selectedBank_), selectedBank_ == 0 ? breakpoints_ : Breakpoints()); // So far, breakpoints are only supported for default bank...
}


void MemoryWidget::onBreakpointsChanged(const Breakpoints& breakpoints) {
    breakpoints_ = breakpoints;
    content_->setMemory(memory_.at(selectedBank_), selectedBank_ == 0 ? breakpoints_ : Breakpoints()); // So far, breakpoints are only supported for default bank...
}

MemoryContent::MemoryContent(Controller* controller, QScrollArea* parent) :
    QWidget(parent), controller_(controller), scrollArea_(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    // Compute the size of the widget:
    QFontMetrics robotofm(Resources::robotoMonoFont());
    ascent_ = robotofm.ascent();
    borderW_ = robotofm.horizontalAdvance(" ");
    addressW_ = robotofm.horizontalAdvance("00000");
    separatorW_ = robotofm.horizontalAdvance(kSeparator);
    hexSpaceW_ = robotofm.horizontalAdvance("00 ");
    hexCharW_ = robotofm.horizontalAdvance("0");

    QFontMetrics c64fm(Resources::c64Font());
    charW_ = c64fm.horizontalAdvance("0");

    lineH_ = c64fm.height() > robotofm.height() ? c64fm.height() : robotofm.height();

    editActive_ = false;
    petsciiBase_ = kPetsciiUCBase;

    updateSize(0);
}

MemoryContent::~MemoryContent() {
}

void MemoryContent::setMemory(const std::vector<std::uint8_t>& memory, const Breakpoints& breakpoints) {
    memory_ = memory;
    breakpoints_ = breakpoints;
    breakpointTypes_.resize(memory.size());
    breakpoint_.resize(memory.size());
    for (int i = 0; i < memory.size(); i++) {
        breakpointTypes_[i] = 0;
        breakpoint_[i] = nullptr;
    }
    for (const auto& bp : breakpoints_) {
        std::uint8_t op = bp.op & (Breakpoint::Type::READ | Breakpoint::Type::WRITE);
        if (op == 0) {
            continue;
        }
        for (int addr = bp.addrStart; addr <= bp.addrEnd; addr++) {
            breakpointTypes_[addr] = op;
            breakpoint_[addr] = &bp;
        }
    }
    updateSize(memory_.size() / kBytesPerLine);
    update();
}

QString formatBpAddr(const Breakpoint* bp) {
    if (bp->addrStart == bp->addrEnd) {
        return QString::asprintf("%04x", bp->addrStart);
    }
    return QString::asprintf("%04x - %04x", bp->addrStart, bp->addrEnd);
}

QString formatBpOp(const Breakpoint* bp) {
    QString s;
    std::vector<QString> ops;
    if (bp->op & Breakpoint::Type::READ) {
        ops.push_back("<b>Load</b>");
    }
    if (bp->op & Breakpoint::Type::WRITE) {
        ops.push_back("<b>Store</b>");
    }
    if (bp->op & Breakpoint::Type::EXEC) {
        ops.push_back("<b>Execute</b>");
    }
    switch(ops.size()) {
    case 3:
        return ops[0] + ", " + ops[1] + ", and " + ops[2];
    case 2:
        return ops[0] + " and " + ops[1];
    case 1:
        return ops[0];
    default:
        return "WTF????";
    }
}

bool MemoryContent::event(QEvent* event) {
    if (event->type() != QEvent::ToolTip) {
        return QWidget::event(event);
    }
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    QPoint p = helpEvent->pos();
    std::uint16_t addr;
    bool dontCareBool;
    int dontCareInt;
    bool hideToolTip = true;
    if (addrAtPos(p, addr, dontCareBool, dontCareInt)) {
        const Breakpoint* bp = breakpoint_[addr];
        if (bp != nullptr) {
            hideToolTip = false;
            QString tooltip = QString::asprintf("<b>Breakpoint %d</b><br>", bp->number);
            if (!bp->enabled) {
                tooltip += "Disabled<br>";
            }
            tooltip += "Address: " + formatBpAddr(bp) + "<br>";
            tooltip += "Break on " + formatBpOp(bp);
            QToolTip::showText(helpEvent->globalPos(), tooltip);
        }
    }
    if (hideToolTip) {
        QToolTip::hideText();
        event->ignore();
    }
    return true;
}

void MemoryContent::contextMenuEvent(QContextMenuEvent* event) {
    std::uint16_t addr;
    bool dontCareBool;
    int dontCareInt;
    if (!addrAtPos(event->pos(), addr, dontCareBool, dontCareInt)) {
        return;
    }

    QMenu menu(this);
    const Breakpoint* bp = breakpoint_[addr];
    if (bp) {
        connect(menu.addAction("Delete breakpoint"), &QAction::triggered, [bp, this] {
            controller_->deleteBreakpoint(bp->number);
        });
        QString label = QString(bp->enabled ? "Disable" : "Enable") + " breakpoint";
        connect(menu.addAction(label), &QAction::triggered, [bp, this] {
            controller_->enableBreakpoint(bp->number, !bp->enabled);
        });
    } else {
        if (selectedBankId_ == 0) {
            // So far, breakpoints are only available in "main" bank...
            QMenu* submenu = menu.addMenu("Add breakpoint...");
            connect(submenu->addAction("Break on load"), &QAction::triggered, [addr, this] {
                controller_->createBreakpoint(Breakpoint::Type::READ, addr, addr, true);
            });
            connect(submenu->addAction("Break on store"), &QAction::triggered, [addr, this] {
                controller_->createBreakpoint(Breakpoint::Type::WRITE, addr, addr, true);
            });
            connect(submenu->addAction("Break on both"), &QAction::triggered, [addr, this] {
                controller_->createBreakpoint(Breakpoint::Type::READ | Breakpoint::Type::WRITE, addr, addr, true);
            });
        }
    }
    QMenu* submenu = menu.addMenu("Add watch...");
    connect(submenu->addAction("int8"), &QAction::triggered, [addr, this] {
        qDebug() << "IMPLEMENT ME!";
    });
    menu.exec(event->globalPos());
    event->ignore();
}

void MemoryContent::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    int firstLine = event->rect().top()/lineH_;
    int lastLine = event->rect().bottom()/lineH_;

    auto fg = isEnabled() ? kFg : kFgDisabled;
    auto bg = isEnabled() ? kBg : kBgDisabled;
    auto fgSelected = isEnabled() ? kFgSelected : kFgDisabled;
    auto bgSelected = isEnabled() ? kBgSelected : kBgDisabled;

    QBrush bpEnableddBg = QBrush(QColor(255,0,0,100));
    QBrush bpDisabledBg = QBrush(QColor(255,0,0,50));

    painter.setPen(fg);
    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(QBrush(bg));
    painter.fillRect(event->rect(), bg);

    int memSize = memory_.size() > 0 ? memory_.size() : 0;
    QString sep("  ");
    const char* addrFormatString = memSize <= 0x10000 ? " %04X" : "%05X";
    for (int pos = firstLine * kBytesPerLine, y = firstLine*lineH_+ascent_; pos < (lastLine + 1) * kBytesPerLine; pos += kBytesPerLine, y += lineH_) {
        if (pos >= memSize) {
            continue;
        }

        QString addr = QString::asprintf(addrFormatString, pos);
        painter.setFont(Resources::robotoMonoFont());
        painter.drawText(borderW_, y, addr);
        for (int i = 0; i < kBytesPerLine; i++) {
            QString hex;
            QString text;
            const Breakpoint* bp = breakpoint_[pos + i];
            if (pos + i < memSize) {
                std::uint8_t c = memory_[pos+i];
                hex = QString::asprintf("%02X ", c);
                text = PETSCII::isPrintable(c) ? QString(QChar(petsciiBase_ + PETSCII::toScreenCode(c))) : ".";
            } else {
                hex = "   ";
                text = " ";
            }
            if (bp) {
                painter.setBackground(QBrush(bp->enabled ? bpEnableddBg : bpDisabledBg));
            }
            painter.setFont(Resources::robotoMonoFont());
            painter.drawText(borderW_ + addressW_ + separatorW_ + i*hexSpaceW_, y, hex);
            painter.setFont(Resources::c64Font());
            painter.drawText(borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + i * charW_ , y, text);
            if (bp) {
                painter.setBackground(QBrush(bg));
            }
        }

        if (editActive_) {
            painter.setPen(fgSelected);
            painter.setBackground(QBrush(bgSelected));
            QString text;
            if (nibbleMode_) {
                if (pos <= cursorPos_/2 && cursorPos_/2 < pos+kBytesPerLine) {
                    int x = borderW_ + addressW_ + separatorW_ + ((cursorPos_/2) % kBytesPerLine) * hexSpaceW_ + (cursorPos_ % 2) * hexCharW_;
                    text = QString::asprintf("%02X",memory_[cursorPos_/2]).mid((cursorPos_ % 2),1);
                    painter.setFont(Resources::robotoMonoFont());
                    painter.drawText(x, y, text);
                }
            } else {
                if (pos <= cursorPos_ && cursorPos_ < pos+kBytesPerLine) {
                    char c = (char)memory_[cursorPos_];
                    text = PETSCII::isPrintable(c) ? QString(QChar(petsciiBase_ + PETSCII::toScreenCode(c))) : ".";
                    painter.setFont(Resources::c64Font());
                    painter.drawText(borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (cursorPos_ % kBytesPerLine) * charW_ , y, text);
                }
            }
            painter.setPen(fg);
            painter.setBackground(QBrush(bg));
        }
    }

    // Draw separators
    int h = lineH_ * ((memSize + kBytesPerLine - 1)/kBytesPerLine);
    int x = borderW_ + addressW_ + separatorW_/2;
    painter.drawLine(x, 0, x, h);
    x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_/2;
    painter.drawLine(x, 0,x, h);
}

void MemoryContent::mousePressEvent(QMouseEvent* event) {
    event->accept();
    QPoint pos = event->pos();

    std::uint16_t addr;
    int nibbleOfs;
    if (addrAtPos(pos, addr, nibbleMode_, nibbleOfs)) {
        editActive_ = true;
        if (nibbleMode_) {
            cursorPos_ = 2*addr + nibbleOfs;
        } else {
            cursorPos_ = addr;
        }
        update();
        return;
    }
}

bool MemoryContent::addrAtPos(QPoint pos, std::uint16_t& addr, bool& nibbleMode, int& nibbleOfs) {
    int x = pos.x();
    int y = pos.y();
    int line = y/lineH_;

    // Are we in the hex part?
    int leftEdge = borderW_ + addressW_ + separatorW_;
    int rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - hexCharW_;
    if (x >= leftEdge && x < rightEdge) {
        // Maybe we're in nibble mode!
        x -= leftEdge;
        int byteOfs = x / hexSpaceW_;
        x = x % hexSpaceW_;
        nibbleOfs = x / hexCharW_;
        // Are we on a separator space?
        if (nibbleOfs == 2) {
            // Yes
            return false;
        }

        addr = line * kBytesPerLine + byteOfs;
        if (addr >= memory_.size()) {
            // Out of bounds
            return false;
        }
        nibbleMode = true;
        return true;
    }

    // Are we in the text part?
    leftEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_;
    rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + kBytesPerLine * charW_;
    if (x >= leftEdge && x < rightEdge) {
        x -= leftEdge;
        x /= charW_; // x is now the "character pos"
        addr = line * kBytesPerLine + x;
        if (addr >= memory_.size()) {
            // Out of bounds
            return false;
        }
        return true;
    }
    return false;
}

void MemoryContent::focusOutEvent(QFocusEvent* event) {
    editActive_ = false;
    update();
}

int charToHex(char c) {
    if (isdigit(c)) {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return 10+ c-'a';
    } else if ('A' <= c && c <= 'F') {
        return 10+ c-'A';
    }
    return -1;
}

void MemoryContent::moveCursorRight() {
    int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
    if (cursorPos_ < maxPos) {
        cursorPos_++;
    }
    update();
    ensureCursorVisible();
}

void MemoryContent::ensureCursorVisible() {
    int x, y;
    if (nibbleMode_) {
        y = (cursorPos_/2) / kBytesPerLine * lineH_;
        // TODO(asigner): Extract those calculations in functions!
        x = borderW_ + addressW_ + separatorW_ + ((cursorPos_/2) % kBytesPerLine) * hexSpaceW_ + (cursorPos_ % 2) * hexCharW_; // T
    } else {
        y = cursorPos_ / kBytesPerLine * lineH_;
        x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (cursorPos_ % kBytesPerLine) * charW_;
    }
    scrollArea_->ensureVisible(x, y, 0, 50);
}

void MemoryContent::keyPressEvent(QKeyEvent* event) {
    if (!editActive_) {
        event->ignore();
        return;
    }
    event->accept();

    // Deal with cursor movement
    switch(event->key()) {
    case Qt::Key_Left:
        if (cursorPos_ > 0) {
            cursorPos_--;
        }
        update();
        ensureCursorVisible();
        break;
    case Qt::Key_Right: {
        moveCursorRight();
        update();
        break;
    }
    case Qt::Key_Up: {
        int delta = kBytesPerLine * (nibbleMode_ ? 2 : 1);
        if (cursorPos_ - delta >= 0) {
            cursorPos_ -= delta;
        }
        update();
        ensureCursorVisible();
        break;
    }
    case Qt::Key_Down: {
        int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
        int delta = kBytesPerLine * (nibbleMode_ ? 2 : 1);
        if (cursorPos_ + delta < maxPos) {
            cursorPos_ += delta;
        }
        update();
        ensureCursorVisible();
        break;
    }
    }

    if (nibbleMode_) {
        QString text = event->text().toLower();
        if (text.length() == 1) {
            int v = charToHex(text.toStdString()[0]);
            if (v == -1) { return; }

            std::uint8_t mask = 0xf;
            std::uint8_t shift = cursorPos_%2 == 0 ? 4 : 0;

            std::uint8_t oldVal = memory_[cursorPos_/2];
            std::uint8_t newVal = oldVal & ~(mask<<shift) | (v<<shift);
            int addr = cursorPos_/2;
            // memory and view will be updated in onMemoryChanged();
            emit memoryChanged(addr, newVal);
            moveCursorRight();
        }
    } else {
        QString text = event->text();
        if (text.length() == 1) {
            std::uint8_t v = text.toStdString()[0];
            int addr = cursorPos_;
            // memory and view will be updated in onMemoryChanged();
            emit memoryChanged(addr, v);
            moveCursorRight();
        }
    }

}

void MemoryContent::updateSize(int lines) {
    int w = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - hexCharW_ + separatorW_ + kBytesPerLine * charW_ + borderW_;
    int h = lines * lineH_;
    setMinimumSize(w, h);
}

}
