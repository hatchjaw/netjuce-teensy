//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"
#include "PacketHeader.h"
#include <TeensyID.h>
#include <imxrt_hw.h>

NetJUCEClient::NetJUCEClient(IPAddress &multicastIPAddress, uint16_t remotePort, uint16_t localPort) :
        AudioStream{NUM_SOURCES, new audio_block_t *[NUM_SOURCES]},
        multicastIP{multicastIPAddress},
        remotePort{remotePort},
        localPort{localPort} {
    teensyMAC(clientMAC);
    clientIP[3] = clientMAC[5];
}

bool NetJUCEClient::begin() {
    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  clientMAC[0], clientMAC[1], clientMAC[2], clientMAC[3], clientMAC[4], clientMAC[5]);

    EthernetClass::begin(clientMAC, clientIP);

    receiveTimer = 0;

//    //PLL:
//    int fs = 44101.f;
//    // PLL between 27*24 = 648MHz und 54*24=1296MHz
//    int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
//    int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);
//
//    double C = ((double)fs * 256 * n1 * n2) / 24000000;
//    int c0 = C;
//    int c2 = 10000;
//    int c1 = C * c2 - (c0 * c2);
//    Serial.printf("nfact %d nmult %d ndiv %d\n", c0, c1, c2);
//    set_audioClock(c0, c1, c2, true);

    if (EthernetClass::linkStatus() != LinkON) {
        Serial.println("Ethernet link could not be established.");
        return false;
    } else {
        Serial.print(F("IP: "));
        Serial.println(EthernetClass::localIP());
        return true;
    }
}

bool NetJUCEClient::isConnected() const {
    return connected;
}

void NetJUCEClient::connect(uint connectTimeoutMs) {
    if (!active) {
        Serial.println(F("Client is not connected to any Teensy audio objects."));
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
    receive();

    doAudioOutput();

    send();
}

void NetJUCEClient::doAudioOutput() {
    audio_block_t *outBlock[NUM_SOURCES];
    auto channelFrameSize{AUDIO_BLOCK_SAMPLES * sizeof(uint16_t)};

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        outBlock[ch] = allocate();
        if (outBlock[ch]) {
            // Copy the samples to the output block.
            memcpy(outBlock[ch]->data,
                   reinterpret_cast<int16_t *>(packetBuffer + PACKET_HEADER_SIZE + channelFrameSize * ch),
                   channelFrameSize);
            // Finish up.
            transmit(outBlock[ch], ch);
            release(outBlock[ch]);
        }
    }
}

void NetJUCEClient::receive() {
    auto packetSize{0};
    if (connected) {
        auto didRead{false};
        // TODO: switch to `while`; use circular buffer.
        if ((packetSize = udp.parsePacket()) > 0) {
            ++receivedCount;
            didRead = true;

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

    hexDump(packetBuffer, packetSize);

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
        udp.stop();
        connected = false;
        receiveTimer = 0;
    }
}

void NetJUCEClient::hexDump(const uint8_t *buffer, int length) const {
    if (receivedCount > 0 && receivedCount % 1000 == 0) {
        Serial.printf("Seq. number %d\n", receivedCount);
        int word{0}, row{0};
        for (const uint8_t *p = buffer; word < length; ++p, ++word) {
            if (word % 16 == 0) {
                if (word != 0) Serial.print("\n");
                Serial.printf("%04x: ", row);
                ++row;
            }
            Serial.printf("%02x ", *p);
        }
        Serial.println("\n");
    }
}

void NetJUCEClient::send() {

}


