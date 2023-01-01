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

#include "widgets/watcheswidget.h"
#include "widgets/breakpointswidget.h"
#include "widgets/registerswidget.h"
#include "widgets/memorywidget.h"

namespace vicedebug {

MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QMainWindow(parent),
      controller_(controller)
{   
    createToolBar();
    createMenuBar();
    createMainUI();

    stepInBtn_->setEnabled(false);
    stepOutBtn_->setEnabled(false);
    stepOverBtn_->setEnabled(false);
    continueBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);

    connect(controller_, &Controller::connected, this, &MainWindow::onConnected);
    connect(controller_, &Controller::connectionFailed, this, &MainWindow::onConnectionFailed);
    connect(controller_, &Controller::disconnected, this, &MainWindow::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &MainWindow::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &MainWindow::onExecutionResumed);

    updateUiState();
}

MainWindow::~MainWindow() {
}

void MainWindow::createToolBar() {
#ifdef Q_OS_MACOS
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    connectBtn_ = new QToolButton();
    connectBtn_->setCheckable(true);
    connectBtn_->setText("Connect");
    connectBtn_->setIcon(QIcon(":/images/other-icons/connected.svg"));
    connectBtn_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    connect(connectBtn_, &QAbstractButton::clicked, this, &MainWindow::onConnectClicked);

    stepInBtn_ = new QToolButton();
    stepInBtn_->setText("Step into");
    stepInBtn_->setIcon(QIcon(":/images/codicons/debug-step-into.svg"));
    connect(stepInBtn_, &QAbstractButton::clicked, this, &MainWindow::onStepInClicked);

    stepOutBtn_ = new QToolButton();
    stepOutBtn_->setText("Step out");
    stepOutBtn_->setIcon(QIcon(":/images/codicons/debug-step-out.svg"));
    connect(stepOutBtn_, &QAbstractButton::clicked, this, &MainWindow::onStepOutClicked);

    stepOverBtn_ = new QToolButton();
    stepOverBtn_->setText("Step over");
    stepOverBtn_->setIcon(QIcon(":/images/codicons/debug-step-over.svg"));
    connect(stepOverBtn_, &QAbstractButton::clicked, this, &MainWindow::onStepOverClicked);

    continueBtn_ = new QToolButton();
    continueBtn_->setText("Continue");
    continueBtn_->setIcon(QIcon(":/images/codicons/debug-continue.svg"));
    connect(continueBtn_, &QAbstractButton::clicked, this, &MainWindow::onContinueClicked);

    pauseBtn_ = new QToolButton();
    pauseBtn_->setText("Pause");
    pauseBtn_->setIcon(QIcon(":/images/codicons/debug-pause.svg"));
    connect(pauseBtn_, &QAbstractButton::clicked, this, &MainWindow::onPauseClicked);

    __DEBUG__Btn_ = new QToolButton();
    __DEBUG__Btn_->setCheckable(true);
    __DEBUG__Btn_->setText("DEBUG");
    connect(__DEBUG__Btn_, &QAbstractButton::clicked, this, &MainWindow::__DEBUG__Clicked);

    QToolBar* tb = new QToolBar("Toolbar", this);
    tb->addWidget(connectBtn_);
    tb->addSeparator();
    tb->addWidget(continueBtn_);
    tb->addWidget(pauseBtn_);
    tb->addWidget(stepOverBtn_);
    tb->addWidget(stepInBtn_);
    tb->addWidget(stepOutBtn_);
    tb->addSeparator();
    tb->addWidget(__DEBUG__Btn_);
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
    connectBtn_->setEnabled(false); // We be re-enabled in the connection response
    if (!controller_->isConnected()) {
        controller_->connectToVice("127.0.0.1", 6502);
    } else {
        controller_->disconnect();
    }
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

void MainWindow::onPauseClicked() {
    controller_->pauseExecution();
}

void MainWindow::onContinueClicked() {
    controller_->resumeExecution();
}

void MainWindow::__DEBUG__Clicked() {
    if (controller_->isConnected()) {
//        controller_->listBreakpoints();
    }
}

void MainWindow::onConnected(const MachineState& machineState) {
    connectBtn_->setChecked(true);
    connectBtn_->setEnabled(true);
    connectBtn_->setText("Disconnect");
    connectBtn_->setIcon(QIcon(":/images/codicons/debug-disconnect.svg"));
    updateDebugControlButtons(false);
}

void MainWindow::onConnectionFailed() {
    QMessageBox::warning(this, "Can't connect", "Can't connect to VICE.\nDid you start the emulator with\nthe --binarymonitor flag?");
    onDisconnected();
}

void MainWindow::onDisconnected() {
    connectBtn_->setChecked(false);
    connectBtn_->setEnabled(true);
    connectBtn_->setText("Connect");
    connectBtn_->setIcon(QIcon(":/images/other-icons/connected.svg"));

    stepInBtn_->setEnabled(false);
    stepOutBtn_->setEnabled(false);
    stepOverBtn_->setEnabled(false);
    continueBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);
}

void MainWindow::updateDebugControlButtons(bool running) {
    stepInBtn_->setEnabled(!running);
    stepOutBtn_->setEnabled(!running);
    stepOverBtn_->setEnabled(!running);
    continueBtn_->setEnabled(!running);
    pauseBtn_->setEnabled(running);
}

void MainWindow::onExecutionResumed() {
    updateDebugControlButtons(true);
}

void MainWindow::onExecutionPaused(const MachineState& state) {
    updateDebugControlButtons(false);
}

}
