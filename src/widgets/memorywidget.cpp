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

#include "fonts.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>
#include <QLineEdit>

#include <iostream>

namespace vicedebug {

namespace {

const int kBytesPerLine = 16;
const QString kSeparator("  ");

QColor kBg = QColor(Qt::white);
QColor kBgSelected = QColor(Qt::red);
QColor kBgDisabled = QColor(Qt::lightGray).lighter(120);

QColor kFg = QColor(Qt::black);
QColor kFgSelected = QColor(Qt::yellow);
QColor kFgDisabled = QColor(Qt::lightGray).lighter(80);

}

MemoryWidget::MemoryWidget(Controller* controller, QWidget* parent) :
    QWidget(parent), controller_(controller)
{
    setFont(Fonts::robotoMono());
    setFocusPolicy(Qt::StrongFocus);

    // Compute the size of the widget:
    QFontMetrics fm(Fonts::robotoMono());
    lineH_ = fm.height();
    ascent_ = fm.ascent();
    borderW_ = fm.horizontalAdvance(" ");
    addressW_ = fm.horizontalAdvance("00000");
    separatorW_ = fm.horizontalAdvance(kSeparator);
    hexSpaceW_ = fm.horizontalAdvance("00 ");
    charW_ = fm.horizontalAdvance("0");

    qDebug() << "***" << fm.horizontalAdvance(QString("00 ")) << "***";
//    qDebug() << "***" << QString("00 ").repeated(kBytesPerLine).trimmed() << "***";
//    qDebug() << "***" << hexW_ << "***";

    qDebug() << "WWWW : " << fm.horizontalAdvance("WWWW");
    qDebug() << "WWWW : " << fm.size(Qt::TextSingleLine, "WWWW").width();
    qDebug() << "0000 : " << fm.horizontalAdvance("0000");
    qDebug() << "0000 : " << fm.size(Qt::TextSingleLine, "0000").width();
    qDebug() << "     : " << fm.horizontalAdvance("    ");
    qDebug() << "     : " << fm.size(Qt::TextSingleLine, "    ").width();

    connect(controller_, &Controller::connected, this, &MemoryWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &MemoryWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &MemoryWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &MemoryWidget::onExecutionResumed);
    connect(controller_, &Controller::memoryChanged, this, &MemoryWidget::onMemoryChanged);

    updateSize(0);

    enableControls(false);
}

MemoryWidget::~MemoryWidget() {
}

void MemoryWidget::enableControls(bool enable) {
    setEnabled(enable);
}

void MemoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    int firstLine = event->rect().top()/lineH_;
    int lastLine = event->rect().bottom()/lineH_;

    auto fg = isEnabled() ? kFg : kFgDisabled;
    auto bg = isEnabled() ? kBg : kBgDisabled;
    auto fgSelected = isEnabled() ? kFgSelected : kFgDisabled;
    auto bgSelected = isEnabled() ? kBgSelected : kBgDisabled;

    painter.setPen(fg);
    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(QBrush(bg));
    painter.fillRect(event->rect(), bg);

    QString sep("  ");
    const char* addrFormatString = memory_.size() <= 0x10000 ? " %04X" : "%05X";
    for (int pos = firstLine * kBytesPerLine, y = firstLine*lineH_+ascent_; pos < (lastLine + 1) * kBytesPerLine; pos += kBytesPerLine, y += lineH_) {
        if (pos >= memory_.size()) {
            continue;
        }

        QString addr = QString::asprintf(addrFormatString, pos);
        painter.drawText(borderW_, y, addr);
        for (int i = 0; i < kBytesPerLine; i++) {
            QString hex;
            QString text;
            if (pos + i < memory_.size()) {
                std::uint8_t c = memory_[pos+i];
                hex = QString::asprintf("%02X ", c);
                text = isprint(c) ? QString((char)c) : "."; // TODO use PETSCII!
            } else {
                hex = "   ";
                text = " ";
            }
            painter.drawText(borderW_ + addressW_ + separatorW_ + i*hexSpaceW_, y, hex);
            painter.drawText(borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - charW_ + separatorW_ + i * charW_ , y, text);
        }

        if (editActive_) {
            painter.setPen(fgSelected);
            painter.setBackground(QBrush(bgSelected));
            QString text;
            if (nibbleMode_) {
                if (pos <= cursorPos_/2 && cursorPos_/2 < pos+kBytesPerLine) {
                    int x = borderW_ + addressW_ + separatorW_ + ((cursorPos_/2) % kBytesPerLine) * hexSpaceW_ + (cursorPos_ % 2) * charW_;
                    text = QString::asprintf("%02X",memory_[cursorPos_/2]).mid((cursorPos_ % 2),1);
                    painter.drawText(x, y, text);
                }
            } else {
                if (pos <= cursorPos_ && cursorPos_ < pos+kBytesPerLine) {
                    char c = (char)memory_[cursorPos_];
                    text = isprint(c) ? QString(c) : "."; // TODO use PETSCII!
                    painter.drawText(borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - charW_ + separatorW_ + (cursorPos_ % kBytesPerLine) * charW_ , y, text);
                }
            }
            painter.setPen(fg);
            painter.setBackground(QBrush(bg));
        }


//        for (int i = 0; i < kBytesPerLine; i++) {
//            QString hex;
//            QString text;
//            if (pos + i < memory_.size()) {
//                std::uint8_t c = memory_[pos+i];
//                hex = QString::asprintf("%02X ", c);
//                text = isprint(c) ? QString((char)c) : "."; // TODO use PETSCII!
//            } else {
//                hex = "   ";
//                text = " ";
//            }
//            painter.drawText(borderW_ + addressW_ +  separatorW_ + i * 29, y, hex);
//        }


//        for (int i = 0; i < kBytesPerLine; i++) {
//            if (y + i < memory_.size()) {
//                std::uint8_t c = memory_[y+i];
//                hex = QString::asprintf("%02X ", c);
//                text = isprint(c) ? QString((char)c) : "."; // TODO use PETSCII!
//            } else {
//                hex = "   ";
//                text = " ";
//            }
//            painter.drawText(borderW_ + addressW_ + separatorW_,  + i*)
//        }
//        painter.drawText(borderW_, y, addr + sep + hex + sep + text);
    }

