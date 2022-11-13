#include "fonts.h"

#include <QFontDatabase>

namespace vicedebug {

QFont Fonts::robotoMono_;

void Fonts::init() {
    int id = QFontDatabase::addApplicationFont(":/fonts/RobotoMono.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    robotoMono_ = QFont(family);
}

const QFont& Fonts::robotoMono() {
    return robotoMono_;
}

}
