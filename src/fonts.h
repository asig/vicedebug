#pragma once

#include <QFont>

namespace vicedebug {

class Fonts {

public:
    static void init();
    static const QFont& robotoMono();

private:
    static QFont robotoMono_;
};

}