    // Draw separators
    int h = lineH_ * ((memory_.size() + kBytesPerLine - 1)/kBytesPerLine);
    int x = borderW_ + addressW_ + separatorW_/2;
    painter.drawLine(x, 0, x, h);
    x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - charW_ + separatorW_/2;
    painter.drawLine(x, 0,x, h);
}

void MemoryWidget::mousePressEvent(QMouseEvent* event) {
    event->accept();
    QPoint pos = event->pos();
    int x = pos.x();
    // Are we in the hex part?
    int leftEdge = borderW_ + addressW_ + separatorW_;
    int rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - charW_;
    if (x >= leftEdge && x < rightEdge) {
        maybeEnterNibbleEditMode(x - leftEdge, pos.y());
    }

    // Are we in the text part?
    leftEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - charW_ + separatorW_;
    rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - charW_ + separatorW_ + kBytesPerLine * charW_;
    if (x >= leftEdge && x < rightEdge) {
        // Yes!
        maybeEnterByteEditMode(x - leftEdge, pos.y());
    }



    return;


//    // Create an inline edit
//    QLineEdit* edit = new QLineEdit(this);
//    edit->setFont(Fonts::robotoMono());
//    edit->setMaxLength(2);
//    edit->setAlignment(Qt::AlignHCenter);
//    edit->move(borderW_ + addressW_ + separatorW_ + col*hexSpaceW_,  line*lineH_);
//    edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    edit->setVisible(true);
//    qDebug() << "==================== " << bytePos;
}

void MemoryWidget::maybeEnterNibbleEditMode(int x, int y) {
    x /= charW_; // x is now the "character pos"

    // Are we on a separator space?
    if (x % 3 == 2) {
        return;
    }

    int line = y/lineH_;
    int nibbleOfs = x%3 == 0 ? 0 : 1; // Figure out what nibble we're
    int col = x/3;
    int bytePos = line * kBytesPerLine + col;
    if (bytePos >= memory_.size()) {
        // Out of bounds
        return;
    }

    editActive_ = true;
    nibbleMode_ = true;
    cursorPos_ = 2*bytePos + nibbleOfs;
    update();
    return;
}

void MemoryWidget::maybeEnterByteEditMode(int x, int y) {
    x /= charW_; // x is now the "character pos"
    int line = y/lineH_;
    int bytePos = line * kBytesPerLine + x;
    if (bytePos >= memory_.size()) {
        // Out of bounds
        return;
    }

    editActive_ = true;
    nibbleMode_ = false;
    cursorPos_ = bytePos;
    update();
    return;
}

void MemoryWidget::focusOutEvent(QFocusEvent* event) {
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

void MemoryWidget::moveCursorRight() {
    int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
    if (cursorPos_ < maxPos) {
        cursorPos_++;
    }
}

void MemoryWidget::keyPressEvent(QKeyEvent* event) {
    event->accept();
    if (!editActive_) {
        return;
    }

    // Deal with cursor movement
    switch(event->key()) {
    case Qt::Key_Left:
        if (cursorPos_ > 0) {
            cursorPos_--;
        }
        update();
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
        break;
    }
    case Qt::Key_Down: {
        int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
        int delta = kBytesPerLine * (nibbleMode_ ? 2 : 1);
        if (cursorPos_ + delta < maxPos) {
            cursorPos_ += delta;
        }
        update();
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
            controller_->writeMemory(addr, newVal);
            moveCursorRight();
        }
    } else {
        QString text = event->text();
        if (text.length() == 1) {
            std::uint8_t v = text.toStdString()[0];
            int addr = cursorPos_;
            // memory and view will be updated in onMemoryChanged();
            controller_->writeMemory(addr, v);
            moveCursorRight();
        }
    }

}

void MemoryWidget::onConnected(const MachineState& machineState, const Breakpoints& breakpoints) {
    qDebug() << "MemoryWidget::onConnected called";

    memory_ = machineState.memory;
    updateSize(memory_.size() / kBytesPerLine);

    enableControls(true);
}

void MemoryWidget::updateSize(int lines) {
    int w = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - charW_ + separatorW_ + kBytesPerLine * charW_ + borderW_;
    int h = lines * lineH_;
    setMinimumSize(w, h);
}

void MemoryWidget::onDisconnected() {
    qDebug() << "MemoryWidget::onDisconnected called";
    memory_.resize(0);
    updateSize(0);
    enableControls(false);
}

void MemoryWidget::onExecutionResumed() {
    qDebug() << "MemoryWidget::onExecutionResumed called";
    enableControls(false);
}

void MemoryWidget::onExecutionPaused(const MachineState& machineState) {
    memory_ = machineState.memory;
    enableControls(true);
    update();
}

void MemoryWidget::onMemoryChanged(std::uint16_t addr, std::vector<std::uint8_t> data) {
    for (auto b : data) {
        memory_[addr++] = b;
    }
    update();
}

}
