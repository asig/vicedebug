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

#include <QGroupBox>
#include <QTableWidget>
#include <QLineEdit>

#include "controller.h"
#include "focuswatcher.h"

namespace vicedebug {

class RegsGroup : public QGroupBox {
    Q_OBJECT

public:
    RegsGroup(QString title, QWidget* parent);
    virtual ~RegsGroup() = default;

    virtual void clear() = 0;
    virtual void initFromRegs(const Registers& regs) = 0;
    virtual bool saveToRegs(Registers& regs) = 0;

    QLineEdit* createLineEdit(int len);

    const std::vector<std::unique_ptr<FocusWatcher>>& getFocusWatchers() {
        return focusWatchers_;
    }

protected:
    std::vector<std::unique_ptr<FocusWatcher>> focusWatchers_;
};

class Regs6502Group : public RegsGroup {
    Q_OBJECT

public:
    Regs6502Group(QString title, QWidget* parent);
    ~Regs6502Group() = default;

    virtual void clear() override;
    virtual void initFromRegs(const Registers& regs) override;
    virtual bool saveToRegs(Registers& regs) override;

private:
    QLineEdit* pc_;
    QLineEdit* sp_;
    QLineEdit* a_;
    QLineEdit* x_;
    QLineEdit* y_;
    QLineEdit* flags_;
};

class RegistersWidget : public QWidget {
    Q_OBJECT

public:
    RegistersWidget(Controller* controller, QWidget* parent);
    ~RegistersWidget();

private slots:
    void onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints);
    void onDisconnected();
    void onExecutionResumed();
    void onExecutionPaused(const MachineState& machineState);
    void onFocusLost();
    void onRegistersChanged(const Registers& registers);

private:
    void enableControls(bool enable);
    void clearControls();
    void fillControls();

    Controller* controller_;

    Registers regs_;
    std::vector<RegsGroup*> regsGroups_;
};

}
