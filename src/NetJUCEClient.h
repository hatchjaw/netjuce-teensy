//
// Created by Tommy Rushton on 22/02/2023.
//

#ifndef NETJUCE_NETJUCECLIENT_H
#define NETJUCE_NETJUCECLIENT_H

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
#define DEFAULT_REMOTE_PORT 30000
#endif

#ifndef DEFAULT_LOCAL_PORT
#define DEFAULT_LOCAL_PORT 15000
#endif

#define FIFO_SIZE (AUDIO_BLOCK_SAMPLES * 8)

#include <Audio.h>
#include <NativeEthernet.h>
#include <map>
#include "CircularBuffer.h"
#include "DatagramPacket.h"

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

    void connect(uint connectTimeoutMs = 1000);

    void setDebugMode(DebugMode mode);

private:
    const uint16_t kReceiveTimeoutMs{5000};

    void update(void) override;

    void receive();

    void doAudioOutput();

    void hexDump(const uint8_t *buffer, int length, bool doHeader = false) const;

    EthernetUDP socket;
    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte mac[6]{};
    /**
     * IP address of the server...
     */
    IPAddress adapterIP;
    /**
     * IP to assign to Teensy.
     */
    IPAddress clientIP;
    /**
     * IP of the multicast group to join.
     */
    IPAddress multicastIP;
    uint16_t remotePort, localPort;
    bool joined{false}, connected{false};
    elapsedMillis receiveTimer{0};
    /**
     * Buffer for incoming packets.
     */
    uint8_t packetBuffer[FNET_SOCKET_DEFAULT_SIZE]{};
    uint64_t receivedCount{0};

    CircularBuffer<int16_t> audioBuffer;
    std::map<uint32_t, CircularBuffer<int16_t>*> audioBuffers;
    int16_t **audioBlock;

    /**
     * Something something outgoing packets
     */
    DatagramPacket outgoingPacket;

    DebugMode debugMode;

    void send();
};


#endif //NETJUCE_NETJUCECLIENT_H
