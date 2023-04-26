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

#include <string>
#include <vector>
#include <cstdint>

#include "disassembler.h"

namespace vicedebug {

class DisassemblerZ80 : public Disassembler {
public:
    std::vector<Line> disassembleBackward(std::uint16_t pos, const std::vector<std::uint8_t>& memory, int lines, const std::vector<Line>& disassemblyHint) override;

protected:
    Line disassembleLine(std::uint16_t& pos, const std::vector<std::uint8_t>& memory) override;
};

}
