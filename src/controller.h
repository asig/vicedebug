#pragma once

#include <QString>

#include "viceclient.h"
#include "machinestate.h"

namespace vicedebug {

enum EventType {
    CONNECTION_EVENT,
    MACHINE_STATE,
    BREAKPOINT_HIT
};

struct Event {
    EventType type;

    bool connected; // Only set for CONNECTION_EVENT
    MachineState machineState; // Set for MACHINE_STATE and BREAKPOINT_HIT
};

typedef std::function<void(const Event& event)> EventListener;

class Controller : public QObject {
    Q_OBJECT

public:
    Controller(ViceClient* viceClient);

    void connect(QString host, int port);

    void addListener(const EventListener* listener);
    void removeListener(const EventListener* listener);

signals:
    void publishEvent(const Event& event);

private:
    bool connected_;
    ViceClient viceClient_;

    std::vector<const EventListener*> listeners_;
};

}
