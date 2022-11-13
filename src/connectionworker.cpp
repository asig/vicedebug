#include "connectionworker.h"

#include <iostream>

#include <QIODevice>

#include "vectorutils.h"

namespace vicedebug {

namespace {

constexpr const int kHeaderSize = 12; // Size of the Response header

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
    : QObject(parent), socket_(this)
{
    connect(&socket_, &QIODevice::readyRead, this, &ConnectionWorker::consumeBytes);
}

void ConnectionWorker::connectToHost(QString host, int port, QPromise<bool>* resultPromise) {
    std::cout << "connectToHost: " << QThread::currentThreadId() << std::endl;
    socket_.connectToHost(host, port);
    bool res = socket_.waitForConnected();
    resultPromise->addResult(res);
    resultPromise->finish();
}

void ConnectionWorker::disconnect() {
    socket_.disconnect();
}

void ConnectionWorker::sendCommand(std::uint8_t cmd, std::vector<std::uint8_t> body, ResponseSetter* responseSetter) {
    std::uint32_t id = nextID_++;
    Command command(cmd, id, body);
    std::vector<std::uint8_t> raw = command.getBuffer();

    outstandingRequests_[id] = responseSetter;

    socket_.write((char*)raw.data(), raw.size());
}

std::uint32_t ConnectionWorker::readU32() {
    std::uint32_t res;
    //    if (std::endian::native == std::endian::little) {
    //        std::uint32_t res;
    //        socket_.read((char*)&res, sizeof(res));
    //    } else {
    std::uint8_t b1 = readU8();
    std::uint8_t b2 = readU8();
    std::uint8_t b3 = readU8();
    std::uint8_t b4 = readU8();
    res = b4 << 24 | b3 << 16 | b2 << 8 | b1;
    //    }
    return res;
}

std::uint8_t ConnectionWorker::readU8() {
    std::uint8_t res;
    socket_.read((char*)&res, sizeof(res));
    return res;
}

void ConnectionWorker::putU32(std::uint8_t val, std::uint8_t* buf) {
    *(buf++) = val & 0xff;
    *(buf++) = (val >> 8) & 0xff;
    *(buf++) = (val >> 16) & 0xff;
    *(buf++) = (val >> 24) & 0xff;
}

void ConnectionWorker::consumeBytes() {
    std::cout << "Available bytes: " << socket_.bytesAvailable() << std::endl;

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
        std::cout << "BAD MAGIC!" << std::endl;
        return;
    }
    if (message[1] != 0x02) {
        std::cout << "BAD API VERSION" << std::endl;
        return;
    }
    int responseType = message[6];
    int errorCode = message[7];
    std::uint32_t bodyLength = message[5] << 24 | message[4] << 16 | message[3] << 8 | message[2];
    if (message.size() - kHeaderSize != bodyLength) {
        std::cout << "BAD BODY LENGTH" << std::endl;
        return;
    }
    std::uint32_t responseID = message[11] << 24 | message[10] << 16 | message[9] << 8 | message[8];

    std::shared_ptr<Response> r = nullptr;
    switch(responseType) {
    case RESPONSE_INVALID:
        qWarning() << "Received RESPONSE_INVALID, ignoring.";
        break;
    case RESPONSE_MEM_GET: {
        std::shared_ptr<MemGetResponse> m = std::make_shared<MemGetResponse>();
        // First 2 bytes are memory length. Safe to ignore.
        m->memory = std::vector<std::uint8_t>(message.begin() + kHeaderSize + 2, message.end());
        r = m;
        break;
    }
    case RESPONSE_REGISTER_INFO: {
        std::shared_ptr<RegistersResponse> m = std::make_shared<RegistersResponse>();
        vistream is(message.begin() + kHeaderSize);
        std::uint16_t cnt = is.readU16();
        while (cnt-- > 0) {
            std::uint8_t id = is.readU8();
            std::uint16_t val = is.readU16();
            m->values[id] = val;
        }
        r = m;
        break;
    }
    case RESPONSE_REGISTERS_AVAILABLE: {
        std::shared_ptr<RegistersAvailableResponse> m = std::make_shared<RegistersAvailableResponse>();
        vistream is(message.begin() + kHeaderSize);
        std::uint16_t cnt = is.readU16();
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
        }
        r = m;
        break;
    }
    case RESPONSE_MEM_SET:
    case RESPONSE_CHECKPOINT_INFO:
    case RESPONSE_CHECKPOINT_DELETE:
    case RESPONSE_CHECKPOINT_LIST:
    case RESPONSE_CHECKPOINT_TOGGLE:
    case RESPONSE_CONDITION_SET:
    case RESPONSE_DUMP:
    case RESPONSE_UNDUMP:
    case RESPONSE_RESOURCE_GET:
    case RESPONSE_RESOURCE_SET:
    case RESPONSE_JAM:
    case RESPONSE_STOPPED:
    case RESPONSE_RESUMED:
    case RESPONSE_ADVANCE_INSTRUCTIONS:
    case RESPONSE_KEYBOARD_FEED:
    case RESPONSE_EXECUTE_UNTIL_RETURN:
    case RESPONSE_PING:
    case RESPONSE_BANKS_AVAILABLE:
    case RESPONSE_DISPLAY_GET:
    case RESPONSE_VICE_INFO:
    case RESPONSE_PALETTE_GET:
    case RESPONSE_JOYPORT_SET:
    case RESPONSE_USERPORT_SET:
    case RESPONSE_EXIT:
    case RESPONSE_QUIT:
    case RESPONSE_RESET:
    case RESPONSE_AUTOSTART:
        qWarning() << "Received unhandled response type " << responseType << ", ignoring it.";
        break;
    default: {
        qCritical() << "Received unknown response type " << responseType << ", ignoring it.";
    }
    }

    if (r == nullptr) {
        qWarning() << "Message " << responseID << " of type " << responseType << " received, but handled";
        return;
    }

    if (responseID != -1) {
        auto handler = outstandingRequests_[responseID];
        outstandingRequests_.erase(responseID);
        if (handler != nullptr) {
            handler->set(r);
        } else {
            qWarning() << "Message " << responseID << " of type " << responseType << " is not expected by anybody!";
        }
        // Done with handler.
        delete handler;
    } else {
        emit oobResponseReceived(r);
    }
}

}
