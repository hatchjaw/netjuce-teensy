//
// Created by tar on 3/15/23.
//

#include "NetAudioPeer.h"

NetAudioPeer::NetAudioPeer(DatagramAudioPacket &firstPacket, const ClientSettings &settings) :
        origin(firstPacket.getOrigin()),
        receiveTimer(0),
        audioBuffer(std::make_unique<CB16>(
                firstPacket.getNumAudioChannels(), // Will break if number of channels is greater than NUM_SOURCES
                CIRCULAR_BUFFER_SIZE,
                settings)
        ),
        prevSeqNum(firstPacket.getSeqNumber()) {
}

void NetAudioPeer::handlePacket(DatagramAudioPacket &p) {
    receiveTimer = 0;
    origin = p.getOrigin();
    const int16_t *audio[p.getNumAudioChannels()];
    p.getAudioData(audio);
    audioBuffer->write(audio, p.getBufferSize());

    auto seqStep{p.getSeqNumber() - prevSeqNum};
    if (seqStep != 1 && seqStep != -UINT16_MAX) {
        Serial.printf("Dropped %d packets\n", seqStep - 1);
    }
    prevSeqNum = p.getSeqNumber();
}

void NetAudioPeer::getNextAudioBlock(int16_t **bufferToFill, int numSamples) {
    audioBuffer->read(bufferToFill, numSamples);
}

bool NetAudioPeer::isConnected() {
    return receiveTimer < 1000;
}

const DatagramAudioPacket::Origin &NetAudioPeer::getOrigin() const {
    return origin;
}

float NetAudioPeer::getDriftRatio(bool andPrint) {
    return audioBuffer->getDriftRatio(andPrint);
}

void NetAudioPeer::resetDriftRatio() {
    audioBuffer->clear();
}
