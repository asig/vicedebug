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

#include "addbreakpointdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

namespace vicedebug {

AddBreakpointDialog::AddBreakpointDialog(QWidget* parent) : QDialog(parent) {
    QVBoxLayout* vLayout = new QVBoxLayout();

    addrStart_ = new QLineEdit();
    addrEnd_ = new QLineEdit();

    connect(addrStart_, &QLineEdit::textChanged, this, &AddBreakpointDialog::enableControls);
    connect(addrEnd_, &QLineEdit::textChanged, this, &AddBreakpointDialog::enableControls);

    breakpointType_ = new QComboBox();
    breakpointType_->addItem("Execute", QVariant(Breakpoint::Type::EXEC));
    breakpointType_->addItem("Read", QVariant(Breakpoint::Type::READ));
    breakpointType_->addItem("Write", QVariant(Breakpoint::Type::WRITE));

    QHBoxLayout* data = new QHBoxLayout();
    data->addWidget(new QLabel("Address: from "));
    data->addWidget(addrStart_);
    data->addWidget(new QLabel("to"));
    data->addWidget(addrEnd_);
    data->addSpacing(20);
    data->addWidget(new QLabel("Type:"));
    data->addWidget(breakpointType_);

    QHBoxLayout* buttons = new QHBoxLayout();
    okBtn_ = new QPushButton("Ok");
    connect(okBtn_, &QPushButton::clicked, this, [this] {
        fillBreakpoint();
        accept();
    });
    cancelBtn_ = new QPushButton("Cancel");
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addStretch();
    buttons->addWidget(okBtn_);
    buttons->addWidget(cancelBtn_);

    vLayout->addLayout(data);
    vLayout->addSpacing(10);
    vLayout->addLayout(buttons);

    setLayout(vLayout);

    setWindowTitle("Add Breakpoint...");
    enableControls();
}

Breakpoint AddBreakpointDialog::breakpoint() {
    return bp_;
}

std::uint16_t AddBreakpointDialog::parseAddress(QString str, bool& ok) {
    str = str.trimmed();
    if (str.length() == 0) {
        ok = false;
        return 0;
    }
    int base = 16;
    if (str[0] == '+') {
        str = str.mid(1);
        base = 10;
    }
    uint res = str.toUInt(&ok, base);
    if (res > 0xffff) {
        res = 0;
        ok = false;
    }
    return res;
}

void AddBreakpointDialog::enableControls() {
    cancelBtn_->setEnabled(true);

    QString addrStartStr = addrStart_->text().trimmed();
    QString addrEndStr = addrEnd_->text().trimmed();

    bool ok;
    std::uint16_t addrStart = parseAddress(addrStartStr, ok);
    if (!ok) {
        okBtn_->setEnabled(false);
        return;
    }

    if (addrEndStr.length() == 0) {
        okBtn_->setEnabled(true);
        return;
    }

    std::uint16_t addrEnd = parseAddress(addrEndStr, ok);
    if (!ok) {
        okBtn_->setEnabled(false);
        return;
    }
    okBtn_->setEnabled(addrEnd >= addrStart);
}

void AddBreakpointDialog::fillBreakpoint() {
    bool ok;
    bp_.addrStart = parseAddress(addrStart_->text(), ok);
    bp_.addrEnd = parseAddress(addrEnd_->text(), ok);
    if (!ok) {
        bp_.addrEnd = bp_.addrStart;
    }
    bp_.op = breakpointType_->currentData().toInt();
}

}
