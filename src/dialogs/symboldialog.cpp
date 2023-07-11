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

#include "symboldialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QList>

namespace vicedebug {

SymbolDialog::SymbolDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Add symbol...");
}

SymbolDialog::SymbolDialog(const std::string& symbol, std::uint16_t address, QWidget* parent)
    : QDialog(parent) {
    setupUI();
    addressEdit_->setText(QString::asprintf("%04X", address));
    labelEdit_->setText(symbol.c_str());
    setWindowTitle("Edit symbol...");
}

void SymbolDialog::setupUI() {
    QVBoxLayout* vLayout = new QVBoxLayout();

    labelEdit_ = new QLineEdit();
    QFontMetrics fm = labelEdit_->fontMetrics();
    int w = fm.boundingRect("1234567890123456").width();
    labelEdit_->setFixedWidth(w + 10); // some slack

    connect(labelEdit_, &QLineEdit::textChanged, this, [this]() { enableControls(); });

    addressEdit_ = new QLineEdit();
    addressEdit_->setMaxLength(4);
    fm = addressEdit_->fontMetrics();
    w = fm.boundingRect("0000").width();
    addressEdit_->setFixedWidth(w + 10); // some slack

    connect(addressEdit_, &QLineEdit::textChanged, this, [this]() { enableControls(); });

    QHBoxLayout* elements = new QHBoxLayout();
    elements->addWidget(new QLabel("Synmbol:"));
    elements->addWidget(labelEdit_);
    elements->addWidget(new QLabel("Address:"));
    elements->addWidget(addressEdit_);

    QHBoxLayout* buttons = new QHBoxLayout();
    okBtn_ = new QPushButton("Ok");
    connect(okBtn_, &QPushButton::clicked, this, [this] {
        fillValues();
        accept();
    });
    cancelBtn_ = new QPushButton("Cancel");
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addStretch();
    buttons->addWidget(okBtn_);
    buttons->addWidget(cancelBtn_);

    vLayout->addLayout(elements);
    vLayout->addSpacing(10);
    vLayout->addLayout(buttons);

    setLayout(vLayout);
    enableControls();
}

std::string SymbolDialog::label() {
    return label_;
}

std::uint16_t SymbolDialog::address() {
    return address_;
}


std::uint16_t SymbolDialog::parseAddress(QString str, bool& ok) {
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

void SymbolDialog::enableControls() {
    cancelBtn_->setEnabled(true);

    if (labelEdit_->text().trimmed().length() == 0) {
        okBtn_->setEnabled(false);
        return;
    }

    QString addrStr = addressEdit_->text().trimmed();

    bool ok;
    std::uint16_t addr = parseAddress(addrStr, ok);
    if (!ok) {
        okBtn_->setEnabled(false);
        return;
    }

    okBtn_->setEnabled(true);
}

void SymbolDialog::fillValues() {
    bool ok;

    address_ = parseAddress(addressEdit_->text(), ok);
    label_ = labelEdit_->text().toStdString();
}

}

