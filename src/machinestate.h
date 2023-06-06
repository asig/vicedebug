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

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace vicedebug {

enum class Cpu {
    MOS6502, // Well, not only... also 6510, 7501, 8501, 8502, ... Anything 6502 compatible.
    Z80,
};

std::string cpuName(Cpu cpu);

enum class System {
    VIC20,
    C16, // actually, the 264 series
    C64,
    C128,
    PET,
    SuperPET,
};
    
struct Bank {
    std::uint16_t id;
    std::string name;       
};

typedef std::vector<Bank> Banks;
typedef std::vector<Cpu> Cpus;

struct Registers {
    constexpr static const std::uint8_t kRegAId = 0;
    constexpr static const std::uint8_t kRegXId = 1;
    constexpr static const std::uint8_t kRegYId = 2;
    constexpr static const std::uint8_t kRegPCId = 3;
    constexpr static const std::uint8_t kRegSPId = 4;
    constexpr static const std::uint8_t kRegFlagsId = 5;

    std::uint8_t a;
    std::uint8_t x;
    std::uint8_t y;
    std::uint8_t flags;
    std::uint8_t sp;
    std::uint16_t pc;
};

struct MachineState {
    System system;
    std::uint16_t cpuBankId;
    std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory;
    Registers regs;
    Cpu activeCpu;
    Cpus availableCpus;
};

}
