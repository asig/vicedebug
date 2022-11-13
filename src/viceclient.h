#pragma once

#include <QString>
#include <QTcpSocket>
#include <QPromise>
#include <QWaitCondition>
#include <QThread>

#include "connectionworker.h"

namespace vicedebug {

enum MemSpace {
    MAIN_MEMORY,
    DRIVE8,
    DRIVE9,
    DRIVE10,
    DRIVE11
};

class ViceClient : public QObject {

    Q_OBJECT
public:
    ViceClient(QObject* parent);
    ~ViceClient();

    bool connectToVice(const QString& host, int port);
    void disconnect();

    QFuture<MemGetResponse> memGet(uint16_t startAddress, uint16_t endAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects);
    QFuture<RegistersResponse> registersGet(MemSpace memSpace);
    QFuture<RegistersAvailableResponse> registersAvailable(MemSpace memSpace);

signals:
    void sendCommand(std::uint8_t cmd, std::vector<std::uint8_t> body, ResponseSetter* responseSetter);
    void connectionRequested(QString host, int port, QPromise<bool>* resultPromise);
    void disconnectRequested();

//private slots:
//    void handleResponse(std::uint8_t responseType, std::uint8_t errCode, std::uint32_t requestId, QByteArray body);

private:
    QThread connectionWorkerThread_;
};

}
