//
// Created by tar on 3/13/23.
//

#ifndef NETJUCE_TEENSY_DATAGRAMAUDIOPACKET_H
#define NETJUCE_TEENSY_DATAGRAMAUDIOPACKET_H

#include <Arduino.h>
#include <cstdlib>
#include <IPAddress.h>

class DatagramAudioPacket {
public:
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

    struct Origin {
        IPAddress IP;
        uint16_t Port;
    };

    DatagramAudioPacket(int numChannels, int bufferSize, float sampleRate);

    DatagramAudioPacket(IPAddress &peerIP, uint16_t peerPort, uint8_t *packetData);

    ~DatagramAudioPacket();

    void incrementSeqNumber();

    void writeAudioData(int channel, void *channelData);

    /**
     * Write the header to the packet
     */
    void writeHeader();

    /**
     * Get a pointer to the audio data portion of the packet.
     */
    uint8_t *getRawAudioData();

    void getAudioData(const int16_t **buffer);

    int getSeqNumber() const;

    uint8_t *getData();

    size_t getSize() const;

    DatagramAudioPacket::Origin getOrigin();

    void reset();

    uint8_t getNumAudioChannels() const;

    uint16_t getBufferSize() const;

private:
    Origin origin;
    PacketHeader header{};
    uint8_t *data;
    size_t size, bytesPerChannel;

    void parseHeader();
};

#define PACKET_HEADER_SIZE sizeof(DatagramAudioPacket::PacketHeader)

#endif //NETJUCE_TEENSY_DATAGRAMAUDIOPACKET_H
