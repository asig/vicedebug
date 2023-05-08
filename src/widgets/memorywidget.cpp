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

#include "widgets/memorywidget.h"

#include "resources.h"
#include "tooltipgenerators.h"
#include "petscii.h"

#include <QEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>
#include <QLineEdit>
#include <QScrollBar>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolTip>
#include <QMenu>
#include <QGroupBox>
#include <QPushButton>

#include <iostream>

namespace vicedebug {

namespace {

const int kBytesPerLine = 16;
const QString kSeparator("  ");

const QColor kBg = QColor(Qt::white);
const QColor kBgSelected = QColor(Qt::red);
const QColor kBgDisabled = QColor(Qt::lightGray).lighter(120);

const QColor kFg = QColor(Qt::black);
const QColor kFgSelected = QColor(Qt::yellow);
const QColor kFgDisabled = QColor(Qt::lightGray).lighter(80);

const QColor kFindResultFg = QColor(Qt::white);
const QColor kFindResultBg = QColor(50,50,255);

// Find
const QColor kFindFound = Qt::black;
const QColor kFindNotFound = QColor(255,50,50);


Watches filterByBank(const Watches& watches, int bank) {
    Watches res;
    for (const auto& w : watches) {
        if (w.bankId == bank) {
            res.push_back(w);
        }
    }
    return res;
}

bool isHexChar(QChar c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f' ) || ('A' <= c && c <= 'F');
}

}

// ----------------------
// FindGroup
// ----------------------

FindGroup::FindGroup(QWidget* parent)
    : QGroupBox(parent) {

    textEdit_ = new QLineEdit();
    textEdit_->setFont(Resources::robotoMonoFont());
    connect(textEdit_, &QLineEdit::textEdited, [this](const QString& s) {
        bool valid = isValidSearch(s);
        if (valid) {
            isFirstFind_ = true;
            find(1);
        } else {
            emit markResult({.found = false});
        }
    });

    findPrevBtn_ = new QPushButton("Find previous");
    findPrevBtn_->setEnabled(false);
    connect(findPrevBtn_, &QPushButton::clicked, [this] { find(-1); });

    findNextBtn_ = new QPushButton("Find next");
    findNextBtn_->setEnabled(false);
    connect(findNextBtn_, &QPushButton::clicked, [this] { find(1); });

    closeBtn_ = new QToolButton();
    closeBtn_->setIcon(QIcon(":/images/codicons/close.svg"));
    connect(closeBtn_, &QToolButton::clicked, this, &FindGroup::stop);

    findLabel_ = new QLabel("Find:");
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(findLabel_);
    hLayout->addWidget(textEdit_);
    hLayout->addWidget(findPrevBtn_);
    hLayout->addWidget(findNextBtn_);
    hLayout->addStretch();
    hLayout->addWidget(closeBtn_);

    setLayout(hLayout);
    setVisible(false);
}

FindGroup::~FindGroup() {
}

void FindGroup::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        stop();
    }
}

void FindGroup::start(FindFunc findFunc, bool hexMode) {
    findFunc_ = findFunc;
    isHexMode_ = hexMode;
    findLabel_->setText(hexMode ? "Find (hex):" : "Find:");
    isFirstFind_ = true;
    lastFindPos_ = 0;
    textEdit_->clear();
    setVisible(true);
    textEdit_->setFocus();
    updateUI(false);
}

void FindGroup::stop() {
    setVisible(false);
    emit findFinished();
}

void FindGroup::updateUI(bool found) {
    QColor col = found ? kFindFound : kFindNotFound;
    QPalette palette;
    palette.setColor(QPalette::Text,col);
    textEdit_->setPalette(palette);
    findNextBtn_->setEnabled(found);
    findPrevBtn_->setEnabled(found);
}

void FindGroup::find(int dir) {
    if (isFirstFind_) {
        lastFindPos_ += -dir;
        isFirstFind_ = false;
    }
    auto searchData = convertToSearchData(textEdit_->text());
    FindResult res = findFunc_(searchData, lastFindPos_, dir);
    if (res.found) {
        lastFindPos_ = res.resultPos;
    } else {
        lastFindPos_ = 0;
    }
    updateUI(res.found);
    emit markResult(res);
}

