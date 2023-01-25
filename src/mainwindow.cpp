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

#include <iostream>

#include "mainwindow.h"

#include <QMenuBar>
#include <QToolBar>
#include <QScrollArea>
#include <QAbstractButton>
#include <QToolButton>
#include <QFuture>
#include <QMessageBox>
#include <QShortcut>

#include "resources.h"
#include "widgets/watcheswidget.h"
#include "widgets/breakpointswidget.h"
#include "widgets/registerswidget.h"
#include "widgets/memorywidget.h"

namespace {

// Colors for buttons: Colors from VS Code Debugger (Visual Studio 2017 Light color scheme):
// cont, step: 0d79c9,
// stop: a02a18
// restart: 3c8a3a
const QColor kCol1 = Qt::black;
const QColor kCol2 = QColor(0x0d, 0x79, 0xc9);
const QColor kCol3 = QColor(0xa0, 0x2a, 0x18);
const QColor kCol4 = QColor(0x3c, 0x8a, 0x3a);

}

namespace vicedebug {

MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QMainWindow(parent),
      controller_(controller)
{       
    createActions();
    createToolBar();
    createMenuBar();
    createMainUI();

//    stepInBtn_->setEnabled(false);
//    stepOutBtn_->setEnabled(false);
//    stepOverBtn_->setEnabled(false);
//    continueBtn_->setEnabled(false);
//    pauseBtn_->setEnabled(false);

    connect(controller_, &Controller::connected, this, &MainWindow::onConnected);
    connect(controller_, &Controller::connectionFailed, this, &MainWindow::onConnectionFailed);
    connect(controller_, &Controller::disconnected, this, &MainWindow::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &MainWindow::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &MainWindow::onExecutionResumed);

    updateUiState();
}

MainWindow::~MainWindow() {
}

void MainWindow::createActions() {
    QAction* a = new QAction(Resources::loadColoredIcon(kCol1, ":/images/other-icons/connected.svg"), tr("Connect"));
    a->setShortcut(QKeyCombination(Qt::ALT | Qt::Key_C));
    a->setCheckable(true);
    a->setToolTip(a->text() + " (" + tr("Alt+C") + ")");
    connectAction_ = a;
    connect(connectAction_, &QAction::triggered, this, &MainWindow::onConnectClicked);

    a = new QAction(Resources::loadColoredIcon(kCol1, ":/images/codicons/debug-disconnect.svg"), tr("Disonnect"));
    a->setCheckable(true);
    a->setVisible(false);
    disconnectAction_ = a;
    connect(disconnectAction_, &QAction::triggered, this, &MainWindow::onDisconnectClicked);

    emulatorRunning_ = false;
    a = new QAction(Resources::loadColoredIcon(kCol2, ":/images/codicons/debug-continue.svg"), tr("Continue"));
    a->setShortcut(Qt::Key_F5);
    a->setEnabled(false);
    a->setToolTip(a->text() + " (" + tr("F5") + ")");
    continueAction_ = a;
    connect(continueAction_, &QAction::triggered, this, &MainWindow::onContinueClicked);

    a = new QAction(Resources::loadColoredIcon(kCol2, ":/images/codicons/debug-pause.svg"), tr("Pause"));
    a->setShortcut(Qt::Key_F6);
    a->setEnabled(false);
    a->setToolTip(a->text() + " (" + tr("F6") + ")");
    pauseAction_ = a;
    connect(pauseAction_, &QAction::triggered, this, &MainWindow::onPauseClicked);

    a = new QAction(Resources::loadColoredIcon(kCol2, ":/images/codicons/debug-step-into.svg"), tr("Step into"));
    a->setShortcut(Qt::Key_F11);
    a->setEnabled(false);
    a->setToolTip(a->text() + " (" + tr("F11") + ")");
    stepInAction_ = a;
    connect(stepInAction_, &QAction::triggered, this, &MainWindow::onStepInClicked);

    a = new QAction(Resources::loadColoredIcon(kCol2, ":/images/codicons/debug-step-out.svg"), tr("Step out"));
    a->setShortcut(QKeyCombination(Qt::SHIFT | Qt::Key_F11));
    a->setEnabled(false);
    a->setToolTip(a->text() + " (" + tr("Shift-F11") + ")");
    stepOutAction_ = a;
    connect(stepOutAction_, &QAction::triggered, this, &MainWindow::onStepOutClicked);

    a = new QAction(Resources::loadColoredIcon(kCol2, ":/images/codicons/debug-step-over.svg"), tr("Step over"));
    a->setShortcut(Qt::Key_F10);
    a->setEnabled(false);
    a->setToolTip(a->text() + " (" + tr("F10") + ")");
    stepOverAction_ = a;
    connect(stepOverAction_, &QAction::triggered, this, &MainWindow::onStepOverClicked);

    // We start with only "continue" visible
    pauseAction_->setVisible(false);
}

void MainWindow::createToolBar() {
#ifdef Q_OS_MACOS
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    QToolBar* tb = new QToolBar("Toolbar", this);
    tb->addAction(connectAction_);
    tb->addAction(disconnectAction_);
    tb->addSeparator();
    tb->addAction(continueAction_);
    tb->addAction(pauseAction_);
    tb->addAction(stepOverAction_);
    tb->addAction(stepInAction_);
    tb->addAction(stepOutAction_);
    addToolBar(tb);
}

void MainWindow::createMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

//    menu->addAction(tr("Save layout..."), this, &MainWindow::saveLayout);
//    menu->addAction(tr("Load layout..."), this, &MainWindow::loadLayout);
//    menu->addAction(tr("Switch layout direction"),this, &MainWindow::switchLayoutDirection);

//    menu->addSeparator();
//    menu->addAction(tr("&Quit"), this, &QWidget::close);

//    mainWindowMenu = menuBar()->addMenu(tr("Main window"));

//    QAction *action = mainWindowMenu->addAction(tr("Animated docks"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & AnimatedDocks);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    action = mainWindowMenu->addAction(tr("Allow nested docks"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & AllowNestedDocks);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    action = mainWindowMenu->addAction(tr("Allow tabbed docks"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & AllowTabbedDocks);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    action = mainWindowMenu->addAction(tr("Force tabbed docks"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & ForceTabbedDocks);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    action = mainWindowMenu->addAction(tr("Vertical tabs"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & VerticalTabs);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    action = mainWindowMenu->addAction(tr("Grouped dragging"));
//    action->setCheckable(true);
//    action->setChecked(dockOptions() & GroupedDragging);
//    connect(action, &QAction::toggled, this, &MainWindow::setDockOptions);

//    QMenu *toolBarMenu = menuBar()->addMenu(tr("Tool bars"));
//    for (int i = 0; i < toolBars.count(); ++i)
//        toolBarMenu->addMenu(toolBars.at(i)->toolbarMenu());

//#ifdef Q_OS_MACOS
//    toolBarMenu->addSeparator();

//    action = toolBarMenu->addAction(tr("Unified"));
//    action->setCheckable(true);
//    action->setChecked(unifiedTitleAndToolBarOnMac());
//    connect(action, &QAction::toggled, this, &QMainWindow::setUnifiedTitleAndToolBarOnMac);
//#endif

//    dockWidgetMenu = menuBar()->addMenu(tr("&Dock Widgets"));

//    QMenu *aboutMenu = menuBar()->addMenu(tr("About"));
//    QAction *aboutAct = aboutMenu->addAction(tr("&About"), this, &MainWindow::about);
//    aboutAct->setStatusTip(tr("Show the application's About box"));

//    QAction *aboutQtAct = aboutMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
//    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::createMainUI() {
    RegistersWidget* registersWidget = new RegistersWidget(controller_, this);
    BreakpointsWidget* breakpointsWidget = new BreakpointsWidget(controller_, this);
    WatchesWidget* watchesWidget = new WatchesWidget(controller_, this);

