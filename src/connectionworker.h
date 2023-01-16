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

#include <QThread>
#include <QObject>
#include <QIODevice>
#include <QTcpSocket>
#include <QPromise>

namespace vicedebug {

enum BinaryCommand {
    // Copied from https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/monitor/monitor_binary.c
    CMD_INVALID = 0x00,

    CMD_MEM_GET = 0x01,
    CMD_MEM_SET = 0x02,

    CMD_CHECKPOINT_GET = 0x11,
    CMD_CHECKPOINT_SET = 0x12,
    CMD_CHECKPOINT_DELETE = 0x13,
    CMD_CHECKPOINT_LIST = 0x14,
    CMD_CHECKPOINT_TOGGLE = 0x15,

    CMD_CONDITION_SET = 0x22,

    CMD_REGISTERS_GET = 0x31,
    CMD_REGISTERS_SET = 0x32,

    CMD_DUMP = 0x41,
    CMD_UNDUMP = 0x42,

    CMD_RESOURCE_GET = 0x51,
    CMD_RESOURCE_SET = 0x52,

    CMD_ADVANCE_INSTRUCTIONS = 0x71,
    CMD_KEYBOARD_FEED = 0x72,
    CMD_EXECUTE_UNTIL_RETURN = 0x73,

    CMD_PING = 0x81,
    CMD_BANKS_AVAILABLE = 0x82,
    CMD_REGISTERS_AVAILABLE = 0x83,
    CMD_DISPLAY_GET = 0x84,
    CMD_VICE_INFO = 0x85,

    CMD_PALETTE_GET = 0x91,

    CMD_JOYPORT_SET = 0xa2,

    CMD_USERPORT_SET = 0xb2,

    CMD_EXIT = 0xaa,
    CMD_QUIT = 0xbb,
    CMD_RESET = 0xcc,
    CMD_AUTOSTART = 0xdd,
};

enum ResponseType {
    // Copied from https://sourceforge.net/p/vice-emu/code/HEAD/tree/trunk/vice/src/monitor/monitor_binary.c

    RESPONSE_INVALID = 0x00,
    RESPONSE_MEM_GET = 0x01,
    RESPONSE_MEM_SET = 0x02,

    RESPONSE_CHECKPOINT_INFO = 0x11,

    RESPONSE_CHECKPOINT_DELETE = 0x13,
    RESPONSE_CHECKPOINT_LIST = 0x14,
    RESPONSE_CHECKPOINT_TOGGLE = 0x15,

    RESPONSE_CONDITION_SET = 0x22,

    RESPONSE_REGISTER_INFO = 0x31,

    RESPONSE_DUMP = 0x41,
    RESPONSE_UNDUMP = 0x42,

    RESPONSE_RESOURCE_GET = 0x51,
    RESPONSE_RESOURCE_SET = 0x52,

    RESPONSE_JAM = 0x61,
    RESPONSE_STOPPED = 0x62,
    RESPONSE_RESUMED = 0x63,

    RESPONSE_ADVANCE_INSTRUCTIONS = 0x71,
    RESPONSE_KEYBOARD_FEED = 0x72,
    RESPONSE_EXECUTE_UNTIL_RETURN = 0x73,

    RESPONSE_PING = 0x81,
    RESPONSE_BANKS_AVAILABLE = 0x82,
    RESPONSE_REGISTERS_AVAILABLE = 0x83,
    RESPONSE_DISPLAY_GET = 0x84,
    RESPONSE_VICE_INFO = 0x85,

    RESPONSE_PALETTE_GET = 0x91,

    RESPONSE_JOYPORT_SET = 0xa2,

    RESPONSE_USERPORT_SET = 0xb2,

