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

#include <string>

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

namespace vicedebug {

class SymbolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SymbolDialog(QWidget* parent);
    explicit SymbolDialog(const std::string& label, std::uint16_t address, QWidget* parent);

    std::string label();
    std::uint16_t address();

private:
    void setupUI();
    std::uint16_t parseAddress(QString str, bool& ok);
    void enableControls();
    void fillValues();

    QLineEdit* labelEdit_;
    QLineEdit* addressEdit_;

    QPushButton* okBtn_;
    QPushButton* cancelBtn_;

    std::string label_;
    std::uint16_t address_;
};

}
