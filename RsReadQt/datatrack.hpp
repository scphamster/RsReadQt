#pragma once

#include <utility>

struct DTWordField {
    enum {
        UNDEFINED = -1
    };

    enum class BitOrder {
        UNDEFINED = -1,
        NORMAL    = 0,
        REVERSE   = 1
    };

    enum class ByteOrder {
        UNDEFINED = -1,
        NORMAL,
        REVERSE
    };

    enum class DataFormat {
        UNDEFINED = -1,
        BIN       = 0,
        DEC,
        OCT,
        HEX,
        BCD
    };

    int                 length     = UNDEFINED;
    BitOrder            bitOrder   = BitOrder ::UNDEFINED;
    ByteOrder           byteOrder  = ByteOrder::UNDEFINED;
    std::pair<int, int> activeBits = std::pair<int, int>{ UNDEFINED, UNDEFINED };
    DataFormat          dataFormat = DataFormat::UNDEFINED;

    static uint8_t reverseBitsInByte(uint8_t data)
    {
        data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
        data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
        data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
        return data;
    }
};
