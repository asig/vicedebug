#include "controller.h"

#include <QFuture>

namespace vicedebug {

namespace {
// Constants should not be hardcoded...
constexpr const int kRegAId = 0;
constexpr const int kRegXId = 1;
constexpr const int kRegYId = 2;
constexpr const int kRegPCId = 3;
constexpr const int kRegSPId = 4;
constexpr const int kRegFlagsId = 5;
}

Controller::Controller(ViceClient* viceClient)
    : viceClient_(viceClient),
      connected_(false)
{}

void Controller::connect(QString host, int port) {
    if (connected_) {
        return;
    }

    connected_ = viceClient_.connectToVice(host, port);

    // Get memory and registers
    auto getMemResponseFuture = viceClient_.memGet(0, 0xffff, MemSpace::MAIN_MEMORY, 0, false);
    getMemResponseFuture.waitForFinished();
    auto getMemResponse = getMemResponseFuture.result();

    auto registersResponseFuture = viceClient_.registersGet(MemSpace::MAIN_MEMORY);
    registersResponseFuture.waitForFinished();
    auto registersResponse = registersResponseFuture.result();

    Event evt;
    evt.connected = connected_;
    evt.machineState.memory = getMemResponse.memory;
    evt.machineState.regA = registersResponse.values[kRegAId];
    evt.machineState.regX = registersResponse.values[kRegXId];
    evt.machineState.regY = registersResponse.values[kRegYId];
    evt.machineState.flags = registersResponse.values[kRegFlagsId];
    evt.machineState.pc = registersResponse.values[kRegPCId];
    evt.machineState.sp = registersResponse.values[kRegSPId];

    emit publishEvent(evt);
}

//void Controller::addListener(const EventListener* listener) {
//    listeners_.push_back(listener);
//}

//void Controller::removeListener(const EventListener* listener) {
//    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
//    listeners_.erase(it);
//}

//void Controller::publishEvent(const Event& event) {
//    for(auto h : listeners_) {
//        (*h)(event);
//    }
//}


}
