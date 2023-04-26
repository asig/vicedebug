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

#include "disassembler_6502.h"

#include <string>
#include <QString>

namespace vicedebug {

namespace {

enum AddressMode {
    AM_ABSOLUTE,
    AM_ZERO_PAGE,
    AM_ZERO_PAGE_X,
    AM_ZERO_PAGE_Y,
    AM_ACCUMULATOR,
    AM_IMMEDIATE,
    AM_INDIRECT,
    AM_INDIRECT_X,
    AM_INDIRECT_Y,
    AM_INDEXED_X,
    AM_INDEXED_Y,
    AM_RELATIVE,
    AM_IMPLIED
};

static int additionalBytes[] = {
    /* AM_ABSOLUTE    */ 2,
    /* AM_ZERO_PAGE   */ 1,
    /* AM_ZERO_PAGE_X */ 1,
    /* AM_ZERO_PAGE_Y */ 1,
    /* AM_ACCUMULATOR */ 0,
    /* AM_IMMEDIATE   */ 1,
    /* AM_INDIRECT    */ 2,
    /* AM_INDIRECT_X  */ 1,
    /* AM_INDIRECT_Y  */ 1,
    /* AM_INDEXED_X   */ 2,
    /* AM_INDEXED_Y   */ 2,
    /* AM_RELATIVE    */ 1,
    /* AM_IMPLIED     */ 0,
};

struct InstrDesc {
    std::string mnemo;
    AddressMode mode;
    bool illegal;
};

InstrDesc instructions[256] = {
    /* 0x00 */ {"BRK",AM_IMPLIED, false},
    /* 0x01 */ {"ORA",AM_INDIRECT_X, false},
    /* 0x02 */ {"JAM",AM_IMPLIED, true},
    /* 0x03 */ {"SLO",AM_INDIRECT_X, true},
    /* 0x04 */ {"NOOP",AM_ZERO_PAGE, true},
    /* 0x05 */ {"ORA",AM_ZERO_PAGE, false},
    /* 0x06 */ {"ASL",AM_ZERO_PAGE, false},
    /* 0x07 */ {"SLO",AM_ZERO_PAGE, true},
    /* 0x08 */ {"PHP",AM_IMPLIED, false},
    /* 0x09 */ {"ORA",AM_IMMEDIATE, false},
    /* 0x0a */ {"ASL",AM_ACCUMULATOR, false},
    /* 0x0B */ {"ANC",AM_IMMEDIATE, true},
    /* 0x0C */ {"NOOP",AM_ABSOLUTE, true},
    /* 0x0d */ {"ORA",AM_ABSOLUTE, false},
    /* 0x0e */ {"ASL",AM_ABSOLUTE, false},
    /* 0x0F */ {"SLO",AM_ABSOLUTE, true},
    /* 0x10 */ {"BPL",AM_RELATIVE, false},
    /* 0x11 */ {"ORA",AM_INDIRECT_Y, false},
    /* 0x12 */ {"JAM",AM_IMPLIED, true},
    /* 0x13 */ {"SLO",AM_INDIRECT_Y, true},
    /* 0x14 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0x15 */ {"ORA",AM_ZERO_PAGE_X, false},
    /* 0x16 */ {"ASL",AM_ZERO_PAGE_X, false},
    /* 0x17 */ {"SLO",AM_ZERO_PAGE_X, true},
    /* 0x18 */ {"CLC",AM_IMPLIED, false},
    /* 0x19 */ {"ORA",AM_INDEXED_Y, false},
    /* 0x1A */ {"NOOP",AM_IMPLIED, true},
    /* 0x1B */ {"SLO",AM_INDEXED_Y, true},
    /* 0x1C */ {"NOOP",AM_INDEXED_X, true},
    /* 0x1d */ {"ORA",AM_INDEXED_X, false},
    /* 0x1e */ {"ASL",AM_INDEXED_X, false},
    /* 0x1F */ {"SLO",AM_INDEXED_X, true},
    /* 0x20 */ {"JSR",AM_ABSOLUTE, false},
    /* 0x21 */ {"AND",AM_INDIRECT_X, false},
    /* 0x22 */ {"JAM",AM_IMPLIED, true},
    /* 0x23 */ {"RLA",AM_INDIRECT_X, true},
    /* 0x24 */ {"BIT",AM_ZERO_PAGE, false},
    /* 0x25 */ {"AND",AM_ZERO_PAGE, false},
    /* 0x26 */ {"ROL",AM_ZERO_PAGE, false},
    /* 0x27 */ {"RLA",AM_ZERO_PAGE, true},
    /* 0x28 */ {"PLP",AM_IMPLIED, false},
    /* 0x29 */ {"AND",AM_IMMEDIATE, false},
    /* 0x2a */ {"ROL",AM_ACCUMULATOR, false},
    /* 0x2B */ {"ANC",AM_IMMEDIATE, true},
    /* 0x2c */ {"BIT",AM_ABSOLUTE, false},
    /* 0x2d */ {"AND",AM_ABSOLUTE, false},
    /* 0x2e */ {"ROL",AM_ABSOLUTE, false},
    /* 0x2F */ {"RLA",AM_ABSOLUTE, true},
    /* 0x30 */ {"BMI",AM_RELATIVE, false},
    /* 0x31 */ {"AND",AM_INDIRECT_Y, false},
    /* 0x32 */ {"JAM",AM_IMPLIED, true},
    /* 0x33 */ {"RLA",AM_INDIRECT_Y, true},
    /* 0x34 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0x35 */ {"AND",AM_ZERO_PAGE_X, false},
    /* 0x36 */ {"ROL",AM_ZERO_PAGE_X, false},
    /* 0x37 */ {"RLA",AM_ZERO_PAGE_X, true},
    /* 0x38 */ {"SEC",AM_IMPLIED, false},
    /* 0x39 */ {"AND",AM_INDEXED_Y, false},
    /* 0x3A */ {"NOOP",AM_IMPLIED, true},
    /* 0x3B */ {"RLA",AM_INDEXED_Y, true},
    /* 0x3C */ {"NOOP",AM_INDEXED_X, true},
    /* 0x3d */ {"AND",AM_INDEXED_X, false},
    /* 0x3e */ {"ROL",AM_INDEXED_X, false},
    /* 0x3F */ {"RLA",AM_INDEXED_X, true},
    /* 0x40 */ {"RTI",AM_IMPLIED, false},
    /* 0x41 */ {"EOR",AM_INDIRECT_X, false},
    /* 0x42 */ {"JAM",AM_IMPLIED, true},
    /* 0x43 */ {"SRE",AM_INDIRECT_X, true},
    /* 0x44 */ {"NOOP",AM_ZERO_PAGE, true},
    /* 0x45 */ {"EOR",AM_ZERO_PAGE, false},
    /* 0x46 */ {"LSR",AM_ZERO_PAGE, false},
    /* 0x47 */ {"SRE",AM_ZERO_PAGE, true},
    /* 0x48 */ {"PHA",AM_IMPLIED, false},
    /* 0x49 */ {"EOR",AM_IMMEDIATE, false},
    /* 0x4a */ {"LSR",AM_ACCUMULATOR, false},
    /* 0x4B */ {"ALR",AM_IMMEDIATE, true},
    /* 0x4c */ {"JMP",AM_ABSOLUTE, false},
    /* 0x4d */ {"EOR",AM_ABSOLUTE, false},
    /* 0x4e */ {"LSR",AM_ABSOLUTE, false},
    /* 0x4F */ {"SRE",AM_ABSOLUTE, true},
    /* 0x50 */ {"BVC",AM_RELATIVE, false},
    /* 0x51 */ {"EOR",AM_INDIRECT_Y, false},
    /* 0x52 */ {"JAM",AM_IMPLIED, true},
    /* 0x53 */ {"SRE",AM_INDIRECT_Y, true},
    /* 0x54 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0x55 */ {"EOR",AM_ZERO_PAGE_X, false},
    /* 0x56 */ {"LSR",AM_ZERO_PAGE_X, false},
    /* 0x57 */ {"SRE",AM_ZERO_PAGE_X, true},
    /* 0x58 */ {"CLI",AM_IMPLIED, false},
    /* 0x59 */ {"EOR",AM_INDEXED_Y, false},
    /* 0x5A */ {"NOOP",AM_IMPLIED, true},
    /* 0x5B */ {"SRE",AM_INDEXED_Y, true},
    /* 0x5C */ {"NOOP",AM_INDEXED_X, true},
    /* 0x5d */ {"EOR",AM_INDEXED_X, false},
    /* 0x5e */ {"LSR",AM_INDEXED_X, false},
    /* 0x5F */ {"SRE",AM_INDEXED_X, true},
    /* 0x60 */ {"RTS",AM_IMPLIED, false},
    /* 0x61 */ {"ADC",AM_INDIRECT_X, false},
    /* 0x62 */ {"JAM",AM_IMPLIED, true},
    /* 0x63 */ {"RRA",AM_INDIRECT_X, true},
    /* 0x64 */ {"NOOP",AM_ZERO_PAGE, true},
    /* 0x65 */ {"ADC",AM_ZERO_PAGE, false},
    /* 0x66 */ {"ROR",AM_ZERO_PAGE, false},
    /* 0x67 */ {"RRA",AM_ZERO_PAGE, true},
    /* 0x68 */ {"PLA",AM_IMPLIED, false},
    /* 0x69 */ {"ADC",AM_IMMEDIATE, false},
    /* 0x6a */ {"ROR",AM_ACCUMULATOR, false},
    /* 0x6B */ {"ARR",AM_IMMEDIATE, true},
    /* 0x6c */ {"JMP",AM_INDIRECT, false},
    /* 0x6d */ {"ADC",AM_ABSOLUTE, false},
    /* 0x6e */ {"ROR",AM_INDEXED_X, false},
    /* 0x6F */ {"RRA",AM_ABSOLUTE, true},
    /* 0x70 */ {"BVS",AM_RELATIVE, false},
    /* 0x71 */ {"ADC",AM_INDIRECT_Y, false},
    /* 0x72 */ {"JAM",AM_IMPLIED, true},
    /* 0x73 */ {"RRA",AM_INDIRECT_Y, true},
    /* 0x74 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0x75 */ {"ADC",AM_ZERO_PAGE_X, false},
    /* 0x76 */ {"ROR",AM_ZERO_PAGE_X, false},
    /* 0x77 */ {"RRA",AM_ZERO_PAGE_X, true},
    /* 0x78 */ {"SEI",AM_IMPLIED, false},
    /* 0x79 */ {"ADC",AM_INDEXED_Y, false},
    /* 0x7A */ {"NOOP",AM_IMPLIED, true},
    /* 0x7B */ {"RRA",AM_INDEXED_Y, true},
    /* 0x7C */ {"NOOP",AM_INDEXED_X, true},
    /* 0x7d */ {"ADC",AM_INDEXED_X, false},
    /* 0x7e */ {"ROR",AM_ABSOLUTE, false},
    /* 0x7F */ {"RRA",AM_INDEXED_X, true},
    /* 0x80 */ {"NOOP",AM_IMMEDIATE, true},
    /* 0x81 */ {"STA",AM_INDIRECT_X, false},
    /* 0x82 */ {"NOOP",AM_IMMEDIATE, true},
    /* 0x83 */ {"SAX",AM_INDIRECT_X, true},
    /* 0x84 */ {"STY",AM_ZERO_PAGE, false},
    /* 0x85 */ {"STA",AM_ZERO_PAGE, false},
    /* 0x86 */ {"STX",AM_ZERO_PAGE, false},
    /* 0x87 */ {"SAX",AM_ZERO_PAGE, true},
    /* 0x88 */ {"DEY",AM_IMPLIED, false},
    /* 0x89 */ {"NOOP",AM_IMMEDIATE, true},
    /* 0x8a */ {"TXA",AM_IMPLIED, false},
    /* 0x8B */ {"ANE",AM_IMMEDIATE, true},
    /* 0x8c */ {"STY",AM_ABSOLUTE, false},
    /* 0x8d */ {"STA",AM_ABSOLUTE, false},
    /* 0x8e */ {"STX",AM_ABSOLUTE, false},
    /* 0x8F */ {"SAX",AM_ABSOLUTE, true},
    /* 0x90 */ {"BCC",AM_RELATIVE, false},
    /* 0x91 */ {"STA",AM_INDIRECT_Y, false},
    /* 0x92 */ {"JAM",AM_IMPLIED, true},
    /* 0x93 */ {"SHA",AM_INDIRECT_Y, true},
    /* 0x94 */ {"STY",AM_ZERO_PAGE_X, false},
    /* 0x95 */ {"STA",AM_ZERO_PAGE_X, false},
    /* 0x96 */ {"STX",AM_ZERO_PAGE_Y, false},
    /* 0x97 */ {"SAX",AM_ZERO_PAGE_Y, true},
    /* 0x98 */ {"TYA",AM_IMPLIED, false},
    /* 0x99 */ {"STA",AM_INDEXED_Y, false},
    /* 0x9a */ {"TXS",AM_IMPLIED, false},
    /* 0x9B */ {"TAS",AM_INDEXED_Y, true},
    /* 0x9C */ {"SHY",AM_INDEXED_X, true},
    /* 0x9d */ {"STA",AM_INDEXED_X, false},
    /* 0x9E */ {"SHX",AM_INDEXED_Y, true},
    /* 0x9F */ {"SHA",AM_INDEXED_Y, true},
    /* 0xa0 */ {"LDY",AM_IMMEDIATE, false},
    /* 0xa1 */ {"LDA",AM_INDIRECT_X, false},
    /* 0xa2 */ {"LDX",AM_IMMEDIATE, false},
    /* 0xA3 */ {"LAX",AM_INDIRECT_X, true},
    /* 0xa4 */ {"LDY",AM_ZERO_PAGE, false},
    /* 0xa5 */ {"LDA",AM_ZERO_PAGE, false},
    /* 0xa6 */ {"LDX",AM_ZERO_PAGE, false},
    /* 0xA7 */ {"LAX",AM_ZERO_PAGE, true},
    /* 0xa8 */ {"TAY",AM_IMPLIED, false},
    /* 0xa9 */ {"LDA",AM_IMMEDIATE, false},
    /* 0xaa */ {"TAX",AM_IMPLIED, false},
    /* 0xAB */ {"LXA",AM_IMMEDIATE, true},
    /* 0xac */ {"LDY",AM_ABSOLUTE, false},
    /* 0xad */ {"LDA",AM_ABSOLUTE, false},
    /* 0xae */ {"LDX",AM_ABSOLUTE, false},
    /* 0xAF */ {"LAX",AM_ABSOLUTE, true},
    /* 0xB0 */ {"BCS",AM_RELATIVE, false},
    /* 0xb1 */ {"LDA",AM_INDIRECT_Y, false},
    /* 0xB2 */ {"JAM",AM_IMPLIED, true},
    /* 0xB3 */ {"LAX",AM_INDIRECT_Y, true},
    /* 0xb4 */ {"LDY",AM_ZERO_PAGE_X, false},
    /* 0xb5 */ {"LDA",AM_ZERO_PAGE_X, false},
    /* 0xb6 */ {"LDX",AM_ZERO_PAGE_Y, false},
    /* 0xB7 */ {"LAX",AM_ZERO_PAGE_Y, true},
    /* 0xb8 */ {"CLV",AM_IMPLIED, false},
    /* 0xb9 */ {"LDA",AM_INDEXED_Y, false},
    /* 0xba */ {"TSX",AM_IMPLIED, false},
    /* 0xBB */ {"LAS",AM_INDEXED_Y, true},
    /* 0xbc */ {"LDY",AM_INDEXED_X, false},
    /* 0xbd */ {"LDA",AM_INDEXED_X, false},
    /* 0xbe */ {"LDX",AM_INDEXED_Y, false},
    /* 0xBF */ {"LAX",AM_INDEXED_Y, true},
    /* 0xc0 */ {"CPY",AM_IMMEDIATE, false},
    /* 0xc1 */ {"CMP",AM_INDIRECT_X, false},
    /* 0xC2 */ {"NOOP",AM_IMMEDIATE, true},
    /* 0xC3 */ {"DCP",AM_INDIRECT_X, true},
    /* 0xc4 */ {"CPY",AM_ZERO_PAGE, false},
    /* 0xc5 */ {"CMP",AM_ZERO_PAGE, false},
    /* 0xc6 */ {"DEC",AM_ZERO_PAGE, false},
    /* 0xC7 */ {"DCP",AM_ZERO_PAGE, true},
    /* 0xc8 */ {"INY",AM_IMPLIED, false},
    /* 0xc9 */ {"CMP",AM_IMMEDIATE, false},
    /* 0xca */ {"DEX",AM_IMPLIED, false},
    /* 0xCB */ {"SBX",AM_IMMEDIATE, true},
    /* 0xcc */ {"CPY",AM_ABSOLUTE, false},
    /* 0xcd */ {"CMP",AM_ABSOLUTE, false},
    /* 0xce */ {"DEC",AM_ABSOLUTE, false},
    /* 0xCF */ {"DCP",AM_ABSOLUTE, true},
    /* 0xD0 */ {"BNE",AM_RELATIVE, false},
    /* 0xd1 */ {"CMP",AM_INDIRECT_Y, false},
    /* 0xD2 */ {"JAM",AM_IMPLIED, true},
    /* 0xD3 */ {"DCP",AM_INDIRECT_Y, true},
    /* 0xD4 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0xd5 */ {"CMP",AM_ZERO_PAGE_X, false},
    /* 0xd6 */ {"DEC",AM_ZERO_PAGE_X, false},
    /* 0xD7 */ {"DCP",AM_ZERO_PAGE_X, true},
    /* 0xd8 */ {"CLD",AM_IMPLIED, false},
    /* 0xd9 */ {"CMP",AM_INDEXED_Y, false},
    /* 0xDA */ {"NOOP",AM_IMPLIED, true},
    /* 0xDB */ {"DCP",AM_INDEXED_Y, true},
    /* 0xDC */ {"NOOP",AM_INDEXED_X, true},
    /* 0xdd */ {"CMP",AM_INDEXED_X, false},
    /* 0xde */ {"DEC",AM_INDEXED_X, false},
    /* 0xDF */ {"DCP",AM_INDEXED_X, true},
    /* 0xe0 */ {"CPX",AM_IMMEDIATE, false},
    /* 0xe1 */ {"SBC",AM_INDIRECT_X, false},
    /* 0xE2 */ {"NOOP",AM_IMMEDIATE, true},
    /* 0xE3 */ {"ISC",AM_INDIRECT_X, true},
    /* 0xe4 */ {"CPX",AM_ZERO_PAGE, false},
    /* 0xe5 */ {"SBC",AM_ZERO_PAGE, false},
    /* 0xe6 */ {"INC",AM_ZERO_PAGE, false},
    /* 0xE7 */ {"ISC",AM_ZERO_PAGE, true},
    /* 0xe8 */ {"INX",AM_IMPLIED, false},
    /* 0xe9 */ {"SBC",AM_IMMEDIATE, false},
    /* 0xea */ {"NOP",AM_IMPLIED, false},
    /* 0xEB */ {"USBC",AM_IMMEDIATE, true},
    /* 0xec */ {"CPX",AM_ABSOLUTE, false},
    /* 0xed */ {"SBC",AM_ABSOLUTE, false},
    /* 0xee */ {"INC",AM_ABSOLUTE, false},
    /* 0xEF */ {"ISC",AM_ABSOLUTE, true},
    /* 0xF0 */ {"BEQ",AM_RELATIVE, false},
    /* 0xf1 */ {"SBC",AM_INDIRECT_Y, false},
    /* 0xF2 */ {"JAM",AM_IMPLIED, true},
    /* 0xF3 */ {"ISC",AM_INDIRECT_Y, true},
    /* 0xF4 */ {"NOOP",AM_ZERO_PAGE_X, true},
    /* 0xf5 */ {"SBC",AM_ZERO_PAGE_X, false},
    /* 0xf6 */ {"INC",AM_ZERO_PAGE_X, false},
    /* 0xF7 */ {"ISC",AM_ZERO_PAGE_X, true},
    /* 0xf8 */ {"SED",AM_IMPLIED, false},
    /* 0xf9 */ {"SBC",AM_INDEXED_Y, false},
    /* 0xFA */ {"NOOP",AM_IMPLIED, true},
    /* 0xFB */ {"ISC",AM_INDEXED_Y, true},
    /* 0xFC */ {"NOOP",AM_INDEXED_X, true},
    /* 0xfd */ {"SBC",AM_INDEXED_X, false},
    /* 0xfe */ {"INC",AM_INDEXED_X, false},
    /* 0xFF */ {"ISC",AM_INDEXED_X, true}
};

}

bool Disassembler6502::checkValidInstr(int depth, std::uint16_t pos, const std::vector<std::uint8_t>& memory, int len, bool illegalAllowed) {
    if (pos < 0 || pos+len > 0xffff) {
        // out of range
        return false;
    }
    const InstrDesc& desc = instructions[memory[pos]];
    if (len != additionalBytes[desc.mode]+1) {
        // length is not matching
        return false;
    }
    if (!illegalAllowed && desc.illegal) {
        // illegal instructions not allowed
        return false;
    }
    if (depth < 2) {
        // Check there is also a valid instr before this one before we report success.
        // We do this to have better stability in backward disassembly.
        return
                checkValidInstr(depth+1, pos - 3, memory, 3, illegalAllowed) ||
                checkValidInstr(depth+1, pos - 2, memory, 2, illegalAllowed) ||
                checkValidInstr(depth+1, pos - 1, memory, 1, illegalAllowed);
    }
    return true;
}
 
std::vector<Disassembler::Line> Disassembler6502::disassembleBackward(std::uint16_t pos, const std::vector<std::uint8_t>& memory, int lines, const std::vector<Disassembler::Line>& disassemblyHint) {
    std::vector<Line> res;

    while(lines-- > 0 && pos > 0) {
        std::uint16_t p = pos;
        std::uint16_t tmp;
//        // Go for the longest non-illegal sequence before pos
//        if (checkValidInstr(pos - 3, memory, 3, /* illegalAllowed= */false)) {
//            tmp = pos - 3;
//            res.push_back(disassembleLine(tmp, memory));
//            pos = p - 3;
//        } else if (checkValidInstr(pos - 2, memory, 2, /* illegalAllowed= */false)) {
//            tmp = pos - 2;
//            res.push_back(disassembleLine(tmp, memory));
//            pos = p - 2;
//        } else if (checkValidInstr(pos - 1, memory, 1, /* ilelgalAllowed= */false)) {
//            tmp = pos - 1;
//            res.push_back(disassembleLine(tmp, memory));
//            pos = p - 1;
//        } else
        if (checkValidInstr(0, pos - 3, memory, 3, /* illegalAllowed= */true)) {
            tmp = pos - 3;
            res.push_back(disassembleLine(tmp, memory));
            pos = p - 3;
        } else if (checkValidInstr(0, pos - 2, memory, 2, /* illegalAllowed= */true)) {
            tmp = pos - 2;
            res.push_back(disassembleLine(tmp, memory));
            pos = p - 2;
        } else if (checkValidInstr(0, pos - 1, memory, 1, /* illegalAllowed= */true)) {
            tmp = pos - 1;
            res.push_back(disassembleLine(tmp, memory));
            pos = p - 1;
        } else {
            // No valid instruction?
            pos--;
            Line l;
            l.addr = pos;
            std::uint8_t b = memory[pos % 0xffff];
            l.bytes.push_back(b);
            l.disassembly = "???";
            res.push_back(l);
        }
    }
    std::reverse(res.begin(), res.end());
    return res;
}

Disassembler::Line Disassembler6502::disassembleLine(std::uint16_t& pos, const std::vector<std::uint8_t>& memory) {
    Line res;
    res.addr = pos;

    std::uint8_t b = memory[pos++ % 0xffff];
    res.bytes.push_back(b);

    const InstrDesc& desc = instructions[b];
    res.disassembly = desc.mnemo;

    std::uint8_t t1,t2;
    std::uint16_t t3;
    switch(desc.mode) {
    case AM_ABSOLUTE:
        t1 = memory[pos++ % 0xffff];
        t2 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.bytes.push_back(t2);
        res.disassembly += QString::asprintf(" $%04X", t2 << 8 | t1).toStdString();
        break;
    case AM_ZERO_PAGE:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.disassembly += QString::asprintf(" $%02X", t1).toStdString();
        break;
    case AM_ZERO_PAGE_X:
    case AM_ZERO_PAGE_Y:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.disassembly += QString::asprintf(" $%02X,%s", t1, desc.mode==AM_ZERO_PAGE_X ? "X":"Y").toStdString();
        break;
    case AM_ACCUMULATOR:
        break;
    case AM_IMMEDIATE:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.disassembly += QString::asprintf(" #$%02X", t1).toStdString();
        break;
    case AM_INDIRECT:
        t1 = memory[pos++ % 0xffff];
        t2 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.bytes.push_back(t2);
        res.disassembly += QString::asprintf(" ($%04X)", t2 << 8 | t1).toStdString();
        break;
    case AM_INDIRECT_X:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.disassembly += QString::asprintf(" ($%02X,X)", t1).toStdString();
        break;
    case AM_INDIRECT_Y:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.disassembly += QString::asprintf(" ($%02X),Y", t1).toStdString();
        break;
    case AM_INDEXED_X:
    case AM_INDEXED_Y:
        t1 = memory[pos++ % 0xffff];
        t2 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        res.bytes.push_back(t2);
        res.disassembly += QString::asprintf(" $%04X,%s", t2 << 8 | t1, desc.mode==AM_INDEXED_X ? "X":"Y").toStdString();
        break;
    case AM_RELATIVE:
        t1 = memory[pos++ % 0xffff];
        res.bytes.push_back(t1);
        t3 = pos + (std::int8_t)t1;
        res.disassembly += QString::asprintf(" $%04X", t3).toStdString();
        break;
    case AM_IMPLIED:
        break;
    default:
        ;
    }
    return res;
}

}
