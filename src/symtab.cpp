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

#include "symtab.h"

#include <fstream>
#include <sstream>
#include <string>

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

namespace vicedebug {

bool SymTable::loadFromFile(const std::string filename) {

    std::ifstream file(filename);
    if (!file) {
        return false;
    }

    std::unordered_map<std::string, std::uint16_t> addressForLabel;

    std::string l;
    while (std::getline(file, l)) {
        QString line = QString(l.c_str()).trimmed();
        if (line.startsWith(";")) {
            continue;
        }
        if (line.startsWith("al ") || line.startsWith("add_label ")) {
            // VICE monitor format:
            // "add_label C:1234 .whatever"
            QStringList parts = line.split(QRegularExpression("[ \t]+"));
            if (parts.length() != 3) {
                qDebug() << "SymTable::loadFromFile: bad line: " << line;
                continue;
            }
            QChar device = parts[1].at(0).toLower();
            if (device != 'c') {
                qDebug() << "SymTable::loadFromFile: wrong device on line: " << line;
                continue;
            }
            std::uint16_t address = parts[1].mid(2).toInt(nullptr, 16);
            std::string label = parts[2].mid(1).toStdString();
            addressForLabel[label] = address;
        } else if (line.contains("=")) {
            // ACME fprmat:
            // "labelname = $1234 ; Maybe a comment"
            auto idx = line.indexOf("=");
            std::string label = line.left(idx).trimmed().toStdString();
            line = line.mid(idx+1);
            idx = line.indexOf(";"); // Get rid of comment
            if (idx > -1) {
                line = line.mid(idx).trimmed();
            }
            std::uint16_t address = line.mid(1).trimmed().toInt(nullptr, 16);
            addressForLabel[label] = address;
        } else {
            qDebug() << "SymTable::loadFromFile: bad line: " << line;
        }
    }
    addressForLabel_ = addressForLabel;
    emit symbolsChanged();
    return true;
}

void SymTable::set(const std::string& label, std::uint16_t address) {
    bool changed = false;
    if (!addressForLabel_.contains(label)) {
        // Label not yet in
        changed = true;
        addressForLabel_[label] = address;
    } else {
        changed = addressForLabel_[label] != address;
        addressForLabel_[label] = address;
    }
    if (changed) {
        emit symbolsChanged();
    }
}

void SymTable::remove(const std::string& label) {
    bool changed = addressForLabel_.contains(label);
    addressForLabel_.erase(label);
    if (changed) {
        emit symbolsChanged();
    }
}

bool SymTable::hasLabelForAddress(std::uint16_t addr) const {
    for (const auto& [label, a] : addressForLabel_) {
        if (a == addr) {
            return true;
        }
    }
    return false;
}

std::string SymTable::labelForAddress(std::uint16_t addr) {
    for (const auto& [label, a] : addressForLabel_) {
        if (a == addr) {
            return label;
        }
    }
    return "";
}

std::vector<std::string> SymTable::labels() const {
    std::vector<std::string> labels;
    for (const auto& [label, _] : addressForLabel_) {
        labels.push_back(label);
    }
    return labels;
}

std::vector<std::pair<std::string, std::uint16_t>> SymTable::elements() const {
    std::vector<std::pair<std::string, std::uint16_t>> res;
    for (const auto& p : addressForLabel_) {
        res.push_back(p);
    }
    return res;
}

void SymTable::dump() {
    for (const auto& [label, addr] : addressForLabel_) {
        qDebug() << label.c_str() << "\t" << QString::asprintf("%04x", addr).toStdString().c_str();
    }
}

}
