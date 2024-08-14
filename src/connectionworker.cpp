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

#include "connectionworker.h"

#include <iostream>

#include <QIODevice>

#include "vectorutils.h"

namespace vicedebug {

namespace {

constexpr const int kHeaderSize = 12; // Size of the Response header

std::map<int, const char*> kResponseTypeNames = {
    { RESPONSE_INVALID, "RESPONSE_INVALID" },
    { RESPONSE_MEM_GET, "RESPONSE_MEM_GET" },
    { RESPONSE_MEM_SET, "RESPONSE_MEM_SET" },
    { RESPONSE_CHECKPOINT_INFO, "RESPONSE_CHECKPOINT_INFO" },
    { RESPONSE_CHECKPOINT_DELETE, "RESPONSE_CHECKPOINT_DELETE" },
    { RESPONSE_CHECKPOINT_LIST, "RESPONSE_CHECKPOINT_LIST" },
    { RESPONSE_CHECKPOINT_TOGGLE, "RESPONSE_CHECKPOINT_TOGGLE" },
    { RESPONSE_CONDITION_SET, "RESPONSE_CONDITION_SET" },
    { RESPONSE_REGISTER_INFO, "RESPONSE_REGISTER_INFO" },
    { RESPONSE_DUMP, "RESPONSE_DUMP" },
    { RESPONSE_UNDUMP, "RESPONSE_UNDUMP" },
    { RESPONSE_RESOURCE_GET, "RESPONSE_RESOURCE_GET" },
    { RESPONSE_RESOURCE_SET, "RESPONSE_RESOURCE_SET" },
    { RESPONSE_JAM, "RESPONSE_JAM" },
    { RESPONSE_STOPPED, "RESPONSE_STOPPED" },
    { RESPONSE_RESUMED, "RESPONSE_RESUMED" },
    { RESPONSE_ADVANCE_INSTRUCTIONS, "RESPONSE_ADVANCE_INSTRUCTIONS" },
    { RESPONSE_KEYBOARD_FEED, "RESPONSE_KEYBOARD_FEED" },
    { RESPONSE_EXECUTE_UNTIL_RETURN, "RESPONSE_EXECUTE_UNTIL_RETURN" },
    { RESPONSE_PING, "RESPONSE_PING" },
    { RESPONSE_BANKS_AVAILABLE, "RESPONSE_BANKS_AVAILABLE" },
    { RESPONSE_REGISTERS_AVAILABLE, "RESPONSE_REGISTERS_AVAILABLE" },
    { RESPONSE_DISPLAY_GET, "RESPONSE_DISPLAY_GET" },
    { RESPONSE_VICE_INFO, "RESPONSE_VICE_INFO" },
    { RESPONSE_PALETTE_GET, "RESPONSE_PALETTE_GET" },
    { RESPONSE_JOYPORT_SET, "RESPONSE_JOYPORT_SET" },
    { RESPONSE_USERPORT_SET, "RESPONSE_USERPORT_SET" },
    { RESPONSE_EXIT, "RESPONSE_EXIT" },
    { RESPONSE_QUIT, "RESPONSE_QUIT" },
    { RESPONSE_RESET, "RESPONSE_RESET" },
    { RESPONSE_AUTOSTART, "RESPONSE_AUTOSTART" }
};

}

class Command {
public:
    Command(std::uint8_t cmd, std::uint32_t id, std::vector<std::uint8_t> body) : cmd_(cmd), id_(id), body_(body) {}

