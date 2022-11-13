#pragma once

#include <QPlainTextEdit>
#include <QScrollBar>

#include "controller.h"

namespace vicedebug {

class DisassemblyWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    DisassemblyWidget(Controller* controller, QWidget* parent);
    virtual ~DisassemblyWidget();

    void breakpointAreaPaintEvent(QPaintEvent *event);
    int breakpointAreaWidth();

protected:
//    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void onEventFromController(const Event& event);

private slots:
    void updateBreakpointAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateBreakpointArea(const QRect &rect, int dy);

private:
    Controller* controller_;

    QWidget* breakpointArea;
};

}
