#pragma once

/*
 * Copyright (c) 2023 Andreas Signer <asigner@gmail.com>
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

#include <cstdint>

namespace vicedebug {

class PETSCII {
public:
    static constexpr const std::uint32_t kUCBase = 0xee00;
    static constexpr const std::uint32_t kLCBase = 0xef00;

    static bool isPrintable(std::uint8_t c) {
        bool isCtrl = c < 32 || (128 <= c && c < 160);
        return !isCtrl;
    }

    static std::uint8_t toScreenCode(std::uint8_t c) {
        // See https://sta.c64.org/cbm64pettoscr.html
        if (c < 32) {
            return c + 128;
        }
        if (c < 64) {
            return c;
        }
        if (c < 96) {
            return c - 64;
        }
        if (c < 128) {
            return c - 32;
        }
        if (c < 160) {
            return c + 64;
        }
        if (c < 192) {
            return c - 64;
        }
        if (c < 224) {
            return c - 128;
        }
        if (c < 255) {
            return c - 128;
        }
        return 94;
    }
};

}
