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

#include "watches.h"

namespace vicedebug {

namespace {

std::int16_t readInt(const std::vector<std::uint8_t>& memory, std::uint16_t addr, int len) {
    int shift = 0;
    std::int16_t res = 0;
    while (len-- > 0) {
        res = res | (std::int8_t)memory[addr++] << shift;
        shift += 8;
    }
    return res;
}

std::uint16_t readUInt(const std::vector<std::uint8_t>& memory, std::uint16_t addr, int len) {
    int shift = 0;
    std::uint16_t res = 0;
    while (len-- > 0) {
        res = res | memory[addr++] << shift;
        shift += 8;
    }
    return res;
}

double readFloat(const std::vector<std::uint8_t>& memory, std::uint16_t addr) {
    // See https://www.c64-wiki.com/wiki/Floating_point_arithmetic#Representation_in_the_C-64

    std::int16_t e = memory[addr];
    if (e == 0) {
        return 0.0;
    }
    e -= 128;
    int sign = memory[addr+1] & 0x80;

    std::uint32_t m4 = memory[addr+1] | 0x80;
    std::uint32_t m3 = memory[addr+2];
    std::uint32_t m2 = memory[addr+3];
    std::uint32_t m1 = memory[addr+4];

    double res = (sign > 0 ? -1 : 1)*(m4*pow(2,-8) + m3*pow(2,-16) + m2*pow(2,-24) + m1*pow(2,-32)) * pow(2, e);
    return res;
}

QString readString(const std::vector<std::uint8_t>& memory, std::uint16_t addr, int len) {
    QString res;
    while (len-- > 0) {
        res += QChar(0xee00 + memory[addr++]);
    }
    return res;
}

QString readBytes(const std::vector<std::uint8_t>& memory, std::uint16_t addr, int len) {
    QString res;
    while (len-- > 0) {
        res += QString::asprintf("%02x ", memory[addr++]);
    }
    return res.trimmed();
}

QString removeTrailingZeroes(const QString& s) {
    if (s.indexOf(".") == -1) {
        // no decimal point, so no zero trimming!
        return s;
    }
    int end = s.length()-1;
    while (end >= 0 && s[end] == '0') {
        end--;
    }
    if (end != s.length()-1) {
        // at least 1 zero removed
        return s.left(end+2); // end is *offset* of last non-zero, so +2 to keep exactly one zero
    }
    return s;
}

QString cbmDoubleToString(double d) {
    bool scientific = d != 0.0 && (fabs(d) <= 1e-3 || fabs(d) >= 1e9);
    QString s = QString::asprintf(scientific ? "%.9e" : "%.9f", d);
    return removeTrailingZeroes(s);
}

}

QString Watch::asString(const std::unordered_map<std::uint16_t, std::vector<std::uint8_t>>& memory) const {
    const std::vector<std::uint8_t>& mem = memory.at(0); // TODO use banks!
    switch(viewType) {
    case INT:
        return QString::asprintf("%d", readInt(mem, addrStart, len));
    case UINT:
        return QString::asprintf("%d", readUInt(mem, addrStart, len));
    case UINT_HEX:
        return QString::asprintf(QString::asprintf("%%0%dx", len*2).toStdString().c_str(), readUInt(mem, addrStart, len));
    case FLOAT:
        return cbmDoubleToString(readFloat(mem, addrStart));
    case CHARS:
        return readString(mem, addrStart, len);
    case BYTES:
        return readBytes(mem, addrStart, len);
    }
    return "WTF???";
};

}
