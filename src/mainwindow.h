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
#include "widgets/memorywidget.h"
#include "controller.h"

namespace vicedebug {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Controller* controller, QWidget *parent);
    ~MainWindow();

private slots:
    // Action slots
    void onConnectClicked();
    void onDisconnectClicked();
    void onLoadSymbolsClicked();
    void onContinueClicked();
    void onPauseClicked();
    void onStepInClicked();
    void onStepOutClicked();
    void onStepOverClicked();
    void onAboutClicked();

    // Other slots
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& state);

    void onConnected(const MachineState& state);
    void onConnectionFailed();
    void onDisconnected();

private:
    void createActions();
    void updateUiState();
    void createToolBar();
    void createMenuBar();
    void createMainUI();

    void updateDebugControlButtons();

    MemoryWidget* memoryWidget_;

    QAction* connectAction_;
    QAction* disconnectAction_;
    QAction* loadSymbolsAction_;

    // Debug actions
    QAction* continueAction_;
    QAction* pauseAction_;
    QAction* stepInAction_;
    QAction* stepOutAction_;
    QAction* stepOverAction_;

    // Find actions
    QAction* findTextAction_;
    QAction* findHexAction_;

    bool emulatorRunning_;

    Controller* controller_;
};

}
