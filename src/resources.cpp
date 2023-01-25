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

#include "resources.h"

#include <QFontDatabase>
#include <QString>
#include <QList>
#include <QColor>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include <QTemporaryFile>

namespace vicedebug {

QFont Resources::robotoMono_;
QFont Resources::c64_;

void Resources::init() {
    robotoMono_ = makeFont(":/fonts/RobotoMono.ttf");
    c64_ = makeFont(":/fonts/C64_Pro_Mono-STYLE.ttf");
}

QFont Resources::makeFont(const QString& path) {
    int id = QFontDatabase::addApplicationFont(path);
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    return QFont(family);
}

QIcon Resources::loadColoredIcon(QColor color, QString iconPath) {
    QFile f(iconPath);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&f);
    QString content = in.readAll();
    QString colStr = QString::asprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    content.replace("currentColor", colStr);

    QTemporaryFile modified;
    modified.open();
    QTextStream out(&modified);
    out << content;
    out.flush();
    modified.close();

    return QIcon(modified.fileName());
}

const QFont& Resources::robotoMonoFont() {
    return robotoMono_;
}

const QFont& Resources::c64Font() {
    return c64_;
}
}
