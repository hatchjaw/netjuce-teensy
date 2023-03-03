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

#ifndef REMOTE_PORT
#define REMOTE_PORT 18999
#endif

#ifndef LOCAL_PORT
#define LOCAL_PORT 18999
#endif

#include <Audio.h>
#include <NativeEthernet.h>

class NetJUCEClient : public AudioStream {
public:
    explicit NetJUCEClient(
            IPAddress &multicastIPAddress,
            uint16_t remotePort = REMOTE_PORT,
            uint16_t localPort = LOCAL_PORT
    );

    bool begin();

    bool isConnected();

    void connect(uint connectTimeoutMs = 1000);

private:
    const uint16_t kReceiveTimeoutMs{5000};

    void update(void) override;

    EthernetUDP udp;
    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte clientMAC[6]{};
    /**
     * IP to assign to Teensy.
     */
    IPAddress clientIP{192, 168, 10, 0};
    /**
     * IP of the multicast group to join.
     */
    IPAddress multicastIP;
    uint16_t remotePort, localPort;
    bool connected{false};
    elapsedMillis receiveTimer{0};
    char packetBuffer[1 << 8]{};
    uint64_t receivedCount{0}, rcv{0};
};


#endif //NETJUCE_NETJUCECLIENT_H
