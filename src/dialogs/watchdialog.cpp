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

WatchDialog::WatchDialog(QWidget* parent) : QDialog(parent) {
    setupUI();
    setWindowTitle("Add watch...");
}

WatchDialog::WatchDialog(QWidget* parent, Watch watch) : QDialog(parent) {
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

    setWindowTitle("Edit watch...");
}

void WatchDialog::setupUI() {
    QVBoxLayout* vLayout = new QVBoxLayout();

    addrStart_ = new QLineEdit();
    addrStart_->setMaxLength(6);
    QFontMetrics fm = addrStart_->fontMetrics();
    int w = fm.boundingRect("0000").width();
    addrStart_->setFixedWidth(w + 10); // some slack

    connect(addrStart_, &QLineEdit::textChanged, this, &WatchDialog::enableControls);

    length_ = new QLineEdit();
    length_->setMaxLength(6);
    fm = length_->fontMetrics();
    w = fm.boundingRect("0000").width();
    length_->setFixedWidth(w + 10); // some slack

    viewtype_ = new QComboBox();
    viewtype_->insertItem(0, "int8", QVariant( { QVariant(Watch::ViewType::INT), QVariant(1) } ));
    viewtype_->insertItem(1, "int8 (hex)", QVariant( { QVariant(Watch::ViewType::INT_HEX), QVariant(1) } ));
    viewtype_->insertItem(2, "uint8", QVariant( { QVariant(Watch::ViewType::UINT), QVariant(1) } ));
    viewtype_->insertItem(3, "uint8 (hex)", QVariant( { QVariant(Watch::ViewType::UINT_HEX), QVariant(1) } ));
    viewtype_->insertItem(4, "int16", QVariant( { QVariant(Watch::ViewType::INT), QVariant(2) } ));
    viewtype_->insertItem(5, "int16 (hex)", QVariant( { QVariant(Watch::ViewType::INT_HEX), QVariant(2) } ));
    viewtype_->insertItem(6, "uint16", QVariant( { QVariant(Watch::ViewType::UINT), QVariant(2) } ));
    viewtype_->insertItem(7, "uint16 (hex)", QVariant( { QVariant(Watch::ViewType::UINT_HEX), QVariant(2) } ));
    viewtype_->insertItem(8, "float", QVariant( { QVariant(Watch::ViewType::FLOAT), QVariant(0) } ));
    viewtype_->insertItem(9, "string", QVariant( { QVariant(Watch::ViewType::CHARS), QVariant(0) } ));
    viewtype_->insertItem(10, "bytes", QVariant( { QVariant(Watch::ViewType::BYTES), QVariant(0) } ));

    QHBoxLayout* elements = new QHBoxLayout();
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

    enableControls();
}

Watch WatchDialog::watch() {
    return watch_;
}

std::uint16_t WatchDialog::parseAddress(QString str, bool& ok) {
    str = str.trimmed();
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

void WatchDialog::enableControls() {
    cancelBtn_->setEnabled(true);

    QString addrStartStr = addrStart_->text().trimmed();

    bool ok;
    std::uint16_t addrStart = parseAddress(addrStartStr, ok);
    if (!ok) {
        okBtn_->setEnabled(false);
        return;
    }

    ;
    Watch::ViewType vt = (Watch::ViewType)viewtype_->currentData().toList().at(0).toInt();
    if (vt == Watch::CHARS || vt == Watch::BYTES) {
        // needs to have a valid length
        std::uint16_t len = parseAddress(length_->text().trimmed(), ok);
        if (!ok) {
            okBtn_->setEnabled(false);
            return;
        }
    }

    okBtn_->setEnabled(true);
}

void WatchDialog::fillWatch() {
    bool ok;

    watch_.addrStart = parseAddress(addrStart_->text(), ok);
    QList<QVariant> attrs = viewtype_->currentData().toList();
    watch_.viewType = (Watch::ViewType)attrs.at(0).toInt();
    watch_.len = attrs.at(1).toInt();
    if (watch_.viewType == Watch::ViewType::BYTES || watch_.viewType == Watch::ViewType::CHARS) {
        watch_.len = parseAddress(length_->text(), ok);
    }
}

}

