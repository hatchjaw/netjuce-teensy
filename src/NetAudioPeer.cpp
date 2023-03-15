//
// Created by tar on 3/15/23.
//

#include "NetAudioPeer.h"


NetAudioPeer::NetAudioPeer(DatagramAudioPacket firstPacket) :
        origin(firstPacket.getOrigin()),
        receiveTimer(0) {
    audioBuffer = std::make_unique<CircularBuffer<int16_t>>(firstPacket.getNumAudioChannels(), CIRCULAR_BUFFER_SIZE);
}

void NetAudioPeer::handlePacket(DatagramAudioPacket &p) {
    receiveTimer = 0;
    origin = p.getOrigin();
    const int16_t *audio[p.getNumAudioChannels()];
    p.getAudioData(audio);
    audioBuffer->write(audio, p.getBufferSize());
}

void NetAudioPeer::getNextAudioBlock(int16_t **bufferToFill, int numSamples) {
    audioBuffer->read(bufferToFill, numSamples);
}

bool NetAudioPeer::isConnected() {
    return receiveTimer < 2500;
}

const DatagramAudioPacket::Origin &NetAudioPeer::getOrigin() const {
    return origin;
}
