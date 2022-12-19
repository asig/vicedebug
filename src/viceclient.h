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

    bool connectToVice(const QString& host, int port, int timeoutMs);
    void disconnect();

    QFuture<CheckpointListResponse> checkpointList();
    QFuture<CheckpointDeleteResponse> checkpointDelete(std::uint32_t number);
    QFuture<CheckpointToggleResponse> checkpointToggle(std::uint32_t number, bool enabled);
    QFuture<CheckpointInfoResponse> checkpointGet(std::uint32_t number);
    QFuture<CheckpointInfoResponse> checkpointSet(std::uint16_t startAddr, std::uint16_t endAddr, bool stopWhenHit, bool enabled, std::uint8_t op, bool temporary, MemSpace memSpace);

    QFuture<MemGetResponse> memGet(uint16_t startAddress, uint16_t endAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects);
    QFuture<MemSetResponse> memSet(uint16_t startAddress, MemSpace memSpace , uint16_t bankID, bool sideEffects, std::vector<std::uint8_t> data );
    QFuture<RegistersResponse> registersGet(MemSpace memSpace);
    QFuture<RegistersResponse> registersSet(MemSpace memSpace, std::map<std::uint8_t, std::uint16_t> values);

    QFuture<RegistersAvailableResponse> registersAvailable(MemSpace memSpace);

    QFuture<AdvanceInstructionsResponse> advanceInstructions(bool stepOverSubroutines, int nofInstructions);
    QFuture<ExecuteUntilReturnResponse> executeUntilReturn();

    QFuture<BanksAvailableResponse> banksAvailable();

    QFuture<ExitResponse> exit();

signals:
    // Signals to communicate with connection worker.
    void sendCommand(std::uint8_t cmd, std::vector<std::uint8_t> body, ResponseSetter* responseSetter);
    void connectionRequested(QString host, int port, int timeoutMs, QPromise<bool>* resultPromise);
    void disconnectRequested();

    // Signals that propagate OOB messages
    void stoppedResponseReceived(std::uint16_t pc);

private slots:
    void onOobResponseReceived(std::shared_ptr<Response> r);

//private slots:
//    void handleResponse(std::uint8_t responseType, std::uint8_t errCode, std::uint32_t requestId, QByteArray body);

private:
    QThread connectionWorkerThread_;
    ConnectionWorker* connectionWorker_;
};

}
