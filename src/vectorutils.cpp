#include "vectorutils.h"

namespace vicedebug {

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint8_t val) {
    vec.push_back(val);
    return vec;
}

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint16_t val) {
    vec.push_back(val & 0xff);
    vec.push_back((val >> 8) & 0xff);
    return vec;
}

std::vector<std::uint8_t>& operator<<(std::vector<std::uint8_t>& vec, std::uint32_t val) {
    vec.push_back(val & 0xff);
    vec.push_back((val >> 8) & 0xff);
    vec.push_back((val >> 16) & 0xff);
    vec.push_back((val >> 24) & 0xff);
    return vec;
}

}

