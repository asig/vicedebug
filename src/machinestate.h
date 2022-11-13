#pragma once

#include <vector>
#include <cstdint>

namespace vicedebug {

struct MachineState {
    std::vector<std::uint8_t> memory;

    std::uint8_t regA;
    std::uint8_t regX;
    std::uint8_t regY;
    std::uint8_t flags;
    std::uint16_t pc;
    std::uint16_t sp;
};

}
