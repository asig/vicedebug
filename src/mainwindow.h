#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QGroupBox>

#include "widgets/disassemblywidget.h"
#include "viceclient.h"
#include "controller.h"

namespace vicedebug {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Controller* controller, QWidget *parent);
    ~MainWindow();

private:
    void updateUiState();

private slots:
    void on_connectBtn_clicked();
    void on_viceAddress_textChanged(const QString &arg1);
    void connectClicked();

private:
    void createToolBar();
    void createMenuBar();
    void createMainUI();

    DisassemblyWidget disassembly_;
    Controller* controller_;
};

}
