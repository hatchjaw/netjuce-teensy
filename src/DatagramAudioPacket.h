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

    /**
     * Prepare a new packet for sending.
     * @param numChannels
     * @param bufferSize
     * @param sampleRate
     */
    DatagramAudioPacket(int numChannels, int bufferSize, float sampleRate);

//    /**
//     * Build a new packet from incoming raw data.
//     * @param peerIP
//     * @param peerPort
//     * @param packetData
//     */
//    DatagramAudioPacket(IPAddress &peerIP, uint16_t peerPort, uint8_t *packetData);

    ~DatagramAudioPacket();

    // Copy constructor.
    DatagramAudioPacket(const DatagramAudioPacket &p) = delete;

//    :
//            data(new uint8_t(*p.data)),
//            size(p.size),
//            bytesPerChannel(p.bytesPerChannel) {}

    // Assignment operator.
    DatagramAudioPacket &operator=(const DatagramAudioPacket &p) = delete;
//    {
//        if (this != &p) {
//            *data = *p.data;
//        }
//        return *this;
//    }

    void fromRawPacketData(IPAddress &peerIP, uint16_t peerPort, uint8_t *packetData);

    /**
     * Increment this packet's header's sequence number.
     */
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

    /**
     * Read audio data into a buffer.
     * @param buffer
     */
    void getAudioData(const int16_t **buffer);

    int getSeqNumber() const;

    /**
     * Get a pointer to this packet's raw data.
     * @return
     */
    uint8_t *getData();

    /**
     * Get the size of this packet in bytes.
     * @return
     */
    size_t getSize() const;

    /**
     * Get origin info for this packet, i.e. the IP and port it came from.
     * @return
     */
    DatagramAudioPacket::Origin getOrigin();

    /**
     * Reset the packet. Set sequence number to zero (etc.)
     */
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
