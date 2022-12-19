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

namespace vicedebug {

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint8_t val);
std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint16_t val);
std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint32_t val);

class vistream {
public:
    explicit vistream(std::vector<std::uint8_t>::iterator it)
        : it_(it) {}

    vistream& operator>>(std::uint8_t& val) {
        val = readU8();
        return *this;
    }

    vistream& operator>>(std::uint16_t& val) {
        val = readU16();
        return *this;
    }

    vistream& operator>>(std::uint32_t& val) {
        val = readU32();
        return *this;
    }

    std::uint8_t readU8() {
        std::uint8_t res = *(it_++);
        return res;
    }

    std::uint16_t readU16() {
        std::uint8_t b1 = *(it_++);
        std::uint8_t b2 = *(it_++);
        std::uint16_t res = b2 << 8 | b1;
        return res;
    }

    std::uint32_t readU32() {
        std::uint8_t b1 = *(it_++);
        std::uint8_t b2 = *(it_++);
        std::uint8_t b3 = *(it_++);
        std::uint8_t b4 = *(it_++);
        std::uint32_t res = b4 << 24 | b3 << 16 | b2 << 8 | b1;
        return res;
    }

private:
    std::vector<std::uint8_t>::iterator it_;
};

}

