//
// Created by tar on 3/13/23.
//

#include "DatagramAudioPacket.h"

//DatagramAudioPacket::DatagramAudioPacket(IPAddress &peerIP, uint16_t peerPort, uint8_t *packetData) :
//        origin{peerIP, peerPort},
//        data{packetData} {
//    parseHeader();
//    bytesPerChannel = header.BufferSize * sizeof(int16_t);
//    size = PACKET_HEADER_SIZE + header.NumChannels * bytesPerChannel;
//}

DatagramAudioPacket::DatagramAudioPacket(int numChannels, int bufferSize, float sampleRate) {
    bytesPerChannel = bufferSize * sizeof(int16_t); // TODO: generalise this.
    size = PACKET_HEADER_SIZE + numChannels * bytesPerChannel;
    data = new uint8_t[size];

    header.BitResolution = BIT16;
    header.BufferSize = bufferSize;
    header.NumChannels = numChannels;
    header.SeqNumber = 0;
    switch (static_cast<int>(sampleRate)) {
        case 22050:
            header.SamplingRate = SR22;
            break;
        case 32000:
            header.SamplingRate = SR32;
            break;
        case 44100:
            header.SamplingRate = SR44;
            break;
        case 48000:
            header.SamplingRate = SR48;
            break;
        case 88200:
            header.SamplingRate = SR88;
            break;
        case 96000:
            header.SamplingRate = SR96;
            break;
        case 19200:
            header.SamplingRate = SR192;
            break;
        default:
            header.SamplingRate = UNDEF;
            break;
    }
}

DatagramAudioPacket::~DatagramAudioPacket() {
    delete data;
}

void DatagramAudioPacket::fromRawPacketData(IPAddress &peerIP, uint16_t peerPort, uint8_t *packetData) {
    origin = {peerIP, peerPort};
    header = *reinterpret_cast<PacketHeader *>(packetData);
    bytesPerChannel = header.BufferSize * sizeof(int16_t);
    // TODO: check whether the new size is different, resize data if necessary.
    size = PACKET_HEADER_SIZE + header.NumChannels * bytesPerChannel;
    memcpy(data, packetData, size);
}

void DatagramAudioPacket::incrementSeqNumber() {
    ++header.SeqNumber;
}

int DatagramAudioPacket::getSeqNumber() const {
    return header.SeqNumber;
}

void DatagramAudioPacket::writeHeader() {
    memcpy(data, &header, PACKET_HEADER_SIZE);
}

uint8_t *DatagramAudioPacket::getRawAudioData() {
    return data + PACKET_HEADER_SIZE;
}

uint8_t *DatagramAudioPacket::getData() {
    return data;
}

size_t DatagramAudioPacket::getSize() const {
    return size;
}

void DatagramAudioPacket::writeAudioData(int channel, void *channelData) {
    memcpy(data + PACKET_HEADER_SIZE + channel * bytesPerChannel, channelData, bytesPerChannel);
}

void DatagramAudioPacket::reset() {
    header.SeqNumber = 0;
}

void DatagramAudioPacket::parseHeader() {
    header = *reinterpret_cast<PacketHeader *>(data);
}

DatagramAudioPacket::Origin DatagramAudioPacket::getOrigin() {
    return origin;
}

uint8_t DatagramAudioPacket::getNumAudioChannels() const {
    return header.NumChannels;
}

uint16_t DatagramAudioPacket::getBufferSize() const {
    return header.BufferSize;
}

void DatagramAudioPacket::getAudioData(const int16_t **buffer) {
    for (int ch = 0; ch < header.NumChannels; ++ch) {
        buffer[ch] = reinterpret_cast<int16_t *>(data + PACKET_HEADER_SIZE + ch * bytesPerChannel);
    }
}
