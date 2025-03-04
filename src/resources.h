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

#include <QFont>

namespace vicedebug {

class Resources {

public:
    static void init();
    static const QFont& robotoMonoFont();
    static const QFont& c64Font();
    static QIcon loadColoredIcon(QColor color, QString iconPath);

private:
    static QFont makeFont(const QString& path);

    static QFont robotoMono_;
    static QFont c64_;
};

}