bool FindGroup::isValidSearch(const QString& s) {
    if (isHexMode_) {
        int hexLen = 0;
        for (int i = 0; i < s.length(); i++) {
            QChar c = s[i];
            if (c.isSpace()) {
                continue;
            } else if (!isHexChar(c)) {
                return false;
            }
            hexLen++;
        }
        return (hexLen%2 == 0);
    } else {
        return s.length() > 0;
    }

}

std::vector<std::uint8_t> FindGroup::convertToSearchData(const QString& s) {
    std::vector<std::uint8_t> res;
    if (isHexMode_) {
        QString filtered;
        // Filter out all non-hex chars
        for (int i = 0; i < s.length(); i++) {
            QChar c = s[i];
            if (isHexChar(c)) {
                filtered += c;
            }
        }

        // Translate to byte array
        for (int i = 0; i < filtered.length()/2; i++) {
            res.push_back(filtered.mid(2*i,2).toUInt(nullptr,16));
        }
    } else {
        for (int i = 0; i < s.length(); i++) {
            // TODO convert to PETSCII?
            res.push_back(s[i].cell());
        }
    }
    return res;
}


// ----------------------
// MemoryWidget
// ----------------------

MemoryWidget::MemoryWidget(Controller* controller, QWidget* parent) :
    controller_(controller), QWidget(parent)
{
    // Set up memory content
    scrollArea_ = new QScrollArea(this);
    content_ = new MemoryContent(controller_, scrollArea_);
    connect(content_, &MemoryContent::memoryChanged, this, [this](std::uint16_t addr, std::uint8_t val) {
        controller_->writeMemory(selectedBank_.id, addr, val);
    });
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setWidget(content_);

    // Set up "toolbar"
    bankCombo_ = new QComboBox();
    connect(bankCombo_, &QComboBox::currentIndexChanged, [this](int index) {
        if (index < 0) {
            // No selection
            content_->setMemory({{0,{}}}, Bank{0}, {}, {});
            return;
        }
        selectedBank_ = banks_[index];
        if (memory_.find(selectedBank_.id) != memory_.end()) {
            content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
        }
    });
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Bank:"));
    toolbar->addWidget(bankCombo_);
    toolbar->addStretch();

    // Set up find group
    findGroup_ = new FindGroup(this);
    connect(findGroup_, &FindGroup::findFinished, [this]{
        content_->markSearchResult({.found=false});
    });
    connect(findGroup_, &FindGroup::markResult, [this](const FindResult& r) {
        content_->markSearchResult(r);
    });

    // Set up final layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(toolbar);
    layout->addWidget(scrollArea_);
    layout->addWidget(findGroup_);

    setLayout(layout);

    connect(controller, &Controller::connected, this, &MemoryWidget::onConnected);
    connect(controller, &Controller::disconnected, this, &MemoryWidget::onDisconnected);
    connect(controller, &Controller::executionPaused, this, &MemoryWidget::onExecutionPaused);
    connect(controller, &Controller::executionResumed, this, &MemoryWidget::onExecutionResumed);
    connect(controller, &Controller::memoryChanged, this, &MemoryWidget::onMemoryChanged);
    connect(controller, &Controller::breakpointsChanged, this, &MemoryWidget::onBreakpointsChanged);
    connect(controller, &Controller::watchesChanged, this, &MemoryWidget::onWatchesChanged);

    setEnabled(false);
}

MemoryWidget::~MemoryWidget() {
}

void MemoryWidget::onFindText() {
    content_->markSearchResult({.found=false});
    findGroup_->start([this] (const std::vector<std::uint8_t>& data, std::uint16_t pos, std::int8_t direction) {
        return content_->find(data, pos, direction);
    }, /*hexMode=*/false);
}

void MemoryWidget::onFindHex() {
    content_->markSearchResult({.found=false});
    findGroup_->start([this] (const std::vector<std::uint8_t>& data, std::uint16_t pos, std::int8_t direction) {
        return content_->find(data, pos, direction);
    }, /*hexMode=*/true);
}

