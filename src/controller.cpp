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

#include "controller.h"

#include <QFuture>

namespace vicedebug {

Controller::Controller(ViceClient* viceClient)
    : viceClient_(viceClient),
      connected_(false),
      ignoreStopped_(true)
{
    connect(viceClient_, &ViceClient::stoppedResponseReceived, this, &Controller::onStoppedReceived);
}

Registers Controller::registersFromResponse(RegistersResponse response) const {
    Registers regs;
    regs.a = response.values[Registers::kRegAId];
    regs.x = response.values[Registers::kRegXId];
    regs.y = response.values[Registers::kRegYId];
    regs.flags = response.values[Registers::kRegFlagsId];
    regs.pc = response.values[Registers::kRegPCId];
    regs.sp = response.values[Registers::kRegSPId];
    return regs;
}

MachineState Controller::getMachineState() {    
    // Get memory and registers
    qDebug() << QThread::currentThreadId() << "**** Getting memory";
    auto getMemResponseFuture = viceClient_->memGet(0, 0xffff, MemSpace::MAIN_MEMORY, 0, false);
    if (!getMemResponseFuture.isFinished()) {
        qDebug() << QThread::currentThreadId() << "**** Waiting for memory";
        getMemResponseFuture.waitForFinished();
    }
    auto getMemResponse = getMemResponseFuture.result();
    qDebug() << QThread::currentThreadId() << "**** Got memory";

    qDebug() << QThread::currentThreadId() << "**** Getting registers";
    auto registersResponseFuture = viceClient_->registersGet(MemSpace::MAIN_MEMORY);
    qDebug() << QThread::currentThreadId() << "**** Waiting for registers";
    registersResponseFuture.waitForFinished();
    auto registersResponse = registersResponseFuture.result();
    qDebug() << QThread::currentThreadId() << "**** Got registers";

    MachineState machineState;
    machineState.memory = getMemResponse.memory;
    machineState.regs = registersFromResponse(registersResponse);

    return machineState;
}

void Controller::connectToVice(QString host, int port) {
    if (connected_) {
        return;
    }

    ignoreStopped_ = true;
    host_ = host;
    port_ = port;
    qDebug() <<  QThread::currentThreadId() << "**** Connecting";
    connected_ = viceClient_->connectToVice(host, port, 1000); // 1 sec timeout
    qDebug() << QThread::currentThreadId() << "**** Connected";
    if (!connected_) {
        emit connectionFailed();
        return;
    }

    auto banksAvailableResponseFuture = viceClient_->banksAvailable();
    banksAvailableResponseFuture.waitForFinished();
    auto banksAvailableResponse = banksAvailableResponseFuture.result();
    qDebug() << "Got banks:";
    for (auto p : banksAvailableResponse.banks) {
        qDebug() << "    " << p.first << ": " << p.second.c_str();
    }

    auto checkpointListResponseFuture = viceClient_->checkpointList();
    checkpointListResponseFuture.waitForFinished();
    auto checkpointListResponse = checkpointListResponseFuture.result();
    qDebug() << "Got checkpoints";

    breakpoints_.clear();
    Breakpoints breakpoints;
    for (auto cp : checkpointListResponse.checkpoints) {
        Breakpoint bp;
        bp.addrStart = cp.startAddress;
        bp.addrEnd = cp.endAddress;
        bp.enabled = cp.enabled;
        bp.number = cp.number;
        bp.op = cp.op;
        breakpoints.push_back(bp);
        breakpoints_[bp.number] = bp;
    }

    MachineState machineState = getMachineState();
    ignoreStopped_ = true;
    emit connected(machineState, breakpoints);   
}

void Controller::disconnect() {
    if (!connected_) {
        return;
    }
    viceClient_->disconnect();
    connected_ = false;
    emit disconnected();
}

void Controller::createBreakpoint(std::uint8_t op, std::uint16_t start, std::uint16_t end) {
    auto checkpointSetFuture = viceClient_->checkpointSet(start, end, true, true, op, false, MAIN_MEMORY);
    checkpointSetFuture.waitForFinished();
    auto checkpointSetResponse = checkpointSetFuture.result();

    auto cp = checkpointSetResponse.checkpoint;
    Breakpoint bp{cp.number, cp.op, cp.startAddress, cp.endAddress, cp.enabled};
    breakpoints_[bp.number] = bp;
    emitBreakpoints();
}

void Controller::enableBreakpoint(int breakpointNumber, bool enabled) {
    auto checkpointToggleFuture = viceClient_->checkpointToggle(breakpointNumber, enabled);
    // Response is empty, no need to wait for the result.
//    checkpointToggleFuture.waitForFinished();
//    auto checkpointToggleResponse = checkpointToggleFuture.result();

    breakpoints_[breakpointNumber].enabled = enabled;
    emitBreakpoints();
}

void Controller::emitBreakpoints() {
    Breakpoints bps;
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        bps.push_back(it->second);
    }
    emit breakpointsChanged(bps);
}

void Controller::updateRegisters(const Registers& registers) {
    std::map<std::uint8_t, std::uint16_t> regs = {
        {Registers::kRegAId, registers.a},
        {Registers::kRegXId, registers.x},
        {Registers::kRegYId, registers.y},
        {Registers::kRegSPId, registers.sp},
        {Registers::kRegPCId, registers.pc},
        {Registers::kRegFlagsId, registers.flags},
    };

    auto registersSetResponseFuture = viceClient_->registersSet(MAIN_MEMORY, regs);
    registersSetResponseFuture.waitForFinished();
    auto registersSetResponse = registersSetResponseFuture.result();

    emit registersChanged(registersFromResponse(registersSetResponse));
}

void Controller::stepIn() {
    ignoreStopped_ = false;
    viceClient_->advanceInstructions(false, 1);
}

void Controller::stepOut() {
    ignoreStopped_ = false;
    viceClient_->executeUntilReturn();
}

void Controller::stepOver() {
    ignoreStopped_ = false;
    viceClient_->advanceInstructions(true, 1);
}

void Controller::pauseExecution() {
    // VICE has no "pause" command, so we need to disconnect and reconnect to get the behavior
    viceClient_->disconnect();

    connected_ = viceClient_->connectToVice(host_, port_, 1000); // 1 sec timeout
    if (!connected_) {
        emit disconnected();
        return;
    }

    MachineState machineState = getMachineState();
    emit executionPaused(machineState);
}

void Controller::resumeExecution() {
    ignoreStopped_ = false;
    auto exitFuture = viceClient_->exit();
    exitFuture.waitForFinished();
    emit executionResumed();
}

void Controller::writeMemory(std::uint16_t addr, std::uint8_t data) {
    std::vector<std::uint8_t> vals { data };
    auto memSetResponseFuture = viceClient_->memSet(addr, MAIN_MEMORY, 0, false, vals);
    memSetResponseFuture.waitForFinished();

    auto memGetResponseFuture = viceClient_->memGet(addr, addr, MAIN_MEMORY, 0, false);
    memGetResponseFuture.waitForFinished();
    auto memGetResponse = memGetResponseFuture.result();

    emit memoryChanged(addr, memGetResponse.memory);
}


void Controller::onStoppedReceived(std::uint16_t pc) {
    if (ignoreStopped_) {
        qDebug() << "STOPPED received, but should ignore it. Therefore, ignoring it!";
    }
    MachineState machineState = getMachineState();
    emit executionPaused(machineState);
}

}
