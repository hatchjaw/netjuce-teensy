//
// Created by Tommy Rushton on 22/02/2023.
//

#ifndef NETJUCE_NETJUCECLIENT_H
#define NETJUCE_NETJUCECLIENT_H

#include <Audio.h>
//#include <NativeEthernet.h>
#include <imxrt_hw.h>
#include <unordered_map>
#include <memory>
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include "NetAudioPeer.h"

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES 128
#endif

#ifndef NUM_SOURCES
#define NUM_SOURCES 2
#endif

#ifndef MULTICAST_IP
#define MULTICAST_IP "226.6.38.226"
#endif

#ifndef DEFAULT_REMOTE_PORT
#define DEFAULT_REMOTE_PORT 41814
#endif

#ifndef DEFAULT_LOCAL_PORT
#define DEFAULT_LOCAL_PORT 14841
#endif

enum class DebugMode : uint32_t
{
    NONE = 0,
    HEXDUMP_RECEIVE = 1 << 0,
    HEXDUMP_SEND = 1 << 1,
    HEXDUMP_AUDIO_OUT = 1 << 2
};

constexpr enum DebugMode operator |(const enum DebugMode selfValue, const enum DebugMode inValue )
{
    return (enum DebugMode)(uint32_t(selfValue) | uint32_t(inValue));
}

class NetJUCEClient : public AudioStream {
public:
    NetJUCEClient(IPAddress &networkAdapterIPAddress,
                  IPAddress &multicastIPAddress,
                  uint16_t remotePortNumber = DEFAULT_REMOTE_PORT,
                  uint16_t localPortNumber = DEFAULT_LOCAL_PORT,
                  DebugMode debugModeToUse = DebugMode::NONE);

    virtual ~NetJUCEClient();

    bool begin();

    bool isConnected() const;

//    void connect(uint connectTimeoutMs = 1000);

    void setDebugMode(DebugMode mode);

    void handlePacket(IPAddress& remoteIP, uint16_t remotePortItCameFrom, uint8_t *data, size_t size);

private:
    const uint16_t kReceiveTimeoutMs{5000};

    void update(void) override;

//    void receive();

    void doAudioOutput();

    void send();

    void checkConnectivity();

    void adjustClock();

    void hexDump(const uint8_t *buffer, int length, bool doHeader = false) const;

//    EthernetUDP sender;
    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte mac[6]{};
    /**
     * IP address of the server.
     */
    IPAddress adapterIP;

//    /**
//     * IP to assign to Teensy.
//     */
//    IPAddress clientIP;
//    IPAddress gatewayIP;
//    const IPAddress netmask{255, 255, 255, 0};
//    const IPAddress dnsServer{8, 8, 8, 8};

    /**
     * IP of the multicast group to join.
     */
    IPAddress multicastIP;
    uint16_t remotePort, localPort;
    volatile bool joined{false}, connected{false};
    elapsedMillis receiveTimer{0}, peerCheckTimer{0}, driftCheckTimer{0};
    /**
     * Buffer for incoming packets.
     */
    uint8_t packetBuffer[1024 * 2]{};
    uint64_t receivedCount{0};

    // unordered_map performs better ("average constant-time") than map (logarithmic) according to c++ reference
    // https://en.cppreference.com/w/cpp/container/unordered_map
    std::unordered_map<uint32_t, std::unique_ptr<NetAudioPeer>> peers;
    int16_t **audioBlock;

    DatagramAudioPacket outgoingPacket, incomingPacket;

    DebugMode debugMode;
};

#endif //NETJUCE_NETJUCECLIENT_H