void MemoryWidget::onConnected(const MachineState& machineState, const Banks& banks, const Breakpoints& breakpoints) {
    // Clear bankCombo before we overwrite banks_, because the current selection of bankCombo will change while it is cleared.
    bankCombo_->clear();
    // banks_ needs to be set before we insert items again, because again, the current selection of bankCombo will change when adding items
    banks_ = banks;
    for (Bank b : banks) {
        bankCombo_->addItem(b.name.c_str(), QVariant(b.id));
    }
    selectedBank_ = banks_[0];
    bankCombo_->setCurrentIndex(0);
    memory_ = machineState.memory;
    breakpoints_ = breakpoints;
    content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
    setEnabled(true);
}

void MemoryWidget::onDisconnected() {
    memory_.clear();
    bankCombo_->clear();
    breakpoints_.clear();
    content_->setMemory({{0,{}}}, Bank{0}, {}, {});
    setEnabled(false);
}

void MemoryWidget::onExecutionResumed() {
    // enableControls() seems to change the scroll position when the user was editing contents.
    // As a workaround, remember and restore scroll positions.
    int vert = scrollArea_->verticalScrollBar()->value();
    int hor = scrollArea_->horizontalScrollBar()->value();

    setEnabled(false);

    scrollArea_->verticalScrollBar()->setValue(vert);
    scrollArea_->horizontalScrollBar()->setValue(hor);
}

void MemoryWidget::onExecutionPaused(const MachineState& machineState) {
    memory_ = machineState.memory;
    content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
    setEnabled(true);
    update();
}

void MemoryWidget::onMemoryChanged(std::uint16_t bankId, std::uint16_t addr, std::vector<std::uint8_t> data) {
    std::vector<std::uint8_t>& mem = memory_.at(bankId);
    for (auto b : data) {
        mem[addr++] = b;
    }
    content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
}

void MemoryWidget::onBreakpointsChanged(const Breakpoints& breakpoints) {
    breakpoints_ = breakpoints;
    content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
}

void MemoryWidget::onWatchesChanged(const Watches& watches) {
    watches_ = watches;
    content_->setMemory(memory_, selectedBank_, breakpoints_, watches_); // So far, breakpoints are only supported for default bank...
}

// ----------------------
// MemoryContent
// ----------------------

MemoryContent::MemoryContent(Controller* controller, QScrollArea* parent) :
    QWidget(parent), controller_(controller), scrollArea_(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    // Compute the size of the widget:
    QFontMetrics robotofm(Resources::robotoMonoFont());
    ascent_ = robotofm.ascent();
    borderW_ = robotofm.horizontalAdvance(" ");
    addressW_ = robotofm.horizontalAdvance("00000");
    separatorW_ = robotofm.horizontalAdvance(kSeparator);
    hexSpaceW_ = robotofm.horizontalAdvance("00 ");
    hexCharW_ = robotofm.horizontalAdvance("0");

    QFontMetrics c64fm(Resources::c64Font());
    charW_ = c64fm.horizontalAdvance("0");

    lineH_ = c64fm.height() > robotofm.height() ? c64fm.height() : robotofm.height();

    editActive_ = false;
    petsciiBase_ = PETSCII::kUCBase;

    updateSize(0);
}

MemoryContent::~MemoryContent() {
}

void MemoryContent::setMemory(const std::unordered_map<std::uint16_t, std::vector<std::uint8_t>>& memory, const Bank bank, const Breakpoints& breakpoints, const Watches& watches) {
    bank_ = bank;
    memory_ = memory.at(bank_.id);
    breakpoints_ = bank_.id == 0 ? breakpoints : Breakpoints();
    watches_ = filterByBank(watches, bank_.id);
    breakpointTypes_.resize(memory_.size());
    breakpoint_.resize(memory_.size());
    watch_.resize(memory_.size());
    for (int i = 0; i < memory_.size(); i++) {
        breakpointTypes_[i] = 0;
        breakpoint_[i] = nullptr;
        watch_[i] = nullptr;
    }
    for (const auto& bp : breakpoints_) {
        std::uint8_t op = bp.op & (Breakpoint::Type::READ | Breakpoint::Type::WRITE);
        if (op == 0) {
            continue;
        }
        for (int addr = bp.addrStart; addr <= bp.addrEnd; addr++) {
            breakpointTypes_[addr] = op;
            breakpoint_[addr] = &bp;
        }
    }
    for (const auto& w : watches_) {
        for (std::uint16_t addr = w.addrStart; addr < w.addrStart + w.len; addr++) {
            watch_[addr] = &w;
        }
    }
    updateSize(memory_.size() / kBytesPerLine);
    markSearchResult({.found=false});
    update();
}

