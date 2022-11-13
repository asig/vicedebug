#include "mainwindow.h"
#include "fonts.h"
#include "controller.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    vicedebug::Fonts::init();

    vicedebug::ViceClient viceClient(nullptr);
    vicedebug::Controller controller(&viceClient);

    vicedebug::MainWindow w(&controller, nullptr);
    w.show();
    return a.exec();
}
