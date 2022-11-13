#include "widgets/disassemblywidget.h"

#include "fonts.h"

#include <QEvent>
#include <QTextBlock>
#include <QPainter>
#include <QFontDatabase>

#include <iostream>

namespace vicedebug {

class BreakpointArea : public QWidget
{
public:
    BreakpointArea(DisassemblyWidget* disassembly) : QWidget(disassembly), disassembly_(disassembly)
    {}

    QSize sizeHint() const override
    {
        return QSize(disassembly_->breakpointAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        disassembly_->breakpointAreaPaintEvent(event);
    }

private:
    DisassemblyWidget* disassembly_;
};


DisassemblyWidget::DisassemblyWidget(Controller* controller, QWidget* parent) :
    QPlainTextEdit(parent), controller_(controller)
{
    setFont(Fonts::robotoMono());
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);

    breakpointArea = new BreakpointArea(this);

    connect(this, &DisassemblyWidget::blockCountChanged, this, &DisassemblyWidget::updateBreakpointAreaWidth);
    connect(this, &DisassemblyWidget::updateRequest, this, &DisassemblyWidget::updateBreakpointArea);
    connect(this, &DisassemblyWidget::cursorPositionChanged, this, &DisassemblyWidget::highlightCurrentLine);

    connect(controller_, &Controller::publishEvent, this, &DisassemblyWidget::onEventFromController);

    updateBreakpointAreaWidth(0);
    highlightCurrentLine();