bool MemoryContent::event(QEvent* event) {
    if (event->type() != QEvent::ToolTip) {
        return QWidget::event(event);
    }
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
    QPoint p = helpEvent->pos();
    std::uint16_t addr;
    bool dontCareBool;
    int dontCareInt;
    bool hideToolTip = true;
    if (addrAtPos(p, addr, dontCareBool, dontCareInt)) {
        QString tooltip;
        const Breakpoint* bp = breakpoint_[addr];
        if (bp != nullptr) {
            hideToolTip = false;
            tooltip = BreakpointTooltipGenerator(*bp).generate();
        }
        const Watch* w = watch_[addr];
        if (w != nullptr) {
            hideToolTip = false;
            if (tooltip.length() > 0) {
                tooltip += "<hr>";
            }
            tooltip += WatchTooltipGenerator(*w, {bank_}).generate();
        }
        QToolTip::showText(helpEvent->globalPos(), tooltip);
    }
    if (hideToolTip) {
        QToolTip::hideText();
        event->ignore();
    }
    return true;
}

void MemoryContent::contextMenuEvent(QContextMenuEvent* event) {
    std::uint16_t addr;
    bool dontCareBool;
    int dontCareInt;
    if (!addrAtPos(event->pos(), addr, dontCareBool, dontCareInt)) {
        return;
    }

    QMenu menu(this);
    const Breakpoint* bp = breakpoint_[addr];
    if (bp) {
        connect(menu.addAction("Delete breakpoint"), &QAction::triggered, [bp, this] {
            controller_->deleteBreakpoint(bp->number);
        });
        QString label = QString(bp->enabled ? "Disable" : "Enable") + " breakpoint";
        connect(menu.addAction(label), &QAction::triggered, [bp, this] {
            controller_->enableBreakpoint(bp->number, !bp->enabled);
        });
    } else if (bank_.id == 0) { // So far, breakpoints are only available in "main" bank...
        QMenu* submenu = menu.addMenu("Add breakpoint...");
        connect(submenu->addAction("Break on load"), &QAction::triggered, [addr, this] {
            controller_->createBreakpoint(Breakpoint::Type::READ, addr, addr, true);
        });
        connect(submenu->addAction("Break on store"), &QAction::triggered, [addr, this] {
            controller_->createBreakpoint(Breakpoint::Type::WRITE, addr, addr, true);
        });
        connect(submenu->addAction("Break on both"), &QAction::triggered, [addr, this] {
            controller_->createBreakpoint(Breakpoint::Type::READ | Breakpoint::Type::WRITE, addr, addr, true);
        });
    }
    QMenu* submenu = menu.addMenu("Add watch...");
    connect(submenu->addAction("int8"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::INT, bank_.id, addr, 1);
    });
    connect(submenu->addAction("uint8"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::UINT, bank_.id, addr, 1);
    });
    connect(submenu->addAction("uint8 (hex)"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::UINT_HEX, bank_.id, addr, 1);
    });
    connect(submenu->addAction("int16"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::INT, bank_.id, addr, 2);
    });
    connect(submenu->addAction("uint16"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::UINT, bank_.id, addr, 2);
    });
    connect(submenu->addAction("uint16 (hex)"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::UINT_HEX, bank_.id, addr, 2);
    });
    connect(submenu->addAction("float"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::FLOAT, bank_.id, addr, 5);
    });
    connect(submenu->addAction("String"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::CHARS, bank_.id, addr, 16); // Should we instead of a fixed length just show the dialog?
    });
    connect(submenu->addAction("Bytes"), &QAction::triggered, [addr, this] {
        controller_->createWatch(Watch::ViewType::BYTES, bank_.id, addr, 16); // Should we instead of a fixed length just show the dialog?
    });
    menu.exec(event->globalPos());
    event->ignore();
}

