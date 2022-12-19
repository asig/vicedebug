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

#include "vectorutils.h"

namespace vicedebug {

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint8_t val) {
    vec.push_back(val);
    return vec;
}

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint16_t val) {
    vec.push_back(val & 0xff);
    vec.push_back((val >> 8) & 0xff);
    return vec;
}

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint32_t val) {
    vec.push_back(val & 0xff);
    vec.push_back((val >> 8) & 0xff);
    vec.push_back((val >> 16) & 0xff);
    vec.push_back((val >> 24) & 0xff);
    return vec;
}

}

