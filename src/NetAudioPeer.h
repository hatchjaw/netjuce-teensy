//
// Created by tar on 3/15/23.
//

#ifndef NETJUCE_TEENSY_NETAUDIOPEER_H
#define NETJUCE_TEENSY_NETAUDIOPEER_H

#include <Audio.h>
#include <QNEthernet.h>
#include "ClientSettings.h"
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include <memory>

using CB16 = CircularBuffer<int16_t>;

class NetAudioPeer {
public:
    NetAudioPeer(DatagramAudioPacket &firstPacket, const ClientSettings &settings);

    void handlePacket(DatagramAudioPacket &p);

    void getNextAudioBlock(int16_t **bufferToFill, int numSamples);

    bool isConnected();

    const DatagramAudioPacket::Origin &getOrigin() const;

    float getDriftRatio(bool andPrint = false);

    void resetDriftRatio();

private:
    DatagramAudioPacket::Origin origin;
    elapsedMillis receiveTimer;
    std::unique_ptr<CircularBuffer<int16_t>> audioBuffer;
    uint16_t prevSeqNum;
};


#endif //NETJUCE_TEENSY_NETAUDIOPEER_H
