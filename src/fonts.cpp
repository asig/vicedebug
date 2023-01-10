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

#include "fonts.h"

#include <QFontDatabase>
#include <QString>
#include <QList>

namespace vicedebug {

QFont Fonts::robotoMono_;
QFont Fonts::c64_;

void Fonts::init() {
    robotoMono_ = makeFont(":/fonts/RobotoMono.ttf");
    c64_ = makeFont(":/fonts/C64_Pro_Mono-STYLE.ttf");
}

QFont Fonts::makeFont(const QString& path) {
    int id = QFontDatabase::addApplicationFont(path);
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    return QFont(family);
}

const QFont& Fonts::robotoMono() {
    return robotoMono_;
}

const QFont& Fonts::c64() {
    return c64_;
}
}
