//
// Created by Tommy Rushton on 22/02/2023.
//

#include <Audio.h>
#include <QNEthernet.h>
#include <unordered_map>
#include <memory>
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include "NetAudioPeer.h"
#include "ProgramContext.h"

#ifndef NETJUCE_NETJUCECLIENT_H
#define NETJUCE_NETJUCECLIENT_H

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES 128
#endif

#ifndef NUM_SOURCES
#define NUM_SOURCES 2
#endif

#ifndef MULTICAST_IP
#define MULTICAST_IP "224.4.224.4"
#endif

#ifndef DEFAULT_REMOTE_PORT
#define DEFAULT_REMOTE_PORT 6664
#endif

#ifndef DEFAULT_LOCAL_PORT
#define DEFAULT_LOCAL_PORT 6664
#endif

#ifndef RESAMPLING_MODE
#define RESAMPLING_MODE INTERPOLATE
#endif

// unordered_map performs better ("average constant-time") than map (logarithmic) according to c++ reference
// https://en.cppreference.com/w/cpp/container/unordered_map
using PeerMap = std::unordered_map<uint32_t, std::unique_ptr<NetAudioPeer>>;

using namespace qindesign::network;

class NetJUCEClient : public AudioStream {
public:
    NetJUCEClient(const ClientSettings &settings);

    virtual ~NetJUCEClient();

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
    SmoothedValue_V2<double> fs{AUDIO_SAMPLE_RATE_EXACT, .95, 1e-3};

    uint16_t prevSeqNum{0};
    int numPacketsDropped{0};
};

#endif //NETJUCE_NETJUCECLIENT_H
