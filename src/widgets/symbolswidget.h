/*
 * Copyright (c) 2023 Andreas Signer <asigner@gmail.com>
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

#include <unordered_map>

#include <QGroupBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolButton>

#include "controller.h"
#include "symtab.h"

namespace vicedebug {

class SymbolsWidget : public QGroupBox
{
    Q_OBJECT

    enum Type {
        READ,
        WRITE,
        EXEC
    };

public:
    SymbolsWidget(Controller* controller, SymTable* symtab, QWidget* parent);
    ~SymbolsWidget();

public slots:
    void onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints);
    void onDisconnected();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& machineState);
    void onSymTabChanged();

private slots:
    void onTreeItemSelectionChanged();
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onAddClicked();
    void onRemoveClicked();

private:
    void addItem(const std::string& symbol, std::uint16_t address);

    void enableControls(bool enable);
    void clearTree();
    void fillTree();

    Breakpoints breakpoints_;
    Controller* controller_;
    SymTable* symtab_;

    QTreeWidget* tree_;
    QToolButton* addBtn_;
    QToolButton* removeBtn_;
};

}
