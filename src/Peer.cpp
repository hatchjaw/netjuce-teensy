//
// Created by tar on 3/15/23.
//

#include "Peer.h"


Peer::Peer(DatagramAudioPacket firstPacket) :
        origin(firstPacket.getOrigin()),
        receiveTimer(0) {
    audioBuffer = std::make_unique<CircularBuffer<int16_t>>(firstPacket.getNumAudioChannels(), CIRCULAR_BUFFER_SIZE);
}

void Peer::handlePacket(DatagramAudioPacket &p) {
    receiveTimer = 0;
    origin = p.getOrigin();
//    const int16_t *audio[p.getNumAudioChannels()];
//    for (int ch = 0; ch < p.getNumAudioChannels(); ++ch) {
//        audio[ch] = reinterpret_cast<int16_t *>(p.getRawAudioData() + PACKET_HEADER_SIZE + ch * bytesPerChannel);
//    }
    const int16_t *audio[p.getNumAudioChannels()];
    p.getAudioData(audio);
    audioBuffer->write(audio, p.getBufferSize());
}

void Peer::getNextAudioBlock(int16_t **bufferToFill, int numSamples) {
    audioBuffer->read(bufferToFill, numSamples);
}

bool Peer::isConnected() {
    return receiveTimer < 2500;
}

const DatagramAudioPacket::Origin &Peer::getOrigin() const {
    return origin;
}
