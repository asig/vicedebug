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

#include "tooltipgenerators.h"

namespace vicedebug {

namespace {

QString formatAddrRange(std::uint16_t addrStart, std::uint16_t addrEnd) {
    if (addrStart == addrEnd) {
        return QString::asprintf("%04x", addrStart);
    }
    return QString::asprintf("%04x - %04x", addrStart, addrEnd);
}

QString formatBpOp(const Breakpoint& bp) {
    QString s;
    std::vector<QString> ops;
    if (bp.op & Breakpoint::Type::READ) {
        ops.push_back("<b>Load</b>");
    }
    if (bp.op & Breakpoint::Type::WRITE) {
        ops.push_back("<b>Store</b>");
    }
    if (bp.op & Breakpoint::Type::EXEC) {
        ops.push_back("<b>Execute</b>");
    }
    switch(ops.size()) {
    case 3:
        return ops[0] + ", " + ops[1] + ", and " + ops[2];
    case 2:
        return ops[0] + " and " + ops[1];
    case 1:
        return ops[0];
    default:
        return "WTF????";
    }
}

}

QString BreakpointTooltipGenerator::generate() const {
    QString tooltip = QString::asprintf("<b>Breakpoint %d</b><br>", bp_.number);
    if (!bp_.enabled) {
        tooltip += "Disabled<br>";
    }
    tooltip += "Address: " + formatAddrRange(bp_.addrStart, bp_.addrEnd) + "<br>";
    tooltip += "Break on " + formatBpOp(bp_);
    return tooltip;
}

QString WatchTooltipGenerator::generate() const {
    QString tooltip = QString::asprintf("<b>Watch %d</b><br>", w_.number);
    tooltip += "Bank: " + bankName_ + "<br>";
    tooltip += "Address: " + formatAddrRange(w_.addrStart, w_.addrStart + w_.len - 1) + "<br>";
    tooltip += "Type: " + w_.viewTypeAsString();
    return tooltip;
}

}
