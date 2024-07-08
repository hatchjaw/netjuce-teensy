//
// Created by Tommy Rushton on 22/02/2023.
//

#include <Audio.h>
#include <QNEthernet.h>
#include <unordered_map>
#include <memory>
#include "ClientSettings.h"
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include "NetAudioPeer.h"
#include "ProgramContext.h"

#ifndef NETJUCE_NETJUCECLIENT_H
#define NETJUCE_NETJUCECLIENT_H

// unordered_map performs better ("average constant-time") than map (logarithmic) according to c++ reference
// https://en.cppreference.com/w/cpp/container/unordered_map
using PeerMap = std::unordered_map<uint32_t, std::unique_ptr<NetAudioPeer>>;

using namespace qindesign::network;

class NetJUCEClient : public AudioStream {
public:
    NetJUCEClient(const ClientSettings &settings);

    virtual ~NetJUCEClient();

    /**
     * This method sets up ethernet. Arguably (and certainly if using ethernet
     * for other purposes, e.g. transmitting/receiving OSC messages) this class
     * should not be responsible for ethernet.
     * @see ../examples/wfs, where ethernet management is delegated to a
     * dedicated EthernetManager.
     */
    bool begin();

    bool isConnected() const;

    void connect(uint connectTimeoutMs = 1000);

    /**
     * Operations that occur on the loop ISR.
     */
    void loop();

private:
    const uint16_t kReceiveTimeoutMs{5000};
    const int kExpectedReceiveInterval{int(1000000 * AUDIO_BLOCK_SAMPLES / AUDIO_SAMPLE_RATE)};

    /**
     * Operations that occur on the audio ISR.
     */
    void update(void) override;

    void receive();

    void doAudioOutput();

    void getAudioAndSend();

    void checkConnectivity();

    void adjustClock();

    void handleAudioInput();

    void send();

    static void hexDump(const uint8_t *buffer, int length, bool doHeader = false);

    const ClientSettings &settings;

    EthernetUDP socket{4};
    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte mac[6]{};
    /**
     * IP to assign to Teensy.
     */
    IPAddress clientIP;
    IPAddress gatewayIP{192, 168, 10, 1}, netmask{255, 255, 255, 0};
    volatile bool ready{false}, joined{false}, connected{false};
    elapsedMillis receiveTimer{0}, peerCheckTimer{0}, driftCheckTimer{0};
    elapsedMicros receiveInterval;
    /**
     * Buffer for incoming packets.
     */
    uint8_t packetBuffer[EthernetClass::mtu()]{};
    uint64_t receivedCount{0};

    PeerMap peers;
    PeerMap::iterator server;

    int16_t **audioBlock;

    DatagramAudioPacket outgoingPacket, incomingPacket;

    volatile bool packetReady{false};

    double sampleRate{AUDIO_SAMPLE_RATE_EXACT};
    SmoothedValue_V2<double> fs{AUDIO_SAMPLE_RATE_EXACT, .9, 1e-6};

    uint16_t prevSeqNum{0};
    int numPacketsDropped{0};
};

#endif //NETJUCE_NETJUCECLIENT_H
