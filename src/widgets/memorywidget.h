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

#include "controller.h"

namespace vicedebug {

class MemoryContent;


class MemoryWidget : public QScrollArea {
    Q_OBJECT

public:
    MemoryWidget(Controller* controller, QWidget* parent);
    virtual ~MemoryWidget();

protected:
//    void resizeEvent(QResizeEvent* event) override;

private:
    MemoryContent* content_;
};

class MemoryContent : public QWidget {
    Q_OBJECT

public:
    MemoryContent(Controller* controller, QScrollArea* parent);
    virtual ~MemoryContent();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

public slots:
    void onConnected(const MachineState& machineState, const Breakpoints& breakpoints);
    void onDisconnected();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& machineState);
    void onMemoryChanged(std::uint16_t addr, std::vector<std::uint8_t> data);

private:
    void enableControls(bool enable);
    void updateSize(int lines);

    void maybeEnterNibbleEditMode(int x, int y);
    void maybeEnterByteEditMode(int x, int y);

    void moveCursorRight();

    void ensureCursorVisible();

    Controller* controller_;
    QScrollArea* scrollArea_;

    std::vector<std::uint8_t> memory_;

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
