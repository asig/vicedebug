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

#include "viceclient.h"

#include <QFuture>
#include <QEventLoop>

#include <iostream>

#include "connectionworker.h"
#include "vectorutils.h"

namespace vicedebug {

template<typename T>
class ResponseSetterImpl : public ResponseSetter {
public:
    ResponseSetterImpl(ConnectionWorker* connectionWorker, QPromise<T>* promise)
        : promise_(promise), connectionWorker_(connectionWorker), complete_(false) {}

    void set(std::shared_ptr<Response> response) override {
        qDebug() << QThread::currentThreadId() << "Adding Result to response";
        promise_->addResult(*((T*)response.get()));
        promise_->finish();
        qDebug() << QThread::currentThreadId() << "Promise finished";
        delete promise_;
        promise_ = nullptr;
        connectionWorker_->reportOobResponses(true);
        complete_ = true;
    }

    bool responseIsComplete() override {
        return complete_;
    }

private:
    QPromise<T>* promise_;
    ConnectionWorker* connectionWorker_;
    bool complete_;
};

class CheckpointListResponseSetterImpl : public ResponseSetter {
public:
    CheckpointListResponseSetterImpl(ConnectionWorker* connectionWorker, QPromise<CheckpointListResponse>* promise)
        : promise_(promise), connectionWorker_(connectionWorker), complete_(false) {
    }

    void set(std::shared_ptr<Response> response) override {
        switch(response->responseType) {
        case RESPONSE_CHECKPOINT_INFO: {
            qDebug() << QThread::currentThreadId() << "Adding checkpoint info to response";
            auto r = std::reinterpret_pointer_cast<CheckpointInfoResponse>(response);
            checkpoints_.push_back(r->checkpoint);
            break;
        }
        case RESPONSE_CHECKPOINT_LIST: {
            qDebug() << QThread::currentThreadId() << "Finishing checkpoint list response";
            auto r = std::reinterpret_pointer_cast<CheckpointListResponse>(response);
            r->checkpoints = checkpoints_;
            promise_->addResult(*r);
            promise_->finish();
            qDebug() << QThread::currentThreadId() << "Promise finished";
            delete promise_;
            promise_ = nullptr;
            connectionWorker_->reportOobResponses(true);
            complete_ = true;
            break;
        }
        default:
            qDebug() << "Unexpected response " << response->responseType;
        }
    }

    bool responseIsComplete() override {
        return complete_;
    }

private:
    QPromise<CheckpointListResponse>* promise_;
    std::vector<CheckpointInfo> checkpoints_;
    ConnectionWorker* connectionWorker_;
    bool complete_;
};

ViceClient::ViceClient(QObject* parent)
    : QObject(parent)
{   
    connectionWorker_ = new ConnectionWorker(nullptr);
    connectionWorker_->moveToThread(&connectionWorkerThread_);

    connect(&connectionWorkerThread_, &QThread::finished, connectionWorker_, &QObject::deleteLater);
    //    auto res = connect(worker, &ConnectionWorker::responseReceived, this, &ViceClient::handleResponse);
    //    std::cout << "************************ res == " << res << std::endl;
    //    auto res = worker->connect(worker, &ConnectionWorker::responseReceived, this, &ViceClient::handleResponse,Qt::QueuedConnection);
    //    std::cout << "************************ res == " << res << std::endl;

    connect(this, &ViceClient::connectionRequested, connectionWorker_, &ConnectionWorker::connectToHost);
    connect(this, &ViceClient::disconnectRequested, connectionWorker_, &ConnectionWorker::disconnect);
    connect(this, &ViceClient::sendCommand, connectionWorker_, &ConnectionWorker::sendCommand);

    connect(connectionWorker_, &ConnectionWorker::oobResponseReceived, this, &ViceClient::onOobResponseReceived);

    connectionWorkerThread_.start();
}

ViceClient::~ViceClient() {
    connectionWorkerThread_.quit();
    connectionWorkerThread_.wait();
}

bool ViceClient::connectToVice(const QString& host, int port, int timeoutMs) {
    QPromise<bool> resultPromise;
    QFuture<bool> res = resultPromise.future();
    emit connectionRequested(host, port, timeoutMs, &resultPromise);
    res.waitForFinished();
    return res.result();
}

void ViceClient::disconnect() {
    emit disconnectRequested();
}

QFuture<MemGetResponse> ViceClient::memGet(uint16_t startAddress, uint16_t endAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects ) {
    auto promise = new QPromise<MemGetResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<MemGetResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)(sideEffects ? 0x01 : 0x00)
         << startAddress
         << endAddress
         << (std::uint8_t)memSpace
         << bankID;
    emit sendCommand(CMD_MEM_GET, body, responseSetter);

    return res;
}

QFuture<MemSetResponse> ViceClient::memSet(uint16_t startAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects, std::vector<std::uint8_t> data ) {
    auto promise = new QPromise<MemSetResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<MemSetResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body.reserve(8 + data.size()); // 8 bytes header + data
    body << (std::uint8_t)(sideEffects ? 0x01 : 0x00)
         << startAddress
         << (std::uint16_t)(startAddress + data.size() - 1)
         << (std::uint8_t)memSpace
         << bankID;
    body.insert(body.end(), data.begin(), data.end());

    emit sendCommand(CMD_MEM_SET, body, responseSetter);

    return res;
}

