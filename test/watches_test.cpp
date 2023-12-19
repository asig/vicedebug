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

#include <QTest>

#include <vector>
#include <algorithm>
#include <cstdint>

#include "watches.h"

namespace vicedebug {

class WatchesTest: public QObject
{
    Q_OBJECT

private slots:
    void testInt8AsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory = { {0, { 0x7f, 0x80, 0x81 } } };
        QCOMPARE((Watch{1, Watch::ViewType::INT, 0, 0, 1}.asString(memory)), "127");
        QCOMPARE((Watch{1, Watch::ViewType::INT, 0, 1, 1}.asString(memory)), "-128");
        QCOMPARE((Watch{1, Watch::ViewType::INT, 0, 2, 1}.asString(memory)), "-127");
    }

    void testUInt8AsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory = { {0, { 0x7f, 0x80, 0x81 } } };
        QCOMPARE((Watch{1, Watch::ViewType::UINT, 0, 0, 1}.asString(memory)), "127");
        QCOMPARE((Watch{1, Watch::ViewType::UINT, 0, 1, 1}.asString(memory)), "128");
        QCOMPARE((Watch{1, Watch::ViewType::UINT, 0, 2, 1}.asString(memory)), "129");
    }

    void testUInt8HexAsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory = { {0, { 0x7f, 0x80, 0x81 } } };
        QCOMPARE((Watch{1, Watch::ViewType::UINT_HEX, 0, 0, 1}.asString(memory)), "7f");
        QCOMPARE((Watch{1, Watch::ViewType::UINT_HEX, 0, 1, 1}.asString(memory)), "80");
        QCOMPARE((Watch{1, Watch::ViewType::UINT_HEX, 0, 2, 1}.asString(memory)), "81");
    }

    void testFloatAsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory;

        memory = { {0, { 0x01, 0x00, 0x00, 0x00, 0x00 } }};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "2.938735877e-39"); // CBM Basic says "2.93873588e-39", but I don't want to replicate the actual print routine...

        memory = { {0, { 0xff, 0x7f, 0xff, 0xff, 0xff }}};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "1.701411834e+38"); // CBM Basic says "1.70141183e+38"...

        memory = { {0, { 0xff, 0xff, 0xff, 0xff, 0xff }}};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "-1.701411834e+38"); // CBM Basic says "-1.70141183e+38"...

        memory = { {0, { 0x98, 0x35, 0x44, 0x7A, 0x00 }}};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "11879546.0");

        memory = { {0, { 0x00, 0x35, 0x44, 0x7A, 0x00 }}};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "0.0");

        memory = { {0, { 0x85, 0x3b, 0x5f, 0x5e, 0xfc }}};
        QCOMPARE((Watch{1, Watch::ViewType::FLOAT, 0, 0, 0}.asString(memory)), "23.421567887"); // CBM Basic says "23.4215679"...
    }

    void testBytesAsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory = { {0, { 0x7f, 0x80, 0x81 } } };
        QCOMPARE((Watch{1, Watch::ViewType::BYTES, 0, 0, 3}.asString(memory)), "7f 80 81");
    }

    void testCharsAsString() {
        std::unordered_map<std::uint16_t, std::vector<std::uint8_t>> memory = { {0, { 0x4f, 0x50, 0x51 } } };
        QCOMPARE((Watch{1, Watch::ViewType::CHARS, 0, 0, 3}.asString(memory)), "\uEF0F\uEF10\uEF11");
    }

    void cleanupTestCase() {
    }

};

}

QTEST_MAIN(vicedebug::WatchesTest)

#include "watches_test.moc"
