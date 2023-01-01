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

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

#include "breakpoints.h"

namespace vicedebug {

class BreakpointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BreakpointDialog(QWidget* parent);
    explicit BreakpointDialog(QWidget* parent, Breakpoint bp);

    Breakpoint breakpoint();

private:
    void setupUI();
    std::uint16_t parseAddress(QString str, bool& ok);
    void enableControls();
    void fillBreakpoint();

    QLineEdit* addrStart_;
    QLineEdit* addrEnd_;
    QCheckBox* typeExecute_;
    QCheckBox* typeRead_;
    QCheckBox* typeWrite_;

    QPushButton* okBtn_;
    QPushButton* cancelBtn_;

    Breakpoint bp_;
};

}
