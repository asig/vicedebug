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

#include <set>

#include <QFuture>

namespace vicedebug {

namespace {
  // 6502 regs
  constexpr static const std::uint8_t kRegAId = 0;
  constexpr static const std::uint8_t kRegXId = 1;
  constexpr static const std::uint8_t kRegYId = 2;
  constexpr static const std::uint8_t kRegFlagsId = 5;

  // Z80 regs
  constexpr static const std::uint8_t kRegAFId = 6;
  constexpr static const std::uint8_t kRegBCId = 7;
  constexpr static const std::uint8_t kRegDEId = 8;
  constexpr static const std::uint8_t kRegHLId = 9;
  constexpr static const std::uint8_t kRegIXId = 10;
  constexpr static const std::uint8_t kRegIYId = 11;
  constexpr static const std::uint8_t kRegIId = 12;
  constexpr static const std::uint8_t kRegRId = 13;
  constexpr static const std::uint8_t kRegAFPrimeId = 14;
  constexpr static const std::uint8_t kRegBCPrimeId = 15;
  constexpr static const std::uint8_t kRegDEPrimtId = 16;
  constexpr static const std::uint8_t kRegHLPrimeId = 17;

  // common regs
  constexpr static const std::uint8_t kRegPCId = 3;
  constexpr static const std::uint8_t kRegSPId = 4;
}

Controller::Controller(ViceClient* viceClient)
    : viceClient_(viceClient),
      connected_(false),
      ignoreStopped_(true),
      nextWatchNumber_(1)
{
    connect(viceClient_, &ViceClient::stoppedResponseReceived, this, &Controller::onStoppedReceived);
    connect(viceClient_, &ViceClient::resumedResponseReceived, this, &Controller::onResumedReceived);
}

namespace {

inline static void maybeRegisterFromRespsonse(const RegistersResponse& response, std::uint8_t viceRegId, Registers& regs, Registers::ID reg) {
    if (response.values.contains(viceRegId)) {
//        auto a = ;
        regs[reg] = response.values.at(viceRegId);
    }
}

}

Registers Controller::registersFromResponse(RegistersResponse response) const {
    Registers regs;

    // 6502 regs
    maybeRegisterFromRespsonse(response, kRegAId, regs, Registers::A);
    maybeRegisterFromRespsonse(response, kRegXId, regs, Registers::X);
    maybeRegisterFromRespsonse(response, kRegYId, regs, Registers::Y);
    maybeRegisterFromRespsonse(response, kRegFlagsId, regs, Registers::Flags);

    // Z80 regs
    maybeRegisterFromRespsonse(response, kRegAFId, regs, Registers::AF);
    maybeRegisterFromRespsonse(response, kRegBCId, regs, Registers::BC);
    maybeRegisterFromRespsonse(response, kRegDEId, regs, Registers::DE);
    maybeRegisterFromRespsonse(response, kRegHLId, regs, Registers::HL);
    maybeRegisterFromRespsonse(response, kRegIXId, regs, Registers::IX);
    maybeRegisterFromRespsonse(response, kRegIYId, regs, Registers::IY);
    maybeRegisterFromRespsonse(response, kRegIId, regs, Registers::I);
    maybeRegisterFromRespsonse(response, kRegRId, regs, Registers::R);
    maybeRegisterFromRespsonse(response, kRegAFPrimeId, regs, Registers::AFPrime);
    maybeRegisterFromRespsonse(response, kRegBCPrimeId, regs, Registers::BCPrime);
    maybeRegisterFromRespsonse(response, kRegDEPrimtId, regs, Registers::DEPrime);
    maybeRegisterFromRespsonse(response, kRegHLPrimeId, regs, Registers::HLPrime);

    // Common regs
    maybeRegisterFromRespsonse(response, kRegPCId, regs, Registers::PC);
    maybeRegisterFromRespsonse(response, kRegSPId, regs, Registers::SP);

    return regs;
}

MachineState Controller::getMachineState() {    
    MachineState machineState;

    // Get memory for all banks
    std::unordered_map<std::uint16_t, QFuture<MemGetResponse>> memGetResponseFutures;
    for (const auto& p : availableBanks_) {
        memGetResponseFutures[p.id] = viceClient_->memGet(0, 0xffff, MemSpace::MAIN_MEMORY, p.id, false);
    }
    for (auto& p : memGetResponseFutures) {
        p.second.waitForFinished();
        machineState.memory.insert({p.first, p.second.result().memory});
    }

    // Get registers
    auto getMemResponseFuture = viceClient_->memGet(0, 0xffff, MemSpace::MAIN_MEMORY, 0, false);
    getMemResponseFuture.waitForFinished();
    auto getMemResponse = getMemResponseFuture.result();

    auto registersResponseFuture = viceClient_->registersGet(MemSpace::MAIN_MEMORY);
    registersResponseFuture.waitForFinished();
    auto registersResponse = registersResponseFuture.result();

    machineState.regs = registersFromResponse(registersResponse);

    // Determine "cpu" bank
    for (const auto& p : availableBanks_) {
        if (p.name == "cpu" || p.name == "default") {
            machineState.cpuBankId = p.id;
            break;
        }
    }

    // Determine available CPUs and active CPU
    machineState.activeCpu = Cpu::MOS6502;
    if (system_ == System::C128 && (machineState.memory[0][0xd505] & 1) == 0) {
        machineState.activeCpu = Cpu::Z80;
    }
    machineState.availableCpus = availableCpus_;

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

    auto registersAvailableResponseFuture = viceClient_->registersAvailable(MemSpace::MAIN_MEMORY);
    registersAvailableResponseFuture.waitForFinished();
    auto registersAvailableResponse = registersAvailableResponseFuture.result();
    qDebug() << "Available registers:";
    for (const auto& p : registersAvailableResponse.regInfos) {
        qDebug() << "    " << p.first << ": " << p.second.name.c_str() << " (" << p.second.bits << " bits)";
    }

    auto banksAvailableResponseFuture = viceClient_->banksAvailable();
    banksAvailableResponseFuture.waitForFinished();
    auto banksAvailableResponse = banksAvailableResponseFuture.result();
    availableBanks_.clear();
    qDebug() << "Available banks:";
    for (const auto& p : banksAvailableResponse.banks) {
        availableBanks_.push_back(Bank{p.first, p.second});
        qDebug() << "    " << p.first << ": " << p.second.c_str();
    }
    std::sort(availableBanks_.begin(), availableBanks_.end(), [](const Bank& b1, const Bank& b2) {
        return b1.id < b2.id;
    });

    system_ = determineSystem();
    availableCpus_.clear();
    availableCpus_.push_back(Cpu::MOS6502); // Every system has a 6502
    if (system_ == System::C128) {
        availableCpus_.push_back(Cpu::Z80);
    }

    auto checkpointListResponseFuture = viceClient_->checkpointList();
    checkpointListResponseFuture.waitForFinished();
    auto checkpointListResponse = checkpointListResponseFuture.result();
    qDebug() << "Got checkpoints";

    breakpoints_.clear();
    // It looks like as of 2023-01-24, VICE head is returning breakpoints multiple time,
    // so we need to filter them.
    // Patch submitted to VICE: https://sourceforge.net/p/vice-emu/patches/354/
    Breakpoints breakpoints;
    for (auto cp : checkpointListResponse.checkpoints) {
        Breakpoint bp;
        bp.addrStart = cp.startAddress;
        bp.addrEnd = cp.endAddress;
        bp.enabled = cp.enabled;
        bp.number = cp.number;
        bp.op = cp.op;
        if (breakpoints_.find(bp.number) == breakpoints_.end()) {
            // Not seen yet, add it.
            breakpoints.push_back(bp);
            breakpoints_[bp.number] = bp;
        }
    }

    MachineState machineState = getMachineState();
    ignoreStopped_ = true;
    emit connected(machineState, availableBanks_, breakpoints);
}

System Controller::determineSystem() const {
    std::set<std::string> bankNames;
    for (const auto& bank : availableBanks_) {
        bankNames.emplace(bank.name);
    }

    if (bankNames.contains("vdc")) {
        return System::C128;
    }
    if (bankNames.contains("6809")) {
        return System::SuperPET;
    }
    if (bankNames.contains("extram")) {
        return System::PET;
    }
    if (bankNames.contains("cart1rom")) {
        return System::C16;
    }
    if (bankNames.contains("cart")) {
        return System::C64;
    }
    if (bankNames.contains("ram0f")) {
        return System::CBM_II;
    }
    return System::VIC20;
}

void Controller::disconnect() {
    if (!connected_) {
        return;
    }
    viceClient_->disconnect();
    connected_ = false;
    emit disconnected();
}

void Controller::createBreakpoint(std::uint8_t op, std::uint16_t start, std::uint16_t end, bool enabled) {
    auto checkpointSetFuture = viceClient_->checkpointSet(start, end, true, enabled, op, false, MemSpace::MAIN_MEMORY);
    checkpointSetFuture.waitForFinished();
    auto checkpointSetResponse = checkpointSetFuture.result();

    auto cp = checkpointSetResponse.checkpoint;
    Breakpoint bp{cp.number, cp.op, cp.startAddress, cp.endAddress, cp.enabled};
    breakpoints_[bp.number] = bp;
    emitBreakpoints();
}

void Controller::deleteBreakpoint(std::uint32_t breakpointNumber) {
    auto checkpointDeleteFuture = viceClient_->checkpointDelete(breakpointNumber);
    // Response is empty, no need to wait for the result.
    breakpoints_.erase(breakpointNumber);
    emitBreakpoints();
}

void Controller::enableBreakpoint(std::uint32_t breakpointNumber, bool enabled) {
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

void Controller::createWatch(Watch::ViewType viewType, std::uint16_t bankId, std::uint16_t addr, std::uint16_t len) {
    Watch w = Watch{nextWatchNumber_++, viewType, bankId, addr, len};
    watches_.push_back(w);
    emit watchesChanged(watches_);
}

void Controller::modifyWatch(std::uint32_t number, Watch::ViewType viewType, std::uint16_t bankId, std::uint16_t addr, std::uint16_t len) {
    for (int i = 0; i < watches_.size(); i++) {
        if (watches_[i].number == number) {
            watches_[i].viewType = viewType;
            watches_[i].bankId = bankId;
            watches_[i].addrStart = addr;
            watches_[i].len = len;
            emit watchesChanged(watches_);
            return;
        }
    }
    qWarning() << "modifyWatch: No watch found with number " << number << "!";
}

void Controller::deleteWatch(std::uint32_t number) {
    for (auto it = watches_.begin(); it != watches_.end(); ++it) {
        if (it->number == number) {
            watches_.erase(it, it);
            emit watchesChanged(watches_);
            return;
        }
    }
    qWarning() << "deleteWatch: No watch found with number " << number << "!";
}

namespace {

inline static void maybeRegisterToRequest(const std::set<std::uint8_t>& availableRegs, const Registers& regs, Registers::ID regId, std::map<std::uint8_t, std::uint16_t>& viceRegs, std::uint8_t viceRegId) {
    if (availableRegs.contains(viceRegId) && regs.contains(regId)) {
        viceRegs[viceRegId] = regs[regId];
    }
}

}

void Controller::updateRegisters(const Registers& registers) {

    // VICE really does not like it if we set registers that it doesn't currently support.
    // To ensure we don't set bad regs, we first ask for registers info and filter by it.
    auto registersAvailableResponseFuture = viceClient_->registersAvailable(MemSpace::MAIN_MEMORY);
    registersAvailableResponseFuture.waitForFinished();
    auto registersAvailableResponse = registersAvailableResponseFuture.result();
    std::set<std::uint8_t> availableRegs;
    for (const auto& p : registersAvailableResponse.regInfos) {
        availableRegs.insert(p.first);
    }

    std::map<std::uint8_t, std::uint16_t> regs;

    // 6502 regs
    maybeRegisterToRequest(availableRegs, registers, Registers::A, regs, kRegAId);
    maybeRegisterToRequest(availableRegs, registers, Registers::X, regs, kRegXId);
    maybeRegisterToRequest(availableRegs, registers, Registers::Y, regs, kRegYId);
    maybeRegisterToRequest(availableRegs, registers, Registers::Flags, regs, kRegFlagsId);

    // Z80 regs
    maybeRegisterToRequest(availableRegs, registers, Registers::AF, regs, kRegAFId);
    maybeRegisterToRequest(availableRegs, registers, Registers::BC, regs, kRegBCId);
    maybeRegisterToRequest(availableRegs, registers, Registers::DE, regs, kRegDEId);
    maybeRegisterToRequest(availableRegs, registers, Registers::HL, regs, kRegHLId);
    maybeRegisterToRequest(availableRegs, registers, Registers::IX, regs, kRegIXId);
    maybeRegisterToRequest(availableRegs, registers, Registers::IY, regs, kRegIYId);
    maybeRegisterToRequest(availableRegs, registers, Registers::I, regs, kRegIId);
    maybeRegisterToRequest(availableRegs, registers, Registers::R, regs, kRegRId);
    maybeRegisterToRequest(availableRegs, registers, Registers::AFPrime, regs, kRegAFPrimeId);
    maybeRegisterToRequest(availableRegs, registers, Registers::BCPrime, regs, kRegBCPrimeId);
    maybeRegisterToRequest(availableRegs, registers, Registers::DEPrime, regs, kRegDEPrimtId);
    maybeRegisterToRequest(availableRegs, registers, Registers::HLPrime, regs, kRegHLPrimeId);

    // Common regs
    maybeRegisterToRequest(availableRegs, registers, Registers::SP, regs, kRegSPId);
    maybeRegisterToRequest(availableRegs, registers, Registers::PC, regs, kRegPCId);

    auto registersSetResponseFuture = viceClient_->registersSet(MemSpace::MAIN_MEMORY, regs);
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

void Controller::writeMemory(std::uint16_t bankId, std::uint16_t addr, std::uint8_t data) {
    std::vector<std::uint8_t> vals { data };
    auto memSetResponseFuture = viceClient_->memSet(addr, MemSpace::MAIN_MEMORY, bankId, false, vals);
    memSetResponseFuture.waitForFinished();

    auto memGetResponseFuture = viceClient_->memGet(addr, addr, MemSpace::MAIN_MEMORY, bankId, false);
    memGetResponseFuture.waitForFinished();
    auto memGetResponse = memGetResponseFuture.result();

    emit memoryChanged(bankId, addr, memGetResponse.memory);
}


void Controller::onStoppedReceived(std::uint16_t pc) {
    if (ignoreStopped_) {
        qDebug() << "STOPPED received, but should ignore it. Therefore, ignoring it!";
    }
    MachineState machineState = getMachineState();
    emit executionPaused(machineState);
}

void Controller::onResumedReceived(std::uint16_t pc) {
//    if (ignoreStopped_) {
//        qDebug() << "STOPPED received, but should ignore it. Therefore, ignoring it!";
//    }
//    MachineState machineState = getMachineState();
    emit executionResumed();
}

}
