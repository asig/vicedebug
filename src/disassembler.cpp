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

#include "disassembler.h"

#include <string>
#include <QString>

namespace vicedebug {

std::vector<Disassembler::Line> Disassembler::disassembleForward(std::uint16_t pos, const std::vector<std::uint8_t>& memory, int lines) {
    std::vector<Line> res;

    while(lines-- > 0) {
        std::uint16_t oldpos = pos;
        res.push_back(disassembleLine(pos, memory));
        if (pos < oldpos) {
            // overflow
            break;
        }
    }
    return res;
}

}
