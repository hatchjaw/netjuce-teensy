//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"
#include <TeensyID.h>

NetJUCEClient::NetJUCEClient(IPAddress &multicastIPAddress, uint16_t remotePort, uint16_t localPort) :
        AudioStream{NUM_SOURCES, new audio_block_t *[NUM_SOURCES]},
        multicastIP{multicastIPAddress},
        remotePort{remotePort},
        localPort{localPort} {
    Serial.println("client ctr");
    teensyMAC(clientMAC);
    clientIP[3] = clientMAC[5];
}

bool NetJUCEClient::begin() {
    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  clientMAC[0], clientMAC[1], clientMAC[2], clientMAC[3], clientMAC[4], clientMAC[5]);

    EthernetClass::begin(clientMAC, clientIP);

    receiveTimer = 0;

    if (EthernetClass::linkStatus() != LinkON) {
        Serial.println("Ethernet link could not be established.");
        return false;
    } else {
        Serial.print(F("IP: "));
        Serial.println(EthernetClass::localIP());
        return true;
    }
}

bool NetJUCEClient::isConnected() {
    return connected;
}

void NetJUCEClient::connect(uint connectTimeoutMs) {
    if (!active) {
//        Serial.println(F("===================================================="));
        Serial.println(F("Client is not connected to any Teensy audio objects."));
//        Serial.println(F("===================================================="));
        delay(connectTimeoutMs);
        return;
    }

    Serial.print(F("Joining multicast group at "));
    Serial.print(multicastIP);
    Serial.printf(F(":%d... "), localPort);

    auto joined{udp.beginMulticast(multicastIP, remotePort)};

    if (joined) {
        Serial.println(F("Success!"));
        receivedCount = 0;
        connected = true;
    } else {
        Serial.println(F("Failed."));
        connected = false;
        delay(connectTimeoutMs);
    }
}

void NetJUCEClient::update(void) {
    auto packetSize{0};
    if (connected) {
        auto didRead{0};
        if ((packetSize = udp.parsePacket()) > 0) {
            auto isSmall{packetSize < 50};
            ++receivedCount;
            ++rcv;
            ++didRead;

//            if (receivedCount % 1 == 0) {
//                Serial.printf(F("%16" PRIu64 " Received packet from "), receivedCount);
//                IPAddress remote = udp.remoteIP();
//                for (int i = 0; i < 4; i++) {
//                    Serial.print(remote[i], DEC);
//                    if (i < 3) {
//                        Serial.print(F("."));
//                    }
//                }
//                Serial.printf(F(":%d, size %d bytes\n"), udp.remotePort(), packetSize);
//            }

            // read the packet into packetBuffer
//            isSmall && Serial.print(F("Contents: "));
//            while (0 < udp.read(packetBuffer, packetSize)) {
//                isSmall && Serial.print(packetBuffer);
////                memset(packetBuffer, 0, sizeof packetBuffer);
//            }
//            isSmall && Serial.println();

            if (packetSize != 128) Serial.printf("Packet size: %d\n", packetSize);
            udp.read(packetBuffer, packetSize);
            receiveTimer = 0;
        }

        if (0 == didRead) {
            memset(packetBuffer, 0, sizeof packetBuffer);
        }
    } else {
        memset(packetBuffer, 0, sizeof packetBuffer);
    }

//    audioBuffer.read(audioBlock, AUDIO_BLOCK_SAMPLES);
    audio_block_t *outBlock[NUM_SOURCES];
//    const int16_t *audio[NUM_SOURCES];
    auto channelFrameSize{AUDIO_BLOCK_SAMPLES * sizeof(uint16_t)};

//    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
//        audio[ch] = reinterpret_cast<int16_t *>(packetBuffer + AUDIO_BLOCK_SAMPLES * sizeof(uint16_t) * ch);
//    }

    if (rcv == 1000) {
        Serial.printf("Received %d\n", receivedCount);
        int i = 1;
        for (const char *p = packetBuffer; i <= packetSize; ++p, ++i) {
            Serial.printf("%02x ", *p);
            if (i % 16 == 0) {
                Serial.print("\n");
            }
        }
        Serial.println();

        rcv = 0;
    }

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        outBlock[ch] = allocate();
        if (outBlock[ch]) {
            // Copy the samples to the output block.
//            memcpy(outBlock[ch]->data, audioBlock[ch], CHANNEL_FRAME_SIZE);
            memcpy(outBlock[ch]->data, reinterpret_cast<int16_t *>(packetBuffer + channelFrameSize * ch),
                   channelFrameSize);
//            memset(outBlock[ch]->data, 0, channelFrameSize);
            // Finish up.
            transmit(outBlock[ch], ch);
            release(outBlock[ch]);
        }
    }

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
        udp.stop();
        connected = false;
        receiveTimer = 0;
    }
}


