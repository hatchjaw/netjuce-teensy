//
// Created by tar on 3/13/23.
//

#include "DatagramPacket.h"

DatagramPacket::DatagramPacket(int numChannels, int bufferSize, float sampleRate) {
    bytesPerChannel = bufferSize * sizeof(int16_t);
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

DatagramPacket::~DatagramPacket() {
    std::free(data);
    data = nullptr;
}

void DatagramPacket::incrementSeqNumber() {
    ++header.SeqNumber;
}

int DatagramPacket::getSeqNumber() const {
    return header.SeqNumber;
}

void DatagramPacket::writeHeader() {
    memcpy(data, &header, PACKET_HEADER_SIZE);
}

uint8_t *DatagramPacket::getAudioData() {
    return data + PACKET_HEADER_SIZE;
}

uint8_t *DatagramPacket::getData() {
    return data;
}

size_t DatagramPacket::getSize() const {
    return size;
}

void DatagramPacket::writeAudioData(int channel, void *channelData) {
    memcpy(data + PACKET_HEADER_SIZE + channel * bytesPerChannel, channelData, bytesPerChannel);
}
