#include "machinestate.h"

namespace vicedebug {

std::string cpuName(Cpu cpu) {
    switch(cpu) {
    case kCpu6502: return "6502";
    case kCpuZ80: return "Z80";
    default: return "<unknown>";
    }
}

}
