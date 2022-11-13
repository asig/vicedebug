#include "viceclient.h"

#include <QFuture>
#include <QEventLoop>

#include <iostream>

//#include <bit>

#include "connectionworker.h"
#include "vectorutils.h"

namespace vicedebug {


ViceClient::ViceClient(QObject* parent)
    : QObject(parent)
{   
    ConnectionWorker* worker = new ConnectionWorker(nullptr);
    worker->moveToThread(&connectionWorkerThread_);

    connect(&connectionWorkerThread_, &QThread::finished, worker, &QObject::deleteLater);
//    auto res = connect(worker, &ConnectionWorker::responseReceived, this, &ViceClient::handleResponse);
//    std::cout << "************************ res == " << res << std::endl;
//    auto res = worker->connect(worker, &ConnectionWorker::responseReceived, this, &ViceClient::handleResponse,Qt::QueuedConnection);
//    std::cout << "************************ res == " << res << std::endl;

    connect(this, &ViceClient::connectionRequested, worker, &ConnectionWorker::connectToHost);
    connect(this, &ViceClient::disconnectRequested, worker, &ConnectionWorker::disconnect);
    connect(this, &ViceClient::sendCommand, worker, &ConnectionWorker::sendCommand);

    connectionWorkerThread_.start();
}

ViceClient::~ViceClient() {
    connectionWorkerThread_.quit();
    connectionWorkerThread_.wait();
}

bool ViceClient::connectToVice(const QString& host, int port) {
    std::cout << "connectToVice: " << QThread::currentThreadId() << std::endl;
    std::cout << "connectToVice: " << thread() << std::endl;
    QPromise<bool> resultPromise;
    QFuture<bool> res = resultPromise.future();
    emit connectionRequested(host, port, &resultPromise);
    res.waitForFinished();
    return res.result();
}

void ViceClient::disconnect() {
    emit disconnectRequested();
}

QFuture<MemGetResponse> ViceClient::memGet(uint16_t startAddress, uint16_t endAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects ) {
    auto promise = new QPromise<MemGetResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<MemGetResponse>(promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)(sideEffects ? 0x01 : 0x00)
         << startAddress
         << endAddress
         << (std::uint8_t)memSpace
         << bankID;
    emit sendCommand(CMD_MEM_GET, body, responseSetter);

    return res;
}

QFuture<RegistersResponse> ViceClient::registersGet(MemSpace memSpace) {
    auto promise = new QPromise<RegistersResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<RegistersResponse>(promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)memSpace;
    emit sendCommand(CMD_REGISTERS_GET, body, responseSetter);

    return res;
}

QFuture<RegistersAvailableResponse> ViceClient::registersAvailable(MemSpace memSpace) {
    auto promise = new QPromise<RegistersAvailableResponse>();
    auto res = promise->future();
    ResponseSetter* responseSetter = new ResponseSetterImpl<RegistersAvailableResponse>(promise);

    std::vector<std::uint8_t> body;
    body << (std::uint8_t)memSpace;
    emit sendCommand(CMD_REGISTERS_AVAILABLE, body, responseSetter);

    return res;
}


}
