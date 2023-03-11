//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_PACKETHEADER_H
#define NETJUCE_TEENSY_PACKETHEADER_H

#include <Arduino.h>

enum BitResolutionT {
    BIT8 = 1,
    BIT16 = 2,
    BIT24 = 3,
    BIT32 = 4
};

enum SamplingRateT {
    SR22,
    SR32,
    SR44,
    SR48,
    SR88,
    SR96,
    SR192,
    UNDEF
};

struct PacketHeader {//: public Printable {
public:
    uint16_t SeqNumber;
    uint8_t BufferSize;
    uint8_t SamplingRate;
    uint8_t BitResolution;
    uint8_t NumChannels;

//    size_t printTo(Print &p) const override {
//        p.printf("SeqNumber: %d, BufferSize: %d, NumChannels: %d", SeqNumber, BufferSize, NumChannels);
//        return 0;
//    }
};

#define PACKET_HEADER_SIZE sizeof (PacketHeader)

#endif //NETJUCE_TEENSY_PACKETHEADER_H