void MemoryContent::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    int firstLine = event->rect().top()/lineH_;
    int lastLine = event->rect().bottom()/lineH_;

    auto fg = isEnabled() ? kFg : kFgDisabled;
    auto bg = isEnabled() ? kBg : kBgDisabled;
    auto fgSelected = isEnabled() ? kFgSelected : kFgDisabled;
    auto bgSelected = isEnabled() ? kBgSelected : kBgDisabled;

    QColor bpEnabledBgCol = QColor(255,0,0,150);
    QColor bpDisabledBgCol = QColor(255,0,0,100);
    QColor watchBgCol = QColor(0,0,255,100);

    painter.setPen(fg);
    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(QBrush(bg));
    painter.fillRect(event->rect(), bg);

    int memSize = memory_.size() > 0 ? memory_.size() : 0;
    QString sep("  ");
    const char* addrFormatString = memSize <= 0x10000 ? " %04X" : "%05X";
    for (int pos = firstLine * kBytesPerLine, y = firstLine*lineH_+ascent_; pos < (lastLine + 1) * kBytesPerLine; pos += kBytesPerLine, y += lineH_) {
        if (pos >= memSize) {
            continue;
        }

        QString addr = QString::asprintf(addrFormatString, pos);
        painter.setFont(Resources::robotoMonoFont());
        painter.drawText(borderW_, y, addr);
        for (int i = 0; i < kBytesPerLine; i++) {
            QString hex;
            QString text;
            int hexX = borderW_ + addressW_ + separatorW_ + i*hexSpaceW_;
            int textX = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + i * charW_;
            const Breakpoint* bp = breakpoint_[pos + i];
            const Watch* w = watch_[pos+i];
            bool inSearchResult = pos + i >= resultStart_ && pos + i < resultStart_+resultLen_;
            if (pos + i < memSize) {
                std::uint8_t c = memory_[pos+i];
                hex = QString::asprintf("%02X ", c);
                text = PETSCII::isPrintable(c) ? QString(QChar(petsciiBase_ + PETSCII::toScreenCode(c))) : ".";
            } else {
                hex = "   ";
                text = " ";
            }
            if (inSearchResult) {
                painter.setBrush(kFindResultBg);
                painter.setPen(Qt::NoPen);
                painter.drawRect(hexX, y - ascent_, hexSpaceW_ - hexCharW_, lineH_);
                painter.drawRect(textX, y - ascent_, charW_, lineH_);
                painter.setPen(kFindResultFg);
                painter.setBackgroundMode(Qt::TransparentMode);
                painter.setBrush(Qt::NoBrush);
            } else if (bp != nullptr || w != nullptr) {
                QColor bpBgCol = (bp != nullptr && bp->enabled) ? bpEnabledBgCol : bpDisabledBgCol;
                if (w == nullptr) { // Only breakpoint
                    painter.setBrush(bpBgCol);
                } else if (bp == nullptr) { // Only Watch
                    painter.setBrush(watchBgCol);
                } else { // both
                    QLinearGradient grad(QPointF(0, y - ascent_), QPointF(0, y - ascent_ + lineH_));
                    grad.setColorAt(0, bpBgCol);
                    grad.setColorAt(1, watchBgCol);
                    painter.setBrush(grad);
                }

                painter.setPen(Qt::NoPen);
                painter.drawRect(hexX, y - ascent_, hexSpaceW_ - hexCharW_, lineH_);
                painter.drawRect(textX, y - ascent_, charW_, lineH_);
                painter.setPen(kBg);
                painter.setBackgroundMode(Qt::TransparentMode);
                painter.setBrush(Qt::NoBrush);
            }
            painter.setFont(Resources::robotoMonoFont());
            painter.drawText(hexX, y, hex);
            painter.setFont(Resources::c64Font());
            painter.drawText(textX , y, text);
            if (bp || w || inSearchResult) {
                painter.setBackgroundMode(Qt::OpaqueMode);
                painter.setPen(fg);
            }
        }

        if (editActive_) {
            painter.setPen(fgSelected);
            painter.setBackground(QBrush(bgSelected));
            QString text;
            if (nibbleMode_) {
                if (pos <= cursorPos_/2 && cursorPos_/2 < pos+kBytesPerLine) {
                    // Highlight nibble
                    int x = borderW_ + addressW_ + separatorW_ + ((cursorPos_/2) % kBytesPerLine) * hexSpaceW_ + (cursorPos_ % 2) * hexCharW_;
                    text = QString::asprintf("%02X",memory_[cursorPos_/2]).mid((cursorPos_ % 2),1);
                    painter.setFont(Resources::robotoMonoFont());
                    painter.drawText(x, y, text);

                    // highlight character
                    x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (cursorPos_/2 % kBytesPerLine) * charW_;
                    painter.setPen(kBgSelected);
                    painter.drawRect(x, y-ascent_, charW_, lineH_);
                }
            } else {
                if (pos <= cursorPos_ && cursorPos_ < pos+kBytesPerLine) {
                    // Highlight character
                    char c = (char)memory_[cursorPos_];
                    text = PETSCII::isPrintable(c) ? QString(QChar(petsciiBase_ + PETSCII::toScreenCode(c))) : ".";
                    painter.setFont(Resources::c64Font());
                    painter.drawText(borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (cursorPos_ % kBytesPerLine) * charW_ , y, text);

                    // highlight nibbles
                    int x = borderW_ + addressW_ + separatorW_ + (cursorPos_ % kBytesPerLine)*hexSpaceW_ ;
                    painter.setPen(kBgSelected);
                    painter.drawRect(x, y-ascent_, 2*hexCharW_, lineH_);

                }
            }
            painter.setPen(fg);
            painter.setBackground(QBrush(bg));
        }
    }

    // Draw separators
    int h = lineH_ * ((memSize + kBytesPerLine - 1)/kBytesPerLine);
    int x = borderW_ + addressW_ + separatorW_/2;
    painter.drawLine(x, 0, x, h);
    x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_/2;
    painter.drawLine(x, 0,x, h);
}

