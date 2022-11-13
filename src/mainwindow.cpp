#include <iostream>

#include "mainwindow.h"
#include "widgets/watcheswidget.h"
#include "widgets/breakpointswidget.h"
#include "widgets/registerswidget.h"

#include <QMenuBar>
#include <QToolBar>
#include <QAbstractButton>
#include <QToolButton>
#include <QFuture>

namespace vicedebug {

MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QMainWindow(parent),
      disassembly_(controller, this),
      controller_(controller)
{   
    createToolBar();
    createMenuBar();
    createMainUI();

    updateUiState();
}

MainWindow::~MainWindow()
{
}

void MainWindow::createToolBar() {
#ifdef Q_OS_MACOS
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    QToolButton* connectBtn = new QToolButton();
    connectBtn->setCheckable(true);
    connectBtn->setText("Connect");
    connectBtn->setIcon(QIcon(":/images/other_icons/connect.svg"));
    connect(connectBtn, &QAbstractButton::clicked, this, &MainWindow::connectClicked);

    QToolBar* tb = new QToolBar("Toolbar", this);
    tb->addWidget(connectBtn);
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

    QSplitter* verSplitter = new QSplitter(Qt::Vertical);
    verSplitter->addWidget(registersWidget);
    verSplitter->addWidget(breakpointsWidget);
    verSplitter->addWidget(watchesWidget);

    verSplitter->setStretchFactor(0, 0);
    verSplitter->setStretchFactor(1, 1);
    verSplitter->setStretchFactor(2, 1);

    QSplitter* horSplitter = new QSplitter(Qt::Horizontal);
    horSplitter->addWidget(&disassembly_);
    horSplitter->addWidget(verSplitter);

    setCentralWidget(horSplitter);
}

void MainWindow::updateUiState() {
//    ui->connectBtn->setEnabled(ui->viceAddress->text().length() > 0);
}

void MainWindow::on_viceAddress_textChanged(const QString &t)
{
    updateUiState();
}


void MainWindow::on_connectBtn_clicked()
{
//    ui->statusbar->showMessage("BOOYAKA!!!");
}

void MainWindow::connectClicked() {
    controller_->connect("127.0.0.1", 6502);
}

}
