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


    std::unordered_map<std::uint16_t, std::string> t;
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
            QString label = parts[2].mid(1);
            t[address] = label.toStdString();
        } else if (line.contains("=")) {
            // ACME fprmat:
            // "labelname = $1234 ; Maybe a comment"
            auto idx = line.indexOf("=");
            QString label = line.left(idx).trimmed();
            line = line.mid(idx+1);
            idx = line.indexOf(";"); // Get rid of comment
            if (idx > -1) {
                line = line.mid(idx).trimmed();
            }
            std::uint16_t address = line.mid(1).trimmed().toInt(nullptr, 16);
            t[address] = label.toStdString();
        } else {
            qDebug() << "SymTable::loadFromFile: bad line: " << line;
        }
    }
    symtable_ = t;
    return true;
}

bool SymTable::hasLabelForAddress(std::uint16_t addr) const {
    return symtable_.contains(addr);
}

std::string SymTable::labelForAddress(std::uint16_t addr) {
    return symtable_[addr];
}

void SymTable::dump() {
    for (const auto& [addr, label] : symtable_) {
        qDebug() << label.c_str() << "\t" << QString::asprintf("%04x", addr).toStdString().c_str();
    }
}

}
