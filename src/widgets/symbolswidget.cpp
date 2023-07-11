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

#include "widgets/symbolswidget.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QStringList>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QValidator>

#include "dialogs/symboldialog.h"

namespace vicedebug {

class HexValidator : public QValidator {
public:
    State validate(QString &str, int &pos) const {
            str = str.trimmed();
            if (str.length() == 0) {
                return QValidator::Intermediate;
            }
            int base = 16;
            if (str[0] == '+') {
                str = str.mid(1);
                base = 10;
            }
            bool ok;
            uint res = str.toUInt(&ok, base);
            if (!ok) {
                return QValidator::Intermediate;
            }
            if (res > 0xffff || res < 0) {
                return QValidator::Invalid;
            }
            return QValidator::Acceptable;
        }
    };

class NoEditDelegate: public QStyledItemDelegate {
    public:
      NoEditDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}
      virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        return 0;
      }
    };

class HexEditDelegate: public QStyledItemDelegate {
    public:
      HexEditDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}

      virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
          QLineEdit *lineEdit = new QLineEdit(parent);
          lineEdit->setValidator(new HexValidator());
          return lineEdit;
      }

      virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const {
          qDebug() << "row: " << index.row() << "; column: " << index.column();

      }

    };

SymbolsWidget::SymbolsWidget(Controller* controller, SymTable* symtab, QWidget* parent) :
    QGroupBox("Symbols", parent), controller_(controller), symtab_(symtab)
{
    QStringList l;

    tree_ = new QTreeWidget();
    tree_->setColumnCount(2);
    tree_->setHeaderLabels({ "Symbol","Address"} );
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &SymbolsWidget::onTreeItemSelectionChanged);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &SymbolsWidget::onTreeItemDoubleClicked);

    addBtn_ = new QToolButton();
    addBtn_->setIcon(QIcon(":/images/codicons/add.svg"));
    connect(addBtn_, &QToolButton::clicked, this, &SymbolsWidget::onAddClicked);

    removeBtn_ = new QToolButton();
    removeBtn_->setIcon(QIcon(":/images/codicons/remove.svg"));
    connect(removeBtn_, &QToolButton::clicked, this, &SymbolsWidget::onRemoveClicked);

    QVBoxLayout* vLayout = new QVBoxLayout();
    vLayout->addWidget(addBtn_);
    vLayout->addWidget(removeBtn_);
    vLayout->addStretch(10);

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(tree_);
    hLayout->addLayout(vLayout);

    tree_->setItemDelegateForColumn(0, new NoEditDelegate() );
    tree_->setItemDelegateForColumn(1, new HexEditDelegate() );

    setLayout(hLayout);

    connect(controller_, &Controller::connected, this, &SymbolsWidget::onConnected);
    connect(controller_, &Controller::disconnected, this, &SymbolsWidget::onDisconnected);
    connect(controller_, &Controller::executionPaused, this, &SymbolsWidget::onExecutionPaused);
    connect(controller_, &Controller::executionResumed, this, &SymbolsWidget::onExecutionResumed);

    connect(symtab_, &SymTable::symbolsChanged, this, [this](){ clearTree(); fillTree(); });

    enableControls(false);
}

SymbolsWidget::~SymbolsWidget() {
}

void SymbolsWidget::addItem(const std::string& symbol, std::uint16_t address) {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, symbol.c_str());
    item->setText(1, QString::asprintf("%04x", address));
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    tree_->insertTopLevelItem(tree_->topLevelItemCount(), item);
}

void SymbolsWidget::enableControls(bool enable) {
    this->setEnabled(enable);
    tree_->setEnabled(enable);
    addBtn_->setEnabled(enable);
    removeBtn_->setEnabled(enable && tree_->selectedItems().size() == 1);
}

void SymbolsWidget::clearTree() {
    tree_->clear();
}

void SymbolsWidget::fillTree() {
    for (const auto& [label, addr] : symtab_->elements()) {
        addItem(label, addr);
    }
}

void SymbolsWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    qDebug() << "SymbolsWidget::onConnected called";
    enableControls(true);
    fillTree();
}

void SymbolsWidget::onDisconnected() {
    qDebug() << "SymbolsWidget::onDisconnected called";
    enableControls(false);
    clearTree();
}

void SymbolsWidget::onExecutionResumed() {
    qDebug() << "SymbolsWidget::onExecutionResumed called";
    enableControls(false);
}

void SymbolsWidget::onExecutionPaused(const MachineState& machineState) {
    qDebug() << "SymbolsWidget::onExecutionPaused called";
    enableControls(true);
}

void SymbolsWidget::onSymTabChanged() {
    clearTree();
    fillTree();
}

void SymbolsWidget::onTreeItemSelectionChanged() {
    auto selectedItems = tree_->selectedItems();
    removeBtn_->setEnabled(selectedItems.size() == 1);
}

void SymbolsWidget::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column) {
    QString label = item->text(0);
    std::uint16_t address = item->text(1).toUInt(nullptr, 16);
    SymbolDialog dlg(label.toStdString(), address, this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        symtab_->remove(dlg.label());
        symtab_->set(dlg.label(), dlg.address());
    }
    qDebug() << "Leaving onTreeItemDoubleClicked()";
}

void SymbolsWidget::onAddClicked() {
    SymbolDialog dlg(this);
    int res = dlg.exec();
    if (res == QDialog::DialogCode::Accepted) {
        symtab_->set(dlg.label(), dlg.address());
    }
}

void SymbolsWidget::onRemoveClicked() {
    auto selected = tree_->selectedItems();
    if (selected.size() != 1) {
        qDebug() << "SymbolsWidget: 'Remove' button clicked, but selection size is " << selected.size() << "... WTF?";
        return;
    }
    symtab_->remove(selected[0]->text(0).toStdString());
}

}