QFuture<RegistersResponse> ViceClient::registersGet(MemSpace memSpace) {
    auto promise = new QPromise<RegistersResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<RegistersResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)memSpace;
    emit sendCommand(CMD_REGISTERS_GET, body, responseSetter);

    return res;
}

QFuture<RegistersResponse> ViceClient::registersSet(MemSpace memSpace, std::map<std::uint8_t, std::uint16_t> values) {
    auto promise = new QPromise<RegistersResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<RegistersResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)memSpace
         << (std::uint16_t)values.size();
    for (auto p : values) {
        body << (std::uint8_t)3 // Size of the item, excluding this byte
             << (std::uint8_t)p.first // Register ID
             << (std::uint16_t)p.second; // Register Value
    }
    emit sendCommand(CMD_REGISTERS_SET, body, responseSetter);
    return res;
}

QFuture<RegistersAvailableResponse> ViceClient::registersAvailable(MemSpace memSpace) {
    auto promise = new QPromise<RegistersAvailableResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<RegistersAvailableResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)memSpace;
    emit sendCommand(CMD_REGISTERS_AVAILABLE, body, responseSetter);

    return res;
}

QFuture<CheckpointListResponse> ViceClient::checkpointList() {
    auto promise = new QPromise<CheckpointListResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new CheckpointListResponseSetterImpl(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    emit sendCommand(CMD_CHECKPOINT_LIST, body, responseSetter);

    return res;
}

QFuture<CheckpointDeleteResponse> ViceClient::checkpointDelete(std::uint32_t number) {
    auto promise = new QPromise<CheckpointDeleteResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<CheckpointDeleteResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << number;
    emit sendCommand(CMD_CHECKPOINT_DELETE, body, responseSetter);

    return res;
}

QFuture<CheckpointToggleResponse> ViceClient::checkpointToggle(std::uint32_t number, bool enabled) {
    auto promise = new QPromise<CheckpointToggleResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<CheckpointToggleResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << number
         << (std::uint8_t)enabled;
    emit sendCommand(CMD_CHECKPOINT_TOGGLE, body, responseSetter);

    return res;
}

QFuture<CheckpointInfoResponse> ViceClient::checkpointGet(std::uint32_t number) {
    auto promise = new QPromise<CheckpointInfoResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<CheckpointInfoResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << number;
    emit sendCommand(CMD_CHECKPOINT_GET, body, responseSetter);

    return res;
}

QFuture<CheckpointInfoResponse> ViceClient::checkpointSet(std::uint16_t startAddr, std::uint16_t endAddr, bool stopWhenHit, bool enabled, std::uint8_t op, bool temporary, MemSpace memSpace) {
    auto promise = new QPromise<CheckpointInfoResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<CheckpointInfoResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << startAddr
         << endAddr
         << (std::uint8_t)stopWhenHit
         << (std::uint8_t)enabled
         << op
         << (std::uint8_t)temporary
         << (std::uint8_t)memSpace;
    emit sendCommand(CMD_CHECKPOINT_SET, body, responseSetter);

    return res;
}

QFuture<AdvanceInstructionsResponse> ViceClient::advanceInstructions(bool stepOverSubroutines, int nofInstructions) {
    auto promise = new QPromise<AdvanceInstructionsResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<AdvanceInstructionsResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)stepOverSubroutines
         << (std::uint16_t)nofInstructions;
    emit sendCommand(CMD_ADVANCE_INSTRUCTIONS, body, responseSetter);

    return res;
}

QFuture<ExecuteUntilReturnResponse> ViceClient::executeUntilReturn() {
    auto promise = new QPromise<ExecuteUntilReturnResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<ExecuteUntilReturnResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    emit sendCommand(CMD_EXECUTE_UNTIL_RETURN, body, responseSetter);

    return res;
}

QFuture<BanksAvailableResponse> ViceClient::banksAvailable() {
    auto promise = new QPromise<BanksAvailableResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<BanksAvailableResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    emit sendCommand(CMD_BANKS_AVAILABLE, body, responseSetter);

    return res;
}

QFuture<ExitResponse> ViceClient::exit() {
    auto promise = new QPromise<ExitResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<ExitResponse>(connectionWorker_, promise);

    std::vector<std::uint8_t> body;
    emit sendCommand(CMD_EXIT, body, responseSetter);

    return res;
}

void ViceClient::onOobResponseReceived(std::shared_ptr<Response> r) {
    qDebug() << "OOB Response received: " << r->responseType;
    switch(r->responseType) {
    case RESPONSE_STOPPED: {
        auto sr = std::reinterpret_pointer_cast<StoppedResponse>(r);
        emit stoppedResponseReceived(sr->pc);
        break;
    }
    case RESPONSE_RESUMED: {
        auto sr = std::reinterpret_pointer_cast<ResumedResponse>(r);
        emit resumedResponseReceived(sr->pc);
        break;
    }
    default:
        qDebug() << "OOB Reponse " << r->responseType << "ignored.";
    }
}

}
