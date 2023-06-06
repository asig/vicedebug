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

#include <QTest>

#include <vector>
#include <algorithm>
#include <cstdint>

#include "disassembler.h"
#include "disassembler_6502.h"

namespace vicedebug {

class Disassembler6502Test: public QObject
{
    Q_OBJECT

public:
    Disassembler6502Test() : disassembler_(&symtab_) {}

private:
    std::vector<std::uint8_t> memory_;
    SymTable symtab_;
    vicedebug::Disassembler6502 disassembler_;

    void verifyLine(const Disassembler::Line& line, std::uint16_t addr, std::vector<std::uint8_t> bytes, std::string disassembly) {
        QVERIFY2(line.addr == addr, qPrintable(QString("Expected address %1, but got %2").arg(addr).arg(line.addr)));
        QVERIFY2(line.bytes.size() == bytes.size(), qPrintable(QString("Address %1: Expected %2 bytes, but got %3").arg(addr).arg(bytes.size()).arg(line.bytes.size())));
        for (int i = 0; i < bytes.size(); i++) {
            QVERIFY2(line.bytes[i] == bytes[i], qPrintable(QString("Address %1: Byte %2: Expected %3, but got %4").arg(addr).arg(i).arg(bytes[i]).arg(line.bytes[i])));
        }
        QVERIFY2(line.disassembly == disassembly, qPrintable(QString("Address %1: Expected '%2', but got '%3'").arg(addr).arg(disassembly.c_str()).arg(line.disassembly.c_str())));
    }


private slots:
    void initMemory() {
        /* From VICE's x128

  CPU 6502

.C:f000  29 0F       AND #$0F
.C:f002  D0 1F       BNE $F023
.C:f004  20 C8 E9    JSR $E9C8
.C:f007  B0 36       BCS $F03F
.C:f009  20 0F F5    JSR $F50F
.C:f00c  A5 B7       LDA $B7
.C:f00e  F0 0A       BEQ $F01A
.C:f010  20 9A E9    JSR $E99A
.C:f013  90 18       BCC $F02D
.C:f015  F0 28       BEQ $F03F
.C:f017  4C 85 F6    JMP $F685
.C:f01a  20 D0 E8    JSR $E8D0
.C:f01d  90 0E       BCC $F02D
.C:f01f  F0 1E       BEQ $F03F
.C:f021  B0 F4       BCS $F017
.C:f023  20 E9 E9    JSR $E9E9
.C:f026  B0 17       BCS $F03F
.C:f028  A9 04       LDA #$04
.C:f02a  20 19 E9    JSR $E919
.C:f02d  A9 BF       LDA #$BF
.C:f02f  A4 B9       LDY $B9
.C:f031  C0 60       CPY #$60
.C:f033  F0 07       BEQ $F03C
.C:f035  A0 00       LDY #$00
.C:f037  A9 02       LDA #$02
.C:f039  91 B2       STA ($B2),Y
.C:f03b  98          TYA
.C:f03c  85 A6       STA $A6
.C:f03e  18          CLC
.C:f03f  60          RTS
.C:f040  20 B0 F0    JSR $F0B0
.C:f043  8C 14 0A    STY $0A14
.C:f046  C4 B7       CPY $B7
.C:f048  F0 0B       BEQ $F055
.C:f04a  20 AE F7    JSR $F7AE
.C:f04d  99 10 0A    STA $0A10,Y
.C:f050  C8          INY
.C:f051  C0 04       CPY #$04
.C:f053  D0 F1       BNE $F046
.C:f055  20 8E E6    JSR $E68E
.C:f058  8E 15 0A    STX $0A15
.C:f05b  AD 10 0A    LDA $0A10
.C:f05e  29 0F       AND #$0F
.C:f060  F0 1C       BEQ $F07E
.C:f062  0A          ASL A
.C:f063  AA          TAX
.C:f064  AD 03 0A    LDA $0A03
.C:f067  D0 09       BNE $F072
.C:f069  BC 4F E8    LDY $E84F,X
.C:f06c  BD 4E E8    LDA $E84E,X
.C:f06f  4C 78 F0    JMP $F078

*/

        for (int i = 0; i < 0xf000; i++) {
            memory_.push_back(0);
        }
        memory_.insert(memory_.end(), {
                           0x29,0x0F,
                           0xD0,0x1F,
                           0x20,0xC8,0xE9,
                           0xB0,0x36,
                           0x20,0x0F,0xF5,
                           0xA5,0xB7,
                           0xF0,0x0A,
                           0x20,0x9A,0xE9,
                           0x90,0x18,
                           0xF0,0x28,
                           0x4C,0x85,0xF6,
                           0x20,0xD0,0xE8,
                           0x90,0x0E,
                           0xF0,0x1E,
                           0xB0,0xF4,
                           0x20,0xE9,0xE9,
                           0xB0,0x17,
                           0xA9,0x04,
                           0x20,0x19,0xE9,
                           0xA9,0xBF,
                           0xA4,0xB9,
                           0xC0,0x60,
                           0xF0,0x07,
                           0xA0,0x00,
                           0xA9,0x02,
                           0x91,0xB2,
                           0x98,
                           0x85,0xA6,
                           0x18,
                           0x60,
                           0x20,0xB0,0xF0,
                           0x8C,0x14,0x0A,
                           0xC4,0xB7,
                           0xF0,0x0B,
                           0x20,0xAE,0xF7,
                           0x99,0x10,0x0A,
                           0xC8,
                           0xC0,0x04,
                           0xD0,0xF1,
                           0x20,0x8E,0xE6,
                           0x8E,0x15,0x0A,
                           0xAD,0x10,0x0A,
                           0x29,0x0F,
                           0xF0,0x1C,
                           0x0A,
                           0xAA,
                           0xAD,0x03,0x0A,
                           0xD0,0x09,
                           0xBC,0x4F,0xE8,
                           0xBD,0x4E,0xE8,
                           0x4C,0x78,0xF0,
                       });
    }

