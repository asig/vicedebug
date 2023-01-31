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

#pragma once

#include <QString>

#include "machinestate.h"
#include "breakpoints.h"
#include "watches.h"

namespace vicedebug {

class TooltipGenerator {
public:
    virtual QString generate() const = 0;
};

class BreakpointTooltipGenerator : public TooltipGenerator {
public:
    BreakpointTooltipGenerator(const Breakpoint& bp)
        : bp_(bp) {}

    QString generate() const override;

private:
    Breakpoint bp_;
};

class WatchTooltipGenerator : public TooltipGenerator {
public:
    WatchTooltipGenerator(const Watch& w, const Banks& banks)
        : w_(w)
    {
        bankName_ = "<unknown>";
        for (const auto& b : banks) {
            if (b.id == w_.bankId) {
                bankName_ = b.name.c_str();
                break;
            }
        }
    }

    QString generate() const override;

private:
    Watch w_;
    QString bankName_;
};

}
