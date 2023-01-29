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

#pragma once

#include <map>

#include <QPlainTextEdit>
#include <QScrollArea>
#include <QComboBox>

#include "controller.h"

namespace vicedebug {

class MemoryContent;


class MemoryWidget : public QWidget {
    Q_OBJECT

public:
    MemoryWidget(Controller* controller, QWidget* parent);
    virtual ~MemoryWidget();

private slots:
    void onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints);
    void onDisconnected();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& machineState);
    void onMemoryChanged(std::uint16_t bankId, std::uint16_t addr, std::vector<std::uint8_t> data);
    void onBreakpointsChanged(const Breakpoints& breakpoints);
    void onWatchesChanged(const Watches& watches);

private:
    Controller* controller_;

    QComboBox* bankCombo_;
    QScrollArea* scrollArea_;
    MemoryContent* content_;

    int selectedBank_;
    std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory_;
    Breakpoints breakpoints_;
    Watches watches_;
};

class MemoryContent : public QWidget {
    Q_OBJECT

public:
    MemoryContent(Controller* controller, QScrollArea* parent);
    virtual ~MemoryContent();

public:
    void setMemory(const std::vector<std::uint8_t>& memory, const Breakpoints& breakpoints, const Watches& watches, bool canHaveBreakpoints);

signals:
    void memoryChanged(std::uint16_t addr, std::uint8_t newVal);

protected:
    bool event(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateSize(int lines);
    bool addrAtPos(QPoint pos, std::uint16_t& addr, bool& nibbleMode, int& nibbleOfs);
    void moveCursorRight();
    void ensureCursorVisible();

    Controller* controller_;
    QScrollArea* scrollArea_;

    bool canHaveBreakpoints_;
    std::vector<std::uint8_t> memory_;
    std::vector<std::uint8_t> breakpointTypes_;
    std::vector<const Breakpoint*> breakpoint_;
    std::vector<const Watch*> watch_;

    std::uint32_t petsciiBase_; // 0xee00 for uc/graphics, and 0xef00 for lc/uc

    Breakpoints breakpoints_;
    Watches watches_;

    // Edit mode variables
    bool editActive_;
    bool nibbleMode_;
    int cursorPos_; // nibble-offset into memory

    // Widths of parts of the widget
    int lineH_; // Height of a line
    int ascent_;
    int borderW_; // Left and right border
    int addressW_; // Address part
    int separatorW_; // Separator width between address and hex, as well as hex and text
    int hexSpaceW_; // hex part
    int hexCharW_; // single character in the hex part
    int textW_;

    int charW_; // text part
};

}