    void testDisassembleForward6502() {
        initMemory();
        std::vector<vicedebug::Disassembler::Line> lines = disassembler_.disassembleForward(0xf000, memory_, 10);

        QVERIFY(lines.size() == 10);
        verifyLine(lines[0], 0xf000, {0x29, 0x0F      }, "AND #$0F");
        verifyLine(lines[1], 0xf002, {0xD0, 0x1F      }, "BNE $F023");
        verifyLine(lines[2], 0xf004, {0x20, 0xC8, 0xE9}, "JSR $E9C8");
        verifyLine(lines[3], 0xf007, {0xB0, 0x36      }, "BCS $F03F");
        verifyLine(lines[4], 0xf009, {0x20, 0x0F, 0xF5}, "JSR $F50F");
        verifyLine(lines[5], 0xf00c, {0xA5, 0xB7      }, "LDA $B7");
        verifyLine(lines[6], 0xf00e, {0xF0, 0x0A      }, "BEQ $F01A");
        verifyLine(lines[7], 0xf010, {0x20, 0x9A, 0xE9}, "JSR $E99A");
        verifyLine(lines[8], 0xf013, {0x90, 0x18      }, "BCC $F02D");
        verifyLine(lines[9], 0xf015, {0xF0, 0x28      }, "BEQ $F03F");

        lines = disassembler_.disassembleForward(0xf043, memory_, 6);
        QVERIFY(lines.size() == 6);
        verifyLine(lines[0], 0xf043, {0x8C, 0x14, 0x0A}, "STY $0A14");
        verifyLine(lines[1], 0xf046, {0xC4, 0xB7      }, "CPY $B7");
        verifyLine(lines[2], 0xf048, {0xF0, 0x0B      }, "BEQ $F055");
        verifyLine(lines[3], 0xf04a, {0x20, 0xAE, 0xF7}, "JSR $F7AE");
        verifyLine(lines[4], 0xf04d, {0x99, 0x10, 0x0A}, "STA $0A10,Y");
        verifyLine(lines[5], 0xf050, {0xC8            }, "INY");
    }

    void testDisassembleBackward6502() {
        initMemory();
        std::vector<vicedebug::Disassembler::Line> lines = disassembler_.disassembleBackward(0xf017, memory_, 10, {});
        QVERIFY(lines.size() == 10);
        verifyLine(lines[0], 0xf000, {0x29, 0x0F      }, "AND #$0F");
        verifyLine(lines[1], 0xf002, {0xD0, 0x1F      }, "BNE $F023");
        verifyLine(lines[2], 0xf004, {0x20, 0xC8, 0xE9}, "JSR $E9C8");
        verifyLine(lines[3], 0xf007, {0xB0, 0x36      }, "BCS $F03F");
        verifyLine(lines[4], 0xf009, {0x20, 0x0F, 0xF5}, "JSR $F50F");
        verifyLine(lines[5], 0xf00c, {0xA5, 0xB7      }, "LDA $B7");
        verifyLine(lines[6], 0xf00e, {0xF0, 0x0A      }, "BEQ $F01A");
        verifyLine(lines[7], 0xf010, {0x20, 0x9A, 0xE9}, "JSR $E99A");
        verifyLine(lines[8], 0xf013, {0x90, 0x18      }, "BCC $F02D");
        verifyLine(lines[9], 0xf015, {0xF0, 0x28      }, "BEQ $F03F");

        lines = disassembler_.disassembleBackward(0xf051, memory_, 6, {});
        QVERIFY(lines.size() == 6);
        verifyLine(lines[0], 0xf043, {0x8C, 0x14, 0x0A}, "STY $0A14");
        verifyLine(lines[1], 0xf046, {0xC4, 0xB7      }, "CPY $B7");
        verifyLine(lines[2], 0xf048, {0xF0, 0x0B      }, "BEQ $F055");
        verifyLine(lines[3], 0xf04a, {0x20, 0xAE, 0xF7}, "JSR $F7AE");
        verifyLine(lines[4], 0xf04d, {0x99, 0x10, 0x0A}, "STA $0A10,Y");
        verifyLine(lines[5], 0xf050, {0xC8            }, "INY");
    }


    void cleanupTestCase() {
    }

};

}

QTEST_MAIN(vicedebug::Disassembler6502Test)

#include "disassembler_6502_test.moc"