void MemoryContent::mousePressEvent(QMouseEvent* event) {
    event->accept();
    QPoint pos = event->pos();

    std::uint16_t addr;
    int nibbleOfs;
    if (addrAtPos(pos, addr, nibbleMode_, nibbleOfs)) {
        editActive_ = true;
        if (nibbleMode_) {
            cursorPos_ = 2*addr + nibbleOfs;
        } else {
            cursorPos_ = addr;
        }
        update();
        return;
    }
}

bool MemoryContent::addrAtPos(QPoint pos, std::uint16_t& addr, bool& nibbleMode, int& nibbleOfs) {
    int x = pos.x();
    int y = pos.y();
    int line = y/lineH_;

    // Are we in the hex part?
    int leftEdge = borderW_ + addressW_ + separatorW_;
    int rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - hexCharW_;
    if (x >= leftEdge && x < rightEdge) {
        // Maybe we're in nibble mode!
        x -= leftEdge;
        int byteOfs = x / hexSpaceW_;
        x = x % hexSpaceW_;
        nibbleOfs = x / hexCharW_;
        // Are we on a separator space?
        if (nibbleOfs == 2) {
            // Yes
            return false;
        }

        addr = line * kBytesPerLine + byteOfs;
        if (addr >= memory_.size()) {
            // Out of bounds
            return false;
        }
        nibbleMode = true;
        return true;
    }

    // Are we in the text part?
    leftEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_;
    rightEdge = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + kBytesPerLine * charW_;
    if (x >= leftEdge && x < rightEdge) {
        x -= leftEdge;
        x /= charW_; // x is now the "character pos"
        addr = line * kBytesPerLine + x;
        if (addr >= memory_.size()) {
            // Out of bounds
            return false;
        }
        nibbleMode = false;
        return true;
    }
    return false;
}

void MemoryContent::focusOutEvent(QFocusEvent* event) {
    editActive_ = false;
    update();
}

int charToHex(char c) {
    if (isdigit(c)) {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return 10+ c-'a';
    } else if ('A' <= c && c <= 'F') {
        return 10+ c-'A';
    }
    return -1;
}

void MemoryContent::moveCursorRight() {
    int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
    if (cursorPos_ < maxPos) {
        cursorPos_++;
    }
    update();
    ensureCursorVisible();
}

