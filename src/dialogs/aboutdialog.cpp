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

#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>

#include "config.h"

namespace vicedebug {

namespace {
constexpr const int kLogoW = 300;
constexpr const int kLogoH = 131;
}

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setupUI();
    setWindowTitle("About...");
}

void AboutDialog::setupUI() {
    QVBoxLayout* vLayout = new QVBoxLayout();

    auto logo = new QIcon(":/images/logo.svg");


    auto logoLabel = new QLabel();
    logoLabel->setPixmap(logo->pixmap(kLogoW,kLogoH));
    vLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);

    vLayout->addWidget(new QLabel(QString::asprintf("Version %d.%d", VERSION_MAJOR, VERSION_MINOR)), 0, Qt::AlignHCenter);
    vLayout->addWidget(new QLabel("© 2024 Andreas Signer <asigner@gmail.com>"), 0, Qt::AlignHCenter);
    vLayout->addWidget(new QLabel("<a href=\"https://github.com/asig/vicedebug\">https://github.com/asig/vicedebug</a>"), 0, Qt::AlignHCenter);
    auto okBtn = new QPushButton("Ok");
    connect(okBtn, &QPushButton::clicked, this, [this] {
        accept();
    });

    vLayout->addWidget(okBtn, Qt::AlignHCenter);
    setLayout(vLayout);
}

}