    setPlainText(
                ".C:c262  F0 FA       BEQ $C25E\n"
                ".C:c264  20 9F CD    JSR $CD9F\n"
                ".C:c267  20 34 C2    JSR $C234\n"
                ".C:c26a  C9 0D       CMP #$0D\n"
                ".C:c26c  D0 EA       BNE $C258\n"
                ".C:c26e  85 D6       STA $D6\n"
                ".C:c270  A9 00       LDA #$00\n"
                ".C:c272  85 F4       STA $F4\n"
                ".C:c274  20 C3 CB    JSR $CBC3\n"
                ".C:c277  8E 30 0A    STX $0A30\n"
                ".C:c27a  20 B5 CB    JSR $CBB5\n"
                ".C:c27d  A4 E6       LDY $E6\n"
                ".C:c27f  A5 E8       LDA $E8\n"
                ".C:c281  30 13       BMI $C296\n"
                ".C:c283  C5 EB       CMP $EB\n"
                ".C:c285  90 0F       BCC $C296\n"
                ".C:c287  A4 E9       LDY $E9\n"
                ".C:c289  CD 30 0A    CMP $0A30\n"
                ".C:c28c  D0 04       BNE $C292\n"
                ".C:c28e  C4 EA       CPY $EA\n"
                ".C:c290  F0 02       BEQ $C294\n"
                ".C:c292  B0 11       BCS $C2A5\n"
                ".C:c294  85 EB       STA $EB\n"
                ".C:c296  84 EC       STY $EC\n"
                ".C:c298  4C BC C2    JMP $C2BC\n"
                ".C:c29b  98          TYA\n"
                ".C:c29c  48          PHA\n"
                ".C:c29d  8A          TXA\n"
                ".C:c29e  48          PHA\n"
                ".C:c29f  A5 D6       LDA $D6\n"
                ".C:c2a1  F0 B8       BEQ $C25B\n"
                ".C:c2a3  10 17       BPL $C2BC\n"
                ".C:c2a5  A9 00       LDA #$00\n"
                ".C:c2a7  85 D6       STA $D6\n"
                ".C:c2a9  A9 0D       LDA #$0D\n"
                ".C:c2ab  A2 03       LDX #$03\n"
                ".C:c2ad  E4 99       CPX $99\n"
                ".C:c2af  F0 04       BEQ $C2B5\n"
                ".C:c2b1  E4 9A       CPX $9A\n"
                ".C:c2b3  F0 03       BEQ $C2B8\n"
                ".C:c2b5  20 2D C7    JSR $C72D\n"
                ".C:c2b8  A9 0D       LDA #$0D\n"
                ".C:c2ba  D0 39       BNE $C2F5\n"
                ".C:c2bc  20 5C C1    JSR $C15C\n"
                ".C:c2bf  20 58 CB    JSR $CB58\n"
                ".C:c2c2  85 EF       STA $EF\n"
                ".C:c2c4  29 3F       AND #$3F\n"
                ".C:c2c6  06 EF       ASL $EF\n"
                ".C:c2c8  24 EF       BIT $EF\n"
                ".C:c2ca  10 02       BPL $C2CE\n"
                ".C:c2cc  09 80       ORA #$80\n"
                "(C:$c2ce) d\n"
                ".C:c2ce  90 04       BCC $C2D4\n"
                ".C:c2d0  A6 F4       LDX $F4\n"
                ".C:c2d2  D0 04       BNE $C2D8\n"
                ".C:c2d4  70 02       BVS $C2D8\n"
                ".C:c2d6  09 40       ORA #$40\n"
                ".C:c2d8  20 FF C2    JSR $C2FF\n"
                ".C:c2db  A4 EB       LDY $EB\n"
                ".C:c2dd  CC 30 0A    CPY $0A30\n"
                ".C:c2e0  90 0A       BCC $C2EC\n"
                ".C:c2e2  A4 EC       LDY $EC\n"
                ".C:c2e4  C4 EA       CPY $EA\n"
                ".C:c2e6  90 04       BCC $C2EC\n"
                ".C:c2e8  66 D6       ROR $D6\n"
                ".C:c2ea  30 03       BMI $C2EF\n"
                ".C:c2ec  20 ED CB    JSR $CBED\n"
                ".C:c2ef  C9 DE       CMP #$DE\n"
                ".C:c2f1  D0 02       BNE $C2F5\n"
                ".C:c2f3  A9 FF       LDA #$FF\n"
                ".C:c2f5  85 EF       STA $EF\n"
                ".C:c2f7  68          PLA\n"
                ".C:c2f8  AA          TAX\n"
                ".C:c2f9  68          PLA\n"
                ".C:c2fa  A8          TAY\n"
                ".C:c2fb  A5 EF       LDA $EF\n"
                ".C:c2fd  18          CLC\n"
                ".C:c2fe  60          RTS\n"
                ".C:c2ff  C9 22       CMP #$22\n"
                ".C:c301  D0 08       BNE $C30B\n"
                ".C:c303  A5 F4       LDA $F4\n"
                ".C:c305  49 01       EOR #$01\n"
                ".C:c307  85 F4       STA $F4\n"
                ".C:c309  A9 22       LDA #$22\n"
                ".C:c30b  60          RTS\n"
                ".C:c30c  A5 EF       LDA $EF\n"
                ".C:c30e  85 F0       STA $F0\n"
                ".C:c310  20 57 CD    JSR $CD57\n"
                ".C:c313  A5 F5       LDA $F5\n"
                ".C:c315  F0 02       BEQ $C319\n"
                ".C:c317  46 F4       LSR $F4\n"
                ".C:c319  68          PLA\n"
                ".C:c31a  A8          TAY\n"
                ".C:c31b  68          PLA\n"
                ".C:c31c  AA          TAX\n"
                ".C:c31d  68          PLA\n"
                ".C:c31e  18          CLC\n"
                ".C:c31f  60          RTS\n"
                ".C:c320  09 40       ORA #$40\n"
                ".C:c322  A6 F3       LDX $F3\n"
                ".C:c324  F0 02       BEQ $C328\n"
                ".C:c326  09 80       ORA #$80\n"
                ".C:c328  A6 F5       LDX $F5\n"
                "(C:$c32a) d\n"
                ".C:c32a  F0 02       BEQ $C32E\n"
                ".C:c32c  C6 F5       DEC $F5\n"
                ".C:c32e  24 F6       BIT $F6\n"
                ".C:c330  10 09       BPL $C33B\n"
                ".C:c332  48          PHA\n"
                ".C:c333  20 E3 C8    JSR $C8E3\n"
                ".C:c336  A2 00       LDX #$00\n"
                ".C:c338  86 F5       STX $F5\n"
                ".C:c33a  68          PLA\n"
                ".C:c33b  20 2F CC    JSR $CC2F\n"
                ".C:c33e  C4 E7       CPY $E7\n"
                ".C:c340  90 0A       BCC $C34C\n"
                ".C:c342  A6 EB       LDX $EB\n"
                ".C:c344  E4 E4       CPX $E4\n"
                ".C:c346  90 04       BCC $C34C\n"
                ".C:c348  24 F8       BIT $F8\n"
                ".C:c34a  30 16       BMI $C362\n"
                ".C:c34c  20 5C C1    JSR $C15C\n"
                ".C:c34f  20 ED CB    JSR $CBED\n"
                ".C:c352  90 0E       BCC $C362\n"
                ".C:c354  20 74 CB    JSR $CB74\n"
                ".C:c357  B0 08       BCS $C361\n"
                ".C:c359  38          SEC\n"
                ".C:c35a  24 F8       BIT $F8\n"
                ".C:c35c  70 04       BVS $C362\n"
                ".C:c35e  20 7C C3    JSR $C37C\n"
                ".C:c361  18          CLC\n"
                ".C:c362  60          RTS\n"
                ".C:c363  A6 EB       LDX $EB\n"
                ".C:c365  E4 E4       CPX $E4\n"
                ".C:c367  90 0E       BCC $C377\n"
                ".C:c369  24 F8       BIT $F8\n"
                ".C:c36b  10 06       BPL $C373\n"
                ".C:c36d  A5 E5       LDA $E5\n"
                ".C:c36f  85 EB       STA $EB\n"
                ".C:c371  B0 06       BCS $C379\n"
                ".C:c373  20 A6 C3    JSR $C3A6\n"
                ".C:c376  18          CLC\n"
                ".C:c377  E6 EB       INC $EB\n"
                ".C:c379  4C 5C C1    JMP $C15C\n"
                ".C:c37c  A6 E8       LDX $E8\n"
                ".C:c37e  30 06       BMI $C386\n"
                ".C:c380  E4 EB       CPX $EB\n"
                ".C:c382  90 02       BCC $C386\n"
                ".C:c384  E6 E8       INC $E8\n"
                ".C:c386  A6 E4       LDX $E4\n"
                ".C:c388  20 5E C1    JSR $C15E\n"
                ".C:c38b  A4 E6       LDY $E6\n"
                ".C:c38d  E4 EB       CPX $EB\n"
                ".C:c38f  F0 0F       BEQ $C3A0\n"
                ".C:c391  CA          DEX\n"
                "(C:$c392) d\n"
                ".C:c392  20 76 CB    JSR $CB76\n"
                ".C:c395  E8          INX\n"
                ".C:c396  20 83 CB    JSR $CB83\n"
                ".C:c399  CA          DEX\n"
                ".C:c39a  20 0D C4    JSR $C40D\n"
                ".C:c39d  4C 88 C3    JMP $C388\n"
                ".C:c3a0  20 A5 C4    JSR $C4A5\n"
                ".C:c3a3  4C 93 CB    JMP $CB93\n"
                ".C:c3a6  A6 E5       LDX $E5\n"
                ".C:c3a8  E8          INX\n"
                ".C:c3a9  20 76 CB    JSR $CB76\n"
                ".C:c3ac  90 0A       BCC $C3B8\n"
                ".C:c3ae  E4 E4       CPX $E4\n"
                ".C:c3b0  90 F6       BCC $C3A8\n"
                ".C:c3b2  A6 E5       LDX $E5\n"
                ".C:c3b4  E8          INX\n"
                ".C:c3b5  20 85 CB    JSR $CB85\n"
                ".C:c3b8  C6 EB       DEC $EB\n"
                ".C:c3ba  24 E8       BIT $E8\n"
                ".C:c3bc  30 02       BMI $C3C0\n"
                ".C:c3be  C6 E8       DEC $E8\n"
                ".C:c3c0  A6 E5       LDX $E5\n"
                ".C:c3c2  E4 DF       CPX $DF\n"
                ".C:c3c4  B0 02       BCS $C3C8\n"
                ".C:c3c6  C6 DF       DEC $DF\n"
                ".C:c3c8  20 DC C3    JSR $C3DC\n"
                ".C:c3cb  A6 E5       LDX $E5\n"
                ".C:c3cd  20 76 CB    JSR $CB76\n"
                ".C:c3d0  08          PHP\n"
                ".C:c3d1  20 85 CB    JSR $CB85\n"
                ".C:c3d4  28          PLP\n"
                ".C:c3d5  90 04       BCC $C3DB\n"
                ".C:c3d7  24 F8       BIT $F8\n"
                ".C:c3d9  30 CB       BMI $C3A6\n"
                ".C:c3db  60          RTS\n"
                ".C:c3dc  20 5E C1    JSR $C15E\n"
                ".C:c3df  A4 E6       LDY $E6\n"
                ".C:c3e1  E4 E4       CPX $E4\n"
                ".C:c3e3  B0 0F       BCS $C3F4\n"
                ".C:c3e5  E8          INX\n"
                ".C:c3e6  20 76 CB    JSR $CB76\n"
                ".C:c3e9  CA          DEX\n"
                ".C:c3ea  20 83 CB    JSR $CB83\n"
                ".C:c3ed  E8          INX\n"
                ".C:c3ee  20 0D C4    JSR $C40D\n"
                ".C:c3f1  4C DC C3    JMP $C3DC\n"
                ".C:c3f4  20 A5 C4    JSR $C4A5\n"
                ".C:c3f7  A9 7F       LDA #$7F\n"
                ".C:c3f9  8D 00 DC    STA $DC00\n"
                ".C:c3fc  AD 01 DC    LDA $DC01\n"
                ".C:c3ff  C9 DF       CMP #$DF\n"
                "(C:$c401) d\n"
                ".C:c401  D0 09       BNE $C40C\n"
                ".C:c403  A0 00       LDY #$00\n"
                ".C:c405  EA          NOP\n"
                ".C:c406  CA          DEX\n"
                ".C:c407  D0 FC       BNE $C405\n"
                ".C:c409  88          DEY\n"
                ".C:c40a  D0 F9       BNE $C405\n"
                ".C:c40c  60          RTS\n"
                ".C:c40d  24 D7       BIT $D7\n"
                ".C:c40f  30 25       BMI $C436\n"
                ".C:c411  BD 33 C0    LDA $C033,X\n"
                ".C:c414  85 DC       STA $DC\n"
                ".C:c416  85 DA       STA $DA\n"
                ".C:c418  BD 4C C0    LDA $C04C,X\n"
                ".C:c41b  29 03       AND #$03\n"
                ".C:c41d  0D 3B 0A    ORA $0A3B\n"
                ".C:c420  85 DB       STA $DB\n"
                ".C:c422  29 03       AND #$03\n"
                ".C:c424  09 D8       ORA #$D8\n"
                ".C:c426  85 DD       STA $DD\n"
                ".C:c428  B1 DA       LDA ($DA),Y\n"
                ".C:c42a  91 E0       STA ($E0),Y\n"
                ".C:c42c  B1 DC       LDA ($DC),Y\n"
                ".C:c42e  91 E2       STA ($E2),Y\n"
                ".C:c430  C4 E7       CPY $E7\n"
                ".C:c432  C8          INY\n"
                ".C:c433  90 F3       BCC $C428\n"
                ".C:c435  60          RTS\n"
                ".C:c436  8E 31 0A    STX $0A31\n"
                ".C:c439  8C 32 0A    STY $0A32\n"
                ".C:c43c  A2 18       LDX #$18\n"
                ".C:c43e  20 DA CD    JSR $CDDA\n"
                ".C:c441  09 80       ORA #$80\n"
                ".C:c443  20 CC CD    JSR $CDCC\n"
                ".C:c446  20 E6 CD    JSR $CDE6\n"
                ".C:c449  AE 31 0A    LDX $0A31\n"
                ".C:c44c  BD 33 C0    LDA $C033,X\n"
                ".C:c44f  0A          ASL A\n"
                ".C:c450  85 DA       STA $DA\n"
                ".C:c452  BD 4C C0    LDA $C04C,X\n"
                ".C:c455  29 03       AND #$03\n"
                ".C:c457  2A          ROL A\n"
                ".C:c458  0D 2E 0A    ORA $0A2E\n"
                ".C:c45b  85 DB       STA $DB\n"
                ".C:c45d  A2 20       LDX #$20\n"
                ".C:c45f  18          CLC\n"
                ".C:c460  98          TYA\n"
                ".C:c461  65 DA       ADC $DA\n"
                ".C:c463  85 DA       STA $DA\n"
                ".C:c465  A9 00       LDA #$00\n"
                ".C:c467  65 DB       ADC $DB\n"
                "(C:$c469) d\n"
                ".C:c469  85 DB       STA $DB\n"
                ".C:c46b  20 CC CD    JSR $CDCC\n"
                ".C:c46e  E8          INX\n"
                ".C:c46f  A5 DA       LDA $DA\n"
                ".C:c471  20 CC CD    JSR $CDCC\n"
                ".C:c474  38          SEC\n"
                ".C:c475  A6 E7       LDX $E7\n"
                ".C:c477  E8          INX\n"
                ".C:c478  8A          TXA\n"
                ".C:c479  ED 32 0A    SBC $0A32\n"
                ".C:c47c  8D 32 0A    STA $0A32\n"
                ".C:c47f  A2 1E       LDX #$1E\n"
                ".C:c481  20 CC CD    JSR $CDCC\n"
                ".C:c484  A2 20       LDX #$20\n"
                ".C:c486  A5 DB       LDA $DB\n"
                ".C:c488  29 07       AND #$07\n"
                ".C:c48a  0D 2F 0A    ORA $0A2F\n"
                ".C:c48d  20 CC CD    JSR $CDCC\n"
                ".C:c490  E8          INX\n"
                ".C:c491  A5 DA       LDA $DA\n"
                ".C:c493  20 CC CD    JSR $CDCC\n"
                ".C:c496  20 F9 CD    JSR $CDF9\n"
                ".C:c499  AD 32 0A    LDA $0A32\n"
                ".C:c49c  A2 1E       LDX #$1E\n"
                ".C:c49e  20 CC CD    JSR $CDCC\n"
                ".C:c4a1  AE 31 0A    LDX $0A31\n"
                ".C:c4a4  60          RTS\n"
                ".C:c4a5  A4 E6       LDY $E6\n"
                ".C:c4a7  20 85 CB    JSR $CB85\n"
                ".C:c4aa  20 5E C1    JSR $C15E\n"
                ".C:c4ad  24 D7       BIT $D7\n"
                ".C:c4af  30 0F       BMI $C4C0\n"
                ".C:c4b1  88          DEY\n"
                ".C:c4b2  C8          INY\n"
                ".C:c4b3  A9 20       LDA #$20\n"
                ".C:c4b5  91 E0       STA ($E0),Y\n"
                ".C:c4b7  A5 F1       LDA $F1\n"
                ".C:c4b9  91 E2       STA ($E2),Y\n"
                ".C:c4bb  C4 E7       CPY $E7\n"
                ".C:c4bd  D0 F3       BNE $C4B2\n"
                ".C:c4bf  60          RTS\n"
                ".C:c4c0  8E 31 0A    STX $0A31\n"
                ".C:c4c3  8C 32 0A    STY $0A32\n"
                ".C:c4c6  A2 18       LDX #$18\n"
                ".C:c4c8  20 DA CD    JSR $CDDA\n"
                ".C:c4cb  29 7F       AND #$7F\n"
                ".C:c4cd  20 CC CD    JSR $CDCC\n"
                ".C:c4d0  A2 12       LDX #$12\n"
                ".C:c4d2  18          CLC\n"
                ".C:c4d3  98          TYA\n"
                ".C:c4d4  65 E0       ADC $E0\n"
                "(C:$c4d6) d\n"
                ".C:c4d6  48          PHA\n"
                ".C:c4d7  8D 3C 0A    STA $0A3C\n"
                ".C:c4da  A9 00       LDA #$00\n"
                ".C:c4dc  65 E1       ADC $E1\n"
                ".C:c4de  8D 3D 0A    STA $0A3D\n"
                ".C:c4e1  20 CC CD    JSR $CDCC\n"
                ".C:c4e4  E8          INX\n"
                ".C:c4e5  68          PLA\n"
                ".C:c4e6  20 CC CD    JSR $CDCC\n"
                ".C:c4e9  A9 20       LDA #$20\n"
                ".C:c4eb  20 CA CD    JSR $CDCA\n"
                ".C:c4ee  38          SEC\n"
                ".C:c4ef  A5 E7       LDA $E7\n"
                ".C:c4f1  ED 32 0A    SBC $0A32\n"
                ".C:c4f4  48          PHA\n"
                ".C:c4f5  F0 14       BEQ $C50B\n"
                ".C:c4f7  AA          TAX\n"
                ".C:c4f8  38          SEC\n"
                ".C:c4f9  6D 3C 0A    ADC $0A3C\n"
                ".C:c4fc  8D 3C 0A    STA $0A3C\n"
                ".C:c4ff  A9 00       LDA #$00\n"
                ".C:c501  6D 3D 0A    ADC $0A3D\n"
                ".C:c504  8D 3D 0A    STA $0A3D\n"
                ".C:c507  8A          TXA\n"
                ".C:c508  20 3E C5    JSR $C53E\n"
                ".C:c50b  A2 12       LDX #$12\n"
                ".C:c50d  18          CLC\n"
                ".C:c50e  98          TYA\n"
                ".C:c50f  65 E2       ADC $E2\n"
                ".C:c511  48          PHA\n"
                ".C:c512  A9 00       LDA #$00\n"
                ".C:c514  65 E3       ADC $E3\n"
                ".C:c516  20 CC CD    JSR $CDCC\n"
                ".C:c519  E8          INX\n"
                ".C:c51a  68          PLA\n"
                ".C:c51b  20 CC CD    JSR $CDCC\n"
                ".C:c51e  AD 3D 0A    LDA $0A3D\n"
                ".C:c521  29 07       AND #$07\n"
                ".C:c523  0D 2F 0A    ORA $0A2F\n"
                ".C:c526  8D 3D 0A    STA $0A3D\n"
                ".C:c529  A5 F1       LDA $F1\n"
                ".C:c52b  29 8F       AND #$8F\n"
                ".C:c52d  20 CA CD    JSR $CDCA\n"
                ".C:c530  68          PLA\n"
                ".C:c531  F0 03       BEQ $C536\n"
                ".C:c533  20 3E C5    JSR $C53E\n"
                ".C:c536  AE 31 0A    LDX $0A31\n"
                ".C:c539  A4 E7       LDY $E7\n"
                ".C:c53b  60          RTS\n"
                ".C:c53c  A9 01       LDA #$01\n"
                ".C:c53e  A2 1E       LDX #$1E\n"
                "\n"
);


