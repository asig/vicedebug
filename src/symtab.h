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
#include <vector>
#include <memory>
#include <unordered_map>

#include <QObject>

namespace vicedebug {

class SymTable : public QObject {
    Q_OBJECT

public:
    SymTable() = default;
    ~SymTable() = default;

    bool loadFromFile(const std::string filename);
    void set(const std::string& label, std::uint16_t address);
    void remove(const std::string& label);

    bool hasLabelForAddress(std::uint16_t addr) const;
    std::string labelForAddress(std::uint16_t addr);

    std::vector<std::string> labels() const;
    std::vector<std::pair<std::string, std::uint16_t>> elements() const;

    void dump();

signals:
    void symbolsChanged();

private:
    std::unordered_map<std::string, std::uint16_t> addressForLabel_;
};

}
