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

#include "watchdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QList>

namespace vicedebug {

WatchDialog::WatchDialog(Banks banks, QWidget* parent)
    : banks_(banks), QDialog(parent) {
    setupUI();
    setWindowTitle("Add watch...");
}

WatchDialog::WatchDialog(Banks banks, Watch watch, QWidget* parent)
    : banks_(banks), QDialog(parent) {
    setupUI();

    addrStart_->setText(QString::asprintf("%04X", watch.addrStart));
    QString lengthStr = "";
    if (watch.viewType == Watch::ViewType::BYTES || watch.viewType == Watch::ViewType::CHARS || watch.viewType == Watch::ViewType::FLOAT) {
        lengthStr = QString::asprintf("%d", watch.len);
    }
    length_->setText(lengthStr);

    int i;
    for (i = 0; i < viewtype_->count(); i++) {
        QList<QVariant> attrs = viewtype_->itemData(i).toList();
        if (attrs.at(0).toInt() != watch.viewType) {
            continue;
        }
        if (watch.viewType == Watch::ViewType::BYTES || watch.viewType == Watch::ViewType::CHARS || watch.viewType == Watch::ViewType::FLOAT) {
            // no need for length to match
            break;
        }
        if (attrs.at(1).toInt() == watch.len) {
            break;
        }
    }
    viewtype_->setCurrentIndex(i);

    for (i = 0; i < bank_->count(); i++) {
        if (bank_->itemData(i).toUInt() == watch.bankId) {
            bank_->setCurrentIndex(i);
            break;
        }
    }

    setWindowTitle("Edit watch...");
}

void WatchDialog::setupUI() {
    QVBoxLayout* vLayout = new QVBoxLayout();

    addrStart_ = new QLineEdit();
    addrStart_->setMaxLength(6);
    QFontMetrics fm = addrStart_->fontMetrics();
    int w = fm.boundingRect("0000").width();
    addrStart_->setFixedWidth(w + 10); // some slack

    connect(addrStart_, &QLineEdit::textChanged, this, &WatchDialog::onAddrStartChanged);

    length_ = new QLineEdit();
    length_->setMaxLength(6);
    fm = length_->fontMetrics();
    w = fm.boundingRect("0000").width();
    length_->setFixedWidth(w + 10); // some slack

    connect(length_, &QLineEdit::textChanged, this, &WatchDialog::onLengthChanged);

    viewtype_ = new QComboBox();
    viewtype_->addItem("int8", QVariant( { QVariant(Watch::ViewType::INT), QVariant(1) } ));
    viewtype_->addItem("uint8", QVariant( { QVariant(Watch::ViewType::UINT), QVariant(1) } ));
    viewtype_->addItem("uint8 (hex)", QVariant( { QVariant(Watch::ViewType::UINT_HEX), QVariant(1) } ));
    viewtype_->addItem("int16", QVariant( { QVariant(Watch::ViewType::INT), QVariant(2) } ));
    viewtype_->addItem("uint16", QVariant( { QVariant(Watch::ViewType::UINT), QVariant(2) } ));
    viewtype_->addItem("uint16 (hex)", QVariant( { QVariant(Watch::ViewType::UINT_HEX), QVariant(2) } ));
    viewtype_->addItem("float", QVariant( { QVariant(Watch::ViewType::FLOAT), QVariant(5) } ));
    viewtype_->addItem("string", QVariant( { QVariant(Watch::ViewType::CHARS), QVariant(0) } ));
    viewtype_->addItem("bytes", QVariant( { QVariant(Watch::ViewType::BYTES), QVariant(0) } ));
    connect(viewtype_, &QComboBox::currentIndexChanged, this, &WatchDialog::onViewtypeChanged);

    bank_ = new QComboBox();
    for (Bank b : banks_) {
        bank_->addItem(b.name.c_str(), QVariant(b.id));
    }
    connect(viewtype_, &QComboBox::currentIndexChanged, this, &WatchDialog::onViewtypeChanged);

    QHBoxLayout* elements = new QHBoxLayout();
    elements->addWidget(new QLabel("Bank:"));
    elements->addWidget(bank_);
    elements->addWidget(new QLabel("Address:"));
    elements->addWidget(addrStart_);
    elements->addWidget(new QLabel("Type:"));
    elements->addWidget(viewtype_);
    elements->addWidget(new QLabel("Length:"));
    elements->addWidget(length_);

    QHBoxLayout* buttons = new QHBoxLayout();
    okBtn_ = new QPushButton("Ok");
    connect(okBtn_, &QPushButton::clicked, this, [this] {
        fillWatch();
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

    onViewtypeChanged(); // Make sure all the relevant field states are set.
}

Watch WatchDialog::watch() {
    return watch_;
}

void WatchDialog::onViewtypeChanged() {
    Watch::ViewType vt = (Watch::ViewType)viewtype_->currentData().toList().at(0).toInt();
    if (vt == Watch::CHARS || vt == Watch::BYTES) {
        length_->setEnabled(true);
    } else {
        length_->setText("");
        length_->setEnabled(false);
    }
    enableControls();
}

void WatchDialog::onAddrStartChanged() {
    enableControls();
}

void WatchDialog::onLengthChanged() {
    enableControls();
}

std::uint16_t WatchDialog::parseInt(QString str, int defaultBase, bool& ok) {
    str = str.trimmed();
    if (str.length() == 0) {
        ok = false;
        return 0;
    }
    int base = defaultBase;
    if (str[0] == '+') {
        str = str.mid(1);
        base = 10;
    } else if (str[0] == '$') {
        str = str.mid(1);
        base = 16;
    }
    uint res = str.toUInt(&ok, base);
    if (res > 0xffff) {
        res = 0;
        ok = false;
    }
    return res;
}

void WatchDialog::enableControls() {
    cancelBtn_->setEnabled(true);

    QString addrStartStr = addrStart_->text().trimmed();

    bool ok;
    std::uint16_t addrStart = parseInt(addrStartStr, 16, ok);
    if (!ok) {
        okBtn_->setEnabled(false);
        return;
    }

    ;
    Watch::ViewType vt = (Watch::ViewType)viewtype_->currentData().toList().at(0).toInt();
    if (vt == Watch::CHARS || vt == Watch::BYTES) {
        // needs to have a valid length
        std::uint16_t len = parseInt(length_->text().trimmed(), 10, ok);
        if (!ok) {
            okBtn_->setEnabled(false);
            return;
        }
    }

    okBtn_->setEnabled(true);
}

void WatchDialog::fillWatch() {
    bool ok;

    watch_.addrStart = parseInt(addrStart_->text(), 16, ok);
    QList<QVariant> attrs = viewtype_->currentData().toList();
    watch_.viewType = (Watch::ViewType)attrs.at(0).toInt();
    watch_.bankId = bank_->currentData().toUInt();
    watch_.len = attrs.at(1).toInt();
    if (watch_.viewType == Watch::ViewType::BYTES || watch_.viewType == Watch::ViewType::CHARS) {
        watch_.len = parseInt(length_->text(), 10, ok);
    }
}

}