void MemoryContent::ensurePosVisible(std::uint16_t pos) {
    int y = pos / kBytesPerLine * lineH_;
    int x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (pos % kBytesPerLine) * charW_;
    scrollArea_->ensureVisible(x, y, 0, 50);
}

void MemoryContent::ensureCursorVisible() {
    int x, y;
    if (nibbleMode_) {
        y = (cursorPos_/2) / kBytesPerLine * lineH_;
        // TODO(asigner): Extract those calculations in functions!
        x = borderW_ + addressW_ + separatorW_ + ((cursorPos_/2) % kBytesPerLine) * hexSpaceW_ + (cursorPos_ % 2) * hexCharW_; // T
    } else {
        y = cursorPos_ / kBytesPerLine * lineH_;
        x = borderW_ + addressW_ + separatorW_ + kBytesPerLine*hexSpaceW_ - hexCharW_ + separatorW_ + (cursorPos_ % kBytesPerLine) * charW_;
    }
    scrollArea_->ensureVisible(x, y, 0, 50);
}

void MemoryContent::keyPressEvent(QKeyEvent* event) {
    if (!editActive_) {
        event->ignore();
        return;
    }
    event->accept();

    // Deal with cursor movement
    switch(event->key()) {
    case Qt::Key_Left:
        if (cursorPos_ > 0) {
            cursorPos_--;
        }
        update();
        ensureCursorVisible();
        break;
    case Qt::Key_Right: {
        moveCursorRight();
        update();
        break;
    }
    case Qt::Key_Up: {
        int delta = kBytesPerLine * (nibbleMode_ ? 2 : 1);
        if (cursorPos_ - delta >= 0) {
            cursorPos_ -= delta;
        }
        update();
        ensureCursorVisible();
        break;
    }
    case Qt::Key_Down: {
        int maxPos = memory_.size() * (nibbleMode_ ? 2 : 1);
        int delta = kBytesPerLine * (nibbleMode_ ? 2 : 1);
        if (cursorPos_ + delta < maxPos) {
            cursorPos_ += delta;
        }
        update();
        ensureCursorVisible();
        break;
    }
    }

    if (nibbleMode_) {
        QString text = event->text().toLower();
        if (text.length() == 1) {
            int v = charToHex(text.toStdString()[0]);
            if (v == -1) { return; }

            std::uint8_t mask = 0xf;
            std::uint8_t shift = cursorPos_%2 == 0 ? 4 : 0;

            std::uint8_t oldVal = memory_[cursorPos_/2];
            std::uint8_t newVal = oldVal & ~(mask<<shift) | (v<<shift);
            int addr = cursorPos_/2;
            // memory and view will be updated in onMemoryChanged();
            emit memoryChanged(addr, newVal);
            moveCursorRight();
        }
    } else {
        QString text = event->text();
        if (text.length() == 1) {
            std::uint8_t v = text.toStdString()[0];
            int addr = cursorPos_;
            // memory and view will be updated in onMemoryChanged();
            emit memoryChanged(addr, v);
            moveCursorRight();
        }
    }

}

void MemoryContent::updateSize(int lines) {
    int w = borderW_ + addressW_ + separatorW_ + kBytesPerLine * hexSpaceW_ - hexCharW_ + separatorW_ + kBytesPerLine * charW_ + borderW_;
    int h = lines * lineH_;
    setMinimumSize(w, h);
}

FindResult MemoryContent::find(const std::vector<std::uint8_t>& data, std::uint16_t pos, std::int8_t direction) {
    std::uint16_t startPos = pos;
    do {
        pos += direction;
        int i;
        for (i = 0; i < data.size() && (data[i] == memory_[(pos + i) & 0xffff]); i++)
            ;
        if (i == data.size()) {
            // Woohoo, we found the string!
            return {.found = true, .resultPos = pos, .resultLen = (int)data.size()};
        }
    } while (pos != startPos);
    return {.found = false};
}

void MemoryContent::markSearchResult(const FindResult& res) {
    if (res.found) {
        resultStart_ = res.resultPos;
        resultLen_ = res.resultLen;
        ensurePosVisible(resultStart_);
    } else {
        resultStart_ = 0;
        resultLen_ = 0;
    }
    update();
}

}
