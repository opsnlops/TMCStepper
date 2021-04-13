#pragma once

#include <cstdint>
#include <cstddef>
#include <deque>
#include <array>

#include <unity.h>

#include <iostream>

#define SW_CAPABLE_PLATFORM false

namespace TMC_HAL {
    using PinDef = uint8_t;

    struct PinCache {
        explicit PinCache(const PinDef _pin) :
            pin(_pin)
            {}
        const PinDef pin;
    };
}

#include <cstdio>

struct SPIClass {
    explicit SPIClass(uint32_t initValue = 0) : registerValue(initValue) {  }

    struct Payload {
        Payload(const uint_least8_t a, const uint_least32_t d) : address(a), data(d) {}
        uint_fast8_t address;
        uint_fast32_t data;

        bool operator ==(const Payload rhs) const {
            if (rhs.address == address && rhs.data == data) {
                return true;
            }
            else {
                return false;
            }
        }
    };

    friend std::ostream& operator<< (std::ostream &out, const SPIClass &cmd);

    operator const char*() {
        int_least16_t n = 0;

        for (auto cmd : sentCommands) {
            n += sprintf(&printout[n], "{0x%02X,0x%08X} ", cmd.address, cmd.data);

            if (n > sizeof(printout)-20) break;
        }

        return printout;
    }

    std::deque<Payload> sentCommands;
    bool active = false;
    int transferCalls = 0;

    uint32_t registerValue;

    char printout[256] = {0};
};

struct HardwareSerial {
    HardwareSerial(void *const handle) {}

    void write(const uint8_t *data, uint8_t length) {}
    void read(uint8_t *buf, uint8_t length) {}
    uint8_t available() { return 0; }
};

inline void delay(int) {}