    //    this->setFont();
    //    this->setMinimumSize(1000,1000);
}

DisassemblyWidget::~DisassemblyWidget() {
    std::cout << "*************************** DisassemblyWidget::~QDisassemblyWidget" << std::endl;
}

//void DisassemblyWidget::paintEvent(QPaintEvent *event) {
//    auto s = size();

//    QPainter painter(this);
//    QBrush brush(Qt::red, Qt::SolidPattern);
//    painter.fillRect(0,0,s.width(), s.height(), brush);

//    painter.drawText(0,20, "$0000 12 34 56 LDA #0");
//}

int DisassemblyWidget::breakpointAreaWidth() {
    return fontMetrics().horizontalAdvance("QQQ");
}


void DisassemblyWidget::updateBreakpointAreaWidth(int /* newBlockCount */) {
    setViewportMargins(breakpointAreaWidth(), 0, 0, 0);
}


void DisassemblyWidget::updateBreakpointArea(const QRect &rect, int dy)
{
    if (dy)
        breakpointArea->scroll(0, dy);
    else
        breakpointArea->update(0, rect.y(), breakpointArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateBreakpointAreaWidth(0);
}




void DisassemblyWidget::resizeEvent(QResizeEvent* e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    breakpointArea->setGeometry(QRect(cr.left(), cr.top(), breakpointAreaWidth(), cr.height()));
}



void DisassemblyWidget::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(60);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}



void DisassemblyWidget::breakpointAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(breakpointArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());


    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, breakpointArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void DisassemblyWidget::onEventFromController(const Event& event) {
    qDebug() << "DisassemblyWidget::onEventFromController called";
}

}
