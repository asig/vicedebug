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

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

#include "machinestate.h"
#include "watches.h"
#include "symtab.h"

namespace vicedebug {

class WatchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WatchDialog(Banks banks, SymTable* symtab, QWidget* parent);
    explicit WatchDialog(Banks banks, Watch watch, SymTable* symtab, QWidget* parent);

    Watch watch();

private slots:
    void onViewtypeChanged();
    void onAddrStartChanged();
    void onLengthChanged();

private:
    void setupUI();
    std::uint16_t parseInt(QString str, int defaultBase, bool& ok);
    void enableControls();
    void fillWatch();

    QLineEdit* addrStart_;
    QLineEdit* length_;
    QComboBox* bank_;
    QComboBox* viewtype_;

    QPushButton* okBtn_;
    QPushButton* cancelBtn_;

    SymTable* symtab_;
    Banks banks_;
    Watch watch_;
};

}
