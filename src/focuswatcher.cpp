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

#include "focuswatcher.h"

#include <QEvent>

namespace vicedebug {

FocusWatcher::FocusWatcher(QObject *parent)
    : QObject{parent}
{
    if (parent) {
        parent->installEventFilter(this);
    }
}

bool FocusWatcher::eventFilter(QObject *obj, QEvent *event) {
    switch(event->type()) {
    case QEvent::FocusIn:
        emit focusAquired();
        break;
    case QEvent::FocusOut:
        emit focusLost();
        break;
    }
    return false;
}

} // namespace vicedebug