    QSplitter* lowerPart = new QSplitter(Qt::Horizontal);
    lowerPart->addWidget(registersWidget);
    lowerPart->addWidget(breakpointsWidget);
    lowerPart->addWidget(watchesWidget);

    lowerPart->setStretchFactor(0, 0);
    lowerPart->setStretchFactor(1, 1);
    lowerPart->setStretchFactor(2, 1);


    MemoryWidget* memoryWidget = new MemoryWidget(controller_, this);
//    QScrollArea* memoryWidgetScrollArea = new QScrollArea();
//    memoryWidgetScrollArea->setBackgroundRole(QPalette::Dark);
//    memoryWidgetScrollArea->setWidget(memoryWdiget);
//    memoryWidgetScrollArea->setWidgetResizable(true);

//    QScrollArea* disassembltWidgetScrollArea = new QScrollArea();
//    disassembltWidgetScrollArea->setBackgroundRole(QPalette::Dark);
//    disassembly_ = new DisassemblyWidget(controller_, disassembltWidgetScrollArea);
//    disassembltWidgetScrollArea->setWidget(disassembly_);
    DisassemblyWidget* disassembly = new DisassemblyWidget(controller_, this);

    QSplitter* upperPart = new QSplitter(Qt::Horizontal);
    upperPart->addWidget(disassembly);
    upperPart->addWidget(memoryWidget);
    upperPart->setSizes({10000, 20000}); // Use large values, Qt seems to keep the ratio. See also https://stackoverflow.com/questions/43831474/how-to-equally-distribute-the-width-of-qsplitter

