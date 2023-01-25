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

#include <QString>

#include "viceclient.h"
#include "machinestate.h"
#include "breakpoints.h"

namespace vicedebug {

class Controller : public QObject {
    Q_OBJECT

public:
    Controller(ViceClient* viceClient);

    void connectToVice(QString host, int port);
    void disconnect();

    void createBreakpoint(std::uint8_t op, std::uint16_t start, std::uint16_t end, bool enabled);
    void deleteBreakpoint(std::uint32_t breakpointNumber);
    void enableBreakpoint(std::uint32_t breakpointNumber, bool enabled);

    bool isConnected() {
        return connected_;
    }

    void updateRegisters(const Registers& newValues);

    void stepIn();
    void stepOut();
    void stepOver();
    void pauseExecution();
    void resumeExecution();

    void writeMemory(std::uint16_t bankId, std::uint16_t addr, std::uint8_t data);

signals:
    void connected(const MachineState& machineState, const Banks& availableBanks, const Breakpoints& breakpoints);
    void connectionFailed();
    void disconnected();
    void executionResumed();
    void executionPaused(const MachineState& machineState);
    void breakpointsChanged(const Breakpoints& breakpoints);
    void registersChanged(const Registers& registers);
    void memoryChanged(std::uint16_t bankId, std::uint16_t address, const std::vector<std::uint8_t>& data);

private slots:
    void onStoppedReceived(std::uint16_t pc);
    void onResumedReceived(std::uint16_t pc);

private:
    Registers registersFromResponse(RegistersResponse response) const;
    MachineState getMachineState();

    void emitBreakpoints();

    bool ignoreStopped_;

    QString host_;
    int port_;
    bool connected_;
    ViceClient* viceClient_;
    std::map<std::uint32_t, Breakpoint> breakpoints_;
    Banks availableBanks_;
};

}