    RESPONSE_EXIT = 0xaa,
    RESPONSE_QUIT = 0xbb,
    RESPONSE_RESET = 0xcc,
    RESPONSE_AUTOSTART = 0xdd,
};

struct Response {
    std::uint8_t errorCode;
    std::uint32_t id;
    ResponseType responseType;
};

struct MemGetResponse : public Response {
    std::vector<std::uint8_t> memory;
};

struct MemSetResponse : public Response {
    // EMPTY
};

struct RegistersResponse : public Response {
    std::map<std::uint8_t, std::uint16_t> values;
};

struct RegInfo {
    int bits;
    std::string name;
};

struct RegistersAvailableResponse : public Response {
    std::map<std::uint8_t, RegInfo> regInfos;
};

struct CheckpointInfo {
    std::uint32_t number;
    bool hit;
    std::uint16_t startAddress;
    std::uint16_t endAddress;
    bool stopWhenHit;
    bool enabled;
    std::uint8_t op; // 0x01: load, 0x02: store, 0x04: exec
    bool isTemporary;
    std::uint32_t hitCount;
    std::uint32_t ignoreCount;
    bool hasCondition;
    std::uint8_t memspace;
};

struct CheckpointListResponse : public Response {
    std::uint32_t nofCheckpoints;
    std::vector<CheckpointInfo> checkpoints;
};

struct CheckpointInfoResponse : public Response {
    CheckpointInfo checkpoint;
};

struct CheckpointDeleteResponse : public Response {
    // EMPTY
};

struct CheckpointToggleResponse : public Response {
    // EMPTY
};

struct ExitResponse : public Response {
    // EMPTY
};

struct AdvanceInstructionsResponse : public Response {
    // EMPTY
};

struct ExecuteUntilReturnResponse : public Response {
    // EMPTY
};

struct BanksAvailableResponse : public Response {
    std::map<std::uint16_t, std::string> banks;
};

struct StoppedResponse : public Response {
    std::uint16_t pc;
};

struct ResumedResponse : public Response {
    std::uint16_t pc;
};

class ResponseSetter {
public:
    virtual ~ResponseSetter() {};

    virtual void set(std::shared_ptr<Response> response) = 0;
    virtual bool responseIsComplete() = 0;
};

class MessageCollector {
public:
    MessageCollector() {
        reset();
    };

    void reset() {
        headerRemaining_ = 12;
        bodyRemaining_ = 0;
        buf_.clear();
    }

    int remaining() {
        if (headerRemaining_ > 0) {
            return headerRemaining_;
        }
        return bodyRemaining_;
    }

    void append(std::uint8_t val) {
        buf_.push_back(val);
        if (headerRemaining_ > 0) {
            headerRemaining_--;
            if (headerRemaining_ == 0) {
                // Header complete: extract body length
                bodyRemaining_ = buf_[5] << 24 | buf_[4] << 16 | buf_[3] << 8 | buf_[2];
            }
        } else {
            bodyRemaining_--;
        }
    }

    std::vector<std::uint8_t> data() {
        return buf_;
    }

private:
    int headerRemaining_;
    int bodyRemaining_;
    std::vector<std::uint8_t> buf_;
};
      
class ConnectionWorker : public QObject {
    Q_OBJECT

public:
    ConnectionWorker(QObject* parent);

    // This is a dirty hack so that we don't get deadlocks
    // at connection time...
    void reportOobResponses(bool enabled);

signals:
    void oobResponseReceived(std::shared_ptr<Response> response);

public slots:
    void connectToHost(QString host, int port, int timeoutMs, QPromise<bool>* resultPromise);
    void disconnect();
    void sendCommand(std::uint8_t cmd, std::vector<std::uint8_t> body, ResponseSetter* resposnseSetter);

private slots:
    void consumeBytes();

private:
    std::uint32_t readU32();
    std::uint8_t readU8();
    //void putU32(std::uint32_t val, std::uint8_t* buf);
    void handleMessage(std::vector<std::uint8_t> message);

    MessageCollector currentMessage_;

    int reportOobStopped_;
    std::vector<std::shared_ptr<Response>> unreportedOobs_;

    std::uint32_t nextID_;
    std::map<std::uint32_t, ResponseSetter*> outstandingRequests_;
    QTcpSocket socket_;

    std::vector<CheckpointInfo> checkpointInfos;
};

}
