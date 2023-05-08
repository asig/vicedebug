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
#include <QGroupBox>
#include <QToolBar>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>

#include "controller.h"

namespace vicedebug {

class MemoryContent;

struct FindResult {
    bool found;
    int totalResults;
    int currentResult;
    int resultPos;
    int resultLen;
};

using FindFunc = std::function<FindResult(const std::vector<std::uint8_t>& data, std::uint16_t pos, std::int8_t direction)>;

class FindGroup : public QGroupBox {
    Q_OBJECT

public:
    FindGroup(QWidget* parent);
    virtual ~FindGroup();

    void start(FindFunc findFunc, bool hexMode);
    void stop();

signals:
    void findFinished();
    void markResult(const FindResult& res);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateUI(bool found);

    void find(int dir);
    bool isValidSearch(const QString& s);
    std::vector<std::uint8_t> convertToSearchData(const QString& s);

    QLabel* findLabel_;
    QLineEdit* textEdit_;
    QPushButton* findPrevBtn_;
    QPushButton* findNextBtn_;
    QToolButton* closeBtn_;

    std::uint16_t lastFindPos_;
    FindFunc findFunc_;
    bool isHexMode_;
    bool isFirstFind_;
};

class MemoryWidget : public QWidget {
    Q_OBJECT

public:
    MemoryWidget(Controller* controller, QWidget* parent);
    virtual ~MemoryWidget();

public slots:
    void onFindText();
    void onFindHex();

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
    FindGroup* findGroup_;

    Banks banks_;
    Bank selectedBank_;
    std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory_;
    Breakpoints breakpoints_;
    Watches watches_;
};

class MemoryContent : public QWidget {
    Q_OBJECT

public:
    MemoryContent(Controller* controller, QScrollArea* parent);
    virtual ~MemoryContent();

    FindResult find(const std::vector<std::uint8_t>& data, std::uint16_t pos, std::int8_t direction);
    void markSearchResult(const FindResult& res);
    void setMemory(const std::unordered_map<std::uint16_t, std::vector<std::uint8_t>>& memory, const Bank bank, const Breakpoints& breakpoints, const Watches& watches);

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
    void ensurePosVisible(std::uint16_t pos);
    void ensureCursorVisible();

    Controller* controller_;
    QScrollArea* scrollArea_;

    std::vector<std::uint8_t> memory_;
    std::vector<std::uint8_t> breakpointTypes_;
    std::vector<const Breakpoint*> breakpoint_;
    std::vector<const Watch*> watch_;

    std::uint32_t petsciiBase_; // 0xee00 for uc/graphics, and 0xef00 for lc/uc

    Breakpoints breakpoints_;
    Watches watches_;
    Bank bank_;

    // Search results highlights
    std::uint16_t resultStart_;
    std::uint16_t resultLen_;

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