    std::vector<std::uint8_t> getBuffer() {
        std::vector<std::uint8_t> msg;
        msg << (std::uint8_t)0x02 // Binary request marker
            << (std::uint8_t)0x02 // API version
            << (std::uint32_t)body_.size()
            << id_
            << cmd_;
        msg.insert(msg.end(), body_.begin(), body_.end());
        return msg;
    }

private:
    uint8_t cmd_;
    uint32_t id_;
    std::vector<std::uint8_t> body_;
};



ConnectionWorker::ConnectionWorker(QObject* parent)
    : QObject(parent), socket_(this), reportOobStopped_(0)
{
    connect(&socket_, &QIODevice::readyRead, this, &ConnectionWorker::consumeBytes);
}

void ConnectionWorker::connectToHost(QString host, int port, int timeoutMs, QPromise<bool>* resultPromise) {
    socket_.connectToHost(host, port);
    bool res = socket_.waitForConnected(timeoutMs);
    resultPromise->addResult(res);
    resultPromise->finish();
}

void ConnectionWorker::disconnect() {
    socket_.disconnectFromHost();
    socket_.waitForDisconnected();
}

void ConnectionWorker::sendCommand(std::uint8_t cmd, std::vector<std::uint8_t> body, ResponseSetter* responseSetter) {
    std::uint32_t id = nextID_++;
    Command command(cmd, id, body);
    std::vector<std::uint8_t> raw = command.getBuffer();

    reportOobResponses(false); // will be reenabled in the reponsesetter
    outstandingRequests_[id] = responseSetter;

    socket_.write((char*)raw.data(), raw.size());
}

std::uint32_t ConnectionWorker::readU32() {
    std::uint32_t res;
    std::uint8_t b1 = readU8();
    std::uint8_t b2 = readU8();
    std::uint8_t b3 = readU8();
    std::uint8_t b4 = readU8();
    res = b4 << 24 | b3 << 16 | b2 << 8 | b1;
    return res;
}

std::uint8_t ConnectionWorker::readU8() {
    std::uint8_t res;
    socket_.read((char*)&res, sizeof(res));
    return res;
}

void ConnectionWorker::consumeBytes() {
    qDebug() << "Available bytes: " << socket_.bytesAvailable();

    while (socket_.bytesAvailable() > 0) {
        std::uint8_t b = readU8();
        currentMessage_.append(b);
        if (currentMessage_.remaining() == 0) {
            handleMessage(currentMessage_.data());
            currentMessage_.reset();
        }
    }
}

void ConnectionWorker::handleMessage(std::vector<std::uint8_t> message) {

    // check whether we understand the message.
    if (message[0] != 0x02) {
        // Bad magic
        qCritical() << "BAD MAGIC!";
        return;
    }
    if (message[1] != 0x02) {
        qCritical() << "BAD API VERSION";
        return;
    }
    int responseType = message[6];
    int errorCode = message[7];
    std::uint32_t bodyLength = message[5] << 24 | message[4] << 16 | message[3] << 8 | message[2];
    if (message.size() - kHeaderSize != bodyLength) {
        qCritical() << "BAD BODY LENGTH";
        return;
    }
    std::uint32_t responseID = message[11] << 24 | message[10] << 16 | message[9] << 8 | message[8];

    std::shared_ptr<Response> r = nullptr;
    qDebug() << QThread::currentThreadId() << "Received " << kResponseTypeNames[responseType] << "(" << responseType << ")";
    switch(responseType) {
    case RESPONSE_INVALID:
        break;
    case RESPONSE_MEM_GET: {
        std::shared_ptr<MemGetResponse> m = std::make_shared<MemGetResponse>();
        // First 2 bytes are memory length. Safe to ignore.
        m->memory = std::vector<std::uint8_t>(message.begin() + kHeaderSize + 2, message.end());
        r = m;
        break;
    }
    case RESPONSE_MEM_SET: {
        std::shared_ptr<MemSetResponse> m = std::make_shared<MemSetResponse>();
        r = m;
        break;
    }
    case RESPONSE_REGISTER_INFO: {
        qDebug() << "Received RESPONSE_REGISTER_INFO (" << RESPONSE_REGISTER_INFO << "):";
        std::shared_ptr<RegistersResponse> m = std::make_shared<RegistersResponse>();
        vistream is(message.begin() + kHeaderSize);
        std::uint16_t cnt = is.readU16();
        qDebug() << "  " << cnt << " registers.";
        while (cnt-- > 0) {
            std::uint8_t len = is.readU8();
            std::uint8_t id = is.readU8();
            std::uint16_t val = is.readU16();
            m->values[id] = val;
            qDebug() << "  register " << id << " == " << val;
        }
        r = m;
        break;
    }
    case RESPONSE_REGISTERS_AVAILABLE: {
        std::shared_ptr<RegistersAvailableResponse> m = std::make_shared<RegistersAvailableResponse>();
        vistream is(message.begin() + kHeaderSize);
        std::uint16_t cnt = is.readU16();
        qDebug() << "  " << cnt << " registers.";
        while (cnt-- > 0) {
            std::uint8_t len = is.readU8();
            std::uint8_t id = is.readU8();
            std::uint8_t bits = is.readU8();
            std::string name;
            std::uint8_t nameLen = is.readU8();
            while (nameLen-- > 0) {
                name += is.readU8();
            }
            m->regInfos[id] = RegInfo{bits, name};
            qDebug() << "  register " << id << ": name == " << name.c_str() << ", bits == " << bits;
        }
        r = m;
        break;
    }
    case RESPONSE_CHECKPOINT_INFO: {
        qDebug() << "Received RESPONSE_CHECKPOINT_INFO (" << RESPONSE_CHECKPOINT_INFO << "):";
        std::shared_ptr<CheckpointInfoResponse> m = std::make_shared<CheckpointInfoResponse>();
        vistream is(message.begin() + kHeaderSize);
        m->checkpoint.number = is.readU32();
        m->checkpoint.hit = is.readU8();
        m->checkpoint.startAddress = is.readU16();
        m->checkpoint.endAddress = is.readU16();
        m->checkpoint.stopWhenHit = is.readU8();
        m->checkpoint.enabled = is.readU8();
        m->checkpoint.op = is.readU8(); // 0x01: load, 0x02: store, 0x04: exec
        m->checkpoint.isTemporary = is.readU8();
        m->checkpoint.hitCount = is.readU32();
        m->checkpoint.ignoreCount = is.readU32();
        m->checkpoint.hasCondition = is.readU8();
        m->checkpoint.memspace = is.readU8();
        r = m;
        break;
    }
    case RESPONSE_CHECKPOINT_LIST: {
        std::shared_ptr<CheckpointListResponse> m = std::make_shared<CheckpointListResponse>();
        vistream is(message.begin() + kHeaderSize);
        m->nofCheckpoints = is.readU32();
        r = m;
        break;
    }
    case RESPONSE_CHECKPOINT_DELETE: {
        std::shared_ptr<CheckpointDeleteResponse> m = std::make_shared<CheckpointDeleteResponse>();
        r = m;
        break;
    }
    case RESPONSE_CHECKPOINT_TOGGLE: {
        std::shared_ptr<CheckpointToggleResponse> m = std::make_shared<CheckpointToggleResponse>();
        r = m;
        break;
    }
    case RESPONSE_ADVANCE_INSTRUCTIONS: {
        std::shared_ptr<AdvanceInstructionsResponse> m = std::make_shared<AdvanceInstructionsResponse>();
        r = m;
        break;
    }
    case RESPONSE_EXIT: {
        std::shared_ptr<ExitResponse> m = std::make_shared<ExitResponse>();
        r = m;
        break;
    }
    case RESPONSE_EXECUTE_UNTIL_RETURN: {
        std::shared_ptr<ExecuteUntilReturnResponse> m = std::make_shared<ExecuteUntilReturnResponse>();
        r = m;
        break;
    }
    case RESPONSE_STOPPED: {
        std::shared_ptr<StoppedResponse> m = std::make_shared<StoppedResponse>();
        vistream is(message.begin() + kHeaderSize);
        m->pc = is.readU16();
        r = m;
        break;
    }
    case RESPONSE_RESUMED: {
        std::shared_ptr<ResumedResponse> m = std::make_shared<ResumedResponse>();
        vistream is(message.begin() + kHeaderSize);
        m->pc = is.readU16();
        r = m;
        break;
    }
    case RESPONSE_BANKS_AVAILABLE: {
        std::shared_ptr<BanksAvailableResponse> m = std::make_shared<BanksAvailableResponse>();
        vistream is(message.begin() + kHeaderSize);
        std::uint16_t cnt = is.readU16();
        qDebug() << "  " << cnt << " banks.";
        while (cnt-- > 0) {
            std::uint8_t len = is.readU8();
            std::uint16_t id = is.readU16();
            std::string name;
            std::uint8_t nameLen = is.readU8();
            while (nameLen-- > 0) {
                name += is.readU8();
            }
            m->banks[id] = name;
            qDebug() << "  bank " << id << ": " << name.c_str();
        }
        r = m;
        break;
    }
    case RESPONSE_CONDITION_SET:
    case RESPONSE_DUMP:
    case RESPONSE_UNDUMP:
    case RESPONSE_RESOURCE_GET:
    case RESPONSE_RESOURCE_SET:
    case RESPONSE_JAM:
    case RESPONSE_KEYBOARD_FEED:
    case RESPONSE_PING:
    case RESPONSE_DISPLAY_GET:
    case RESPONSE_VICE_INFO:
    case RESPONSE_PALETTE_GET:
    case RESPONSE_JOYPORT_SET:
    case RESPONSE_USERPORT_SET:
    case RESPONSE_QUIT:
    case RESPONSE_RESET:
    case RESPONSE_AUTOSTART:
    default: {
        qCritical() << "Received unknown response type " << responseType << ", ignoring it.";
    }
    }
    qDebug() << QThread::currentThreadId() << "Done receiveing " << kResponseTypeNames[responseType] << "(" << responseType << ")";

    if (r == nullptr) {
        qWarning() << "Message " << responseID << " of type " << responseType << " received, but handled";
        return;
    }

    r->responseType = (ResponseType)responseType;

    if (responseID != -1) {
        qDebug() << QThread::currentThreadId() << "Going to call " << kResponseTypeNames[responseType] << " handler for responseID " << responseID;
        auto handler = outstandingRequests_[responseID];

        if (handler != nullptr) {
            qDebug() << QThread::currentThreadId() << "Calling " << kResponseTypeNames[responseType] << " handler for responseID " << responseID;
            handler->set(r);
            if (handler->responseIsComplete()) {
                outstandingRequests_.erase(responseID);
                delete handler;
            }
        } else {
            qWarning() << "Message " << responseID << " of type " << responseType << " is not expected by anybody!";
        }
    } else {
        if (reportOobStopped_ > 0) {
            unreportedOobs_.push_back(r);
        } else {
            emit oobResponseReceived(r);
        }
    }
}

void ConnectionWorker::reportOobResponses(bool enabled) {
    if (enabled) {
        if (--reportOobStopped_ == 0) {
            for (auto r : unreportedOobs_) {
                emit oobResponseReceived(r);
            }
            unreportedOobs_.clear();
        }
    } else {
        reportOobStopped_++;
    }
}

}
