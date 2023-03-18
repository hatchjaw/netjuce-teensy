//
// Created by tar on 3/15/23.
//

#ifndef NETJUCE_TEENSY_NETAUDIOPEER_H
#define NETJUCE_TEENSY_NETAUDIOPEER_H

#include <Audio.h>
#include <NativeEthernet.h>
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include <memory>

using CB16 = CircularBuffer<int16_t>;

class NetAudioPeer {
public:
    NetAudioPeer(DatagramAudioPacket &firstPacket);

    void handlePacket(DatagramAudioPacket &p);

    void getNextAudioBlock(int16_t **bufferToFill, int numSamples);

    bool isConnected();

    const DatagramAudioPacket::Origin &getOrigin() const;

    float getDriftRatio(bool andPrint = false);

private:
    DatagramAudioPacket::Origin origin;
    elapsedMillis receiveTimer;
    std::unique_ptr<CircularBuffer<int16_t>> audioBuffer;
};


#endif //NETJUCE_TEENSY_NETAUDIOPEER_H
