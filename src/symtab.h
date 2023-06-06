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

#include <string>
#include <memory>
#include <unordered_map>

namespace vicedebug {

class SymTable {
public:
    SymTable() = default;
    ~SymTable() = default;

    bool loadFromFile(const std::string filename);

    bool hasLabelForAddress(std::uint16_t addr) const;
    std::string labelForAddress(std::uint16_t addr);

    void dump();
private:
    std::unordered_map<std::uint16_t, std::string> symtable_;
};

}
