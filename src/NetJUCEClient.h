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
#include <QNEthernet.h>

namespace qn = qindesign::network;

class NetJUCEClient : public AudioStream, qn::EthernetUDP {
public:
    explicit NetJUCEClient(
            IPAddress &multicastIPAddress,
            uint16_t remotePort = REMOTE_PORT,
            uint16_t localPort = LOCAL_PORT
    );

    bool begin();

private:
    void update(void) override;

    IPAddress multicastIP;
    uint16_t remotePort, localPort;
};


#endif //NETJUCE_NETJUCECLIENT_H