    QSplitter* all = new QSplitter(Qt::Vertical);
    all->addWidget(upperPart);
    all->addWidget(lowerPart);
    all->setSizes({20000, 5000});

    setCentralWidget(all);
}

void MainWindow::updateUiState() {
//    ui->connectBtn->setEnabled(ui->viceAddress->text().length() > 0);
}

void MainWindow::onConnectClicked() {
    connectAction_->setEnabled(false); // Will be re-enabled in the connection response
    controller_->connectToVice("127.0.0.1", 6502);
}

void MainWindow::onDisconnectClicked() {
    disconnectAction_->setEnabled(false); // Will be re-enabled in the connection response
    controller_->disconnect();
}

void MainWindow::onStepInClicked() {
    controller_->stepIn();
}

void MainWindow::onStepOutClicked() {
    controller_->stepOut();
}

void MainWindow::onStepOverClicked() {
    controller_->stepOver();
}

void MainWindow::onContinueClicked() {
    continueAction_->setEnabled(false); // Will be re-enabled in the response from the emulator
    controller_->resumeExecution();
}

void MainWindow::onPauseClicked() {
    pauseAction_->setEnabled(false); // Will be re-enabled in the response from the emulator
    controller_->pauseExecution();
}

void MainWindow::onConnected(const MachineState& machineState) {
    connectAction_->setVisible(false);
    connectAction_->setEnabled(false);

    disconnectAction_->setChecked(true);
    disconnectAction_->setVisible(true);
    disconnectAction_->setEnabled(true);

    emulatorRunning_ = false;
    updateDebugControlButtons();
}

void MainWindow::onConnectionFailed() {
    QMessageBox::warning(this, "Can't connect", "Can't connect to VICE.\nDid you start the emulator with\nthe --binarymonitor flag?");
    onDisconnected();
}

void MainWindow::onDisconnected() {
    disconnectAction_->setVisible(false);
    disconnectAction_->setEnabled(false);

    connectAction_->setChecked(false);
    connectAction_->setVisible(true);
    connectAction_->setEnabled(true);

    stepInAction_->setEnabled(false);
    stepOutAction_->setEnabled(false);
    stepOverAction_->setEnabled(false);
    continueAction_->setEnabled(false);
    continueAction_->setVisible(true);
    pauseAction_->setEnabled(false);
    pauseAction_->setVisible(false);
}

void MainWindow::updateDebugControlButtons() {
    stepInAction_->setEnabled(!emulatorRunning_);
    stepOutAction_->setEnabled(!emulatorRunning_);
    stepOverAction_->setEnabled(!emulatorRunning_);
    continueAction_->setEnabled(!emulatorRunning_);
    continueAction_->setVisible(!emulatorRunning_);
    pauseAction_->setEnabled(emulatorRunning_);
    pauseAction_->setVisible(emulatorRunning_);
}

void MainWindow::onExecutionResumed() {
    emulatorRunning_ = true;
    updateDebugControlButtons();
}

void MainWindow::onExecutionPaused(const MachineState& state) {
    emulatorRunning_ = false;
    updateDebugControlButtons();
}

}
