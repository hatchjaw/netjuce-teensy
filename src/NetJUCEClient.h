//
// Created by Tommy Rushton on 22/02/2023.
//

#ifndef NETJUCE_NETJUCECLIENT_H
#define NETJUCE_NETJUCECLIENT_H

#include <Audio.h>
#include <TeensyThreads.h>
// Can be included as many times as necessary, without `Multiple Definitions` Linker Error
#include <AsyncUDP_Teensy41.hpp>        // https://github.com/khoih-prog/AsyncUDP_Teensy41
#include <QNEthernet.h>
#include <imxrt_hw.h>
#include <unordered_map>
#include <memory>
#include "CircularBuffer.h"
#include "DatagramAudioPacket.h"
#include "NetAudioPeer.h"
#include "Runnable.h"

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

#ifndef FNET_SOCKET_DEFAULT_SIZE
#define FNET_SOCKET_DEFAULT_SIZE (1024 * 2)
#endif

#if !( defined(CORE_TEENSY) && defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41) )
//#error Only Teensy 4.1 supported
#endif

#define ASYNC_UDP_TEENSY41_DEBUG_PORT       Serial

// Debug Level from 0 to 4
//#define _ASYNC_UDP_TEENSY41_LOGLEVEL_       4

#define SHIELD_TYPE     "Teensy4.1 QNEthernet"

#if (_ASYNC_UDP_TEENSY41_LOGLEVEL_ > 3)
#warning Using QNEthernet lib for Teensy 4.1. Must also use Teensy Packages Patch or error
#endif

#define ASYNC_UDP_TEENSY41_VERSION_MIN_TARGET      "AsyncUDP_Teensy41 v1.2.1"
#define ASYNC_UDP_TEENSY41_VERSION_MIN             1002001

using namespace qindesign::network;

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

    void doSomething();

private:
//    class Receiver : Runnable {
//    public:
//        // Constructor/Destructor
//        Receiver(int interval);
//        ~Receiver();
//
//        // Start thread that will last for duration
//        void startReceiving();
//    protected:
//        // Runnable function that we need to implement
//        void runTarget(void *arg) override;
//    private:
//        // Timing Variables
//        int rxInterval;
//
//        // Thread object
//        std::thread *rxThread{};
//        elapsedMicros timer{0};
//    };

    const uint16_t kReceiveTimeoutMs{5000};

    void update(void) override;

    void handleIncomingPacket(AsyncUDPPacket &packet);

    void receive();

    void doAudioOutput();

    void send();

    void checkConnectivity();

    void adjustClock();

    void hexDump(const uint8_t *buffer, int length, bool doHeader = false) const;

    AsyncUDP udp;
//    EthernetUDP sender;
    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte mac[6]{};
    /**
     * IP address of the server.
     */
    IPAddress adapterIP;
    /**
     * IP to assign to Teensy.
     */
    IPAddress clientIP;
    IPAddress gatewayIP;
    const IPAddress netmask{255, 255, 255, 0};
    const IPAddress dnsServer{8, 8, 8, 8};
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
    uint8_t packetBuffer[FNET_SOCKET_DEFAULT_SIZE]{};
    uint64_t receivedCount{0};

    // unordered_map performs better ("average constant-time") than map (logarithmic) according to c++ reference
    // https://en.cppreference.com/w/cpp/container/unordered_map
    std::unordered_map<uint32_t, std::unique_ptr<NetAudioPeer>> peers;
    int16_t **audioBlock;

    DatagramAudioPacket outgoingPacket, incomingPacket;

    DebugMode debugMode;

//    std::thread *receiveThread;
//    Receiver rx;
//    Threads::Mutex readLock;
//    Threads::Mutex lock;
    elapsedMicros threadTimer{0};
};

#endif //NETJUCE_NETJUCECLIENT_H
