//
// Created by tar on 3/15/23.
//

#ifndef NETJUCE_TEENSY_PEER_H
#define NETJUCE_TEENSY_PEER_H

#include <Audio.h>
#include <NativeEthernet.h>
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include <memory>

class Peer {
public:
    explicit Peer(DatagramAudioPacket firstPacket);

    void handlePacket(DatagramAudioPacket &p);

    void getNextAudioBlock(int16_t **bufferToFill, int numSamples);

    bool isConnected();

    const DatagramAudioPacket::Origin &getOrigin() const;

private:
    DatagramAudioPacket::Origin origin;
    elapsedMillis receiveTimer;
    std::unique_ptr<CircularBuffer<int16_t>> audioBuffer;
};


#endif //NETJUCE_TEENSY_PEER_H
