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

#include "breakpointdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>

namespace vicedebug {

BreakpointDialog::BreakpointDialog(SymTable *symtab, QWidget* parent) : symtab_(symtab), QDialog(parent) {
    setupUI();
    setWindowTitle("Add breakpoint...");
}

BreakpointDialog::BreakpointDialog(Breakpoint breakpoint, SymTable *symtab, QWidget* parent) : symtab_(symtab), QDialog(parent) {
    setupUI();
    addrStart_->setText(QString::asprintf("%04X", breakpoint.addrStart));
    addrEnd_->setText(breakpoint.addrEnd == breakpoint.addrStart ? "" : QString::asprintf("%04X", breakpoint.addrEnd));
    typeExecute_->setChecked(breakpoint.op & Breakpoint::Type::EXEC);
    typeRead_->setChecked(breakpoint.op & Breakpoint::Type::READ);
    typeWrite_->setChecked(breakpoint.op & Breakpoint::Type::WRITE);
    setWindowTitle("Edit breakpoint...");
}

void BreakpointDialog::setupUI() {
    QVBoxLayout* vLayout = new QVBoxLayout();

    addrStart_ = new QLineEdit();
    addrStart_->setMaxLength(4);
    addrEnd_ = new QLineEdit();
    addrEnd_->setMaxLength(4);

    QFontMetrics fm = addrStart_->fontMetrics();
    int w = fm.boundingRect("0000").width();
    addrStart_->setFixedWidth(w + 10); // some slack
    addrEnd_->setFixedWidth(w + 10); // some slack
    connect(addrStart_, &QLineEdit::textChanged, this, &BreakpointDialog::enableControls);
    connect(addrEnd_, &QLineEdit::textChanged, this, &BreakpointDialog::enableControls);

    typeExecute_ = new QCheckBox(tr("Execute"));
    typeRead_ = new QCheckBox(tr("Read"));
    typeWrite_ = new QCheckBox(tr("Write"));
    QHBoxLayout* typeGroupLayout = new QHBoxLayout();
    typeGroupLayout->addWidget(typeExecute_);
    typeGroupLayout->addWidget(typeRead_);
    typeGroupLayout->addWidget(typeWrite_);
    typeGroupLayout->addStretch(1);
    QGroupBox* typeGroup = new QGroupBox(tr("Break on..."));
    typeGroup->setLayout(typeGroupLayout);

    QHBoxLayout* addrRangeLayout = new QHBoxLayout();
    addrRangeLayout->addWidget(new QLabel("From "));
    addrRangeLayout->addWidget(addrStart_);
    addrRangeLayout->addWidget(new QLabel("to"));
    addrRangeLayout->addWidget(addrEnd_);
    addrRangeLayout->addSpacing(20);
    QGroupBox* addrRangeGroup = new QGroupBox(tr("Address range..."));
    addrRangeGroup->setLayout(addrRangeLayout);

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

    vLayout->addWidget(addrRangeGroup);
    vLayout->addWidget(typeGroup);
    vLayout->addSpacing(10);
    vLayout->addLayout(buttons);

    setLayout(vLayout);

    enableControls();
}

Breakpoint BreakpointDialog::breakpoint() {
    return bp_;
}

std::uint16_t BreakpointDialog::parseAddress(QString str, bool& ok) {
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

void BreakpointDialog::enableControls() {
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

void BreakpointDialog::fillBreakpoint() {
    bool ok;
    bp_.addrStart = parseAddress(addrStart_->text(), ok);
    bp_.addrEnd = parseAddress(addrEnd_->text(), ok);
    if (!ok) {
        bp_.addrEnd = bp_.addrStart;
    }
    bp_.op = (typeExecute_->isChecked() ? Breakpoint::Type::EXEC : 0)
            | (typeRead_->isChecked() ? Breakpoint::Type::READ : 0)
            | (typeWrite_->isChecked() ? Breakpoint::Type::WRITE : 0);
}

}
