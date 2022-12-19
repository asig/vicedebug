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

#include <QMainWindow>
#include <QSplitter>
#include <QGroupBox>
#include <QToolButton>

#include "widgets/disassemblywidget.h"
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
    void onConnectClicked();
    void onContinueClicked();
    void onPauseClicked();
    void onStepInClicked();
    void onStepOutClicked();
    void onStepOverClicked();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& state);

    void onConnected(const MachineState& state);
    void onConnectionFailed();
    void onDisconnected();

    void __DEBUG__Clicked();

private:
    void createToolBar();
    void createMenuBar();
    void createMainUI();

    void updateDebugControlButtons(bool running);

    DisassemblyWidget* disassembly_;

    QToolButton* connectBtn_;
    QToolButton* stepInBtn_;
    QToolButton* stepOutBtn_;
    QToolButton* stepOverBtn_;
    QToolButton* continueBtn_;
    QToolButton* pauseBtn_;

    QToolButton* __DEBUG__Btn_;

    Controller* controller_;
};

}
