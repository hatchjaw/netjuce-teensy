//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"

NetJUCEClient::NetJUCEClient(IPAddress &multicastIPAddress, uint16_t remotePort, uint16_t localPort) :
        AudioStream{NUM_SOURCES, new audio_block_t *[NUM_SOURCES]},
        multicastIP{multicastIPAddress},
        remotePort{remotePort},
        localPort{localPort} {

}

bool NetJUCEClient::begin() {
    return qn::EthernetUDP::beginMulticastWithReuse(multicastIP, remotePort);
}

void NetJUCEClient::update(void) {

}
