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

#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>

#include "controller.h"
#include "disassembler.h"

namespace vicedebug {

class DisassemblyContent;

class DisassemblyWidget : public QWidget {
    Q_OBJECT

public:
    DisassemblyWidget(Controller* controller, QWidget* parent);
    virtual ~DisassemblyWidget();

//public slots:
//    void onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints);
//    void onDisconnected();

protected:
//    void resizeEvent(QResizeEvent* event) override;

private:
    std::optional<std::uint16_t> parseAddress(QString s);

    Controller* controller_;

    QScrollArea* scrollArea_;
    DisassemblyContent* content_;

    QLineEdit* addressEdit_;
    QPushButton* goToAddressBtn_;
};

class DisassemblyContent : public QWidget {
    Q_OBJECT

public:
    DisassemblyContent(Controller* controller, QScrollArea* parent);
    virtual ~DisassemblyContent();

    void highlightLine(int line);
    void goTo(std::uint16_t address);

    std::uint16_t getPc() const {
        return pc_;
    }

protected:
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
//    void resizeEvent(QResizeEvent* event) override;

public slots:
    void onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints);
    void onDisconnected();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& machineState);
    void onBreakpointsChanged(const Breakpoints& breakpoints);
    void onRegistersChanged(const Registers& registers);
    void onMemoryChanged(std::uint16_t bankId, std::uint16_t addr, std::vector<std::uint8_t> data);

private:
    void paintLine(QPainter& painter, const QRect& updateRect, int line);
    void updateDisassembly();

    void enableControls(bool enable);

    Controller* controller_;
    std::vector<Disassembler::Line> lines_;
    std::map<std::uint16_t, Breakpoint> addressToBreakpoint_;
    std::map<std::uint16_t, int> addressToLine_;
    std::vector<std::uint8_t> memory_;
    std::uint16_t pc_;

    QScrollArea* scrollArea_;

    int highlightedLine_;

    bool mouseDown_;


    // Widths of parts of the widget
    int lineH_; // Height of a line
    int ascent_;
    int charW_; // single character
    int decorationsW_; // Width of decoration part
    int separatorW_; // Separator width between address and bytes, as well as bytes and text
    int addressW_; // Address part
    int hexW_;

};

}
