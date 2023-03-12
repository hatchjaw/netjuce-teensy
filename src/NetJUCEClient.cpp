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
        localPort{localPort},
        audioBuffer{NUM_SOURCES, AUDIO_BLOCK_SAMPLES * 32},
        audioBlock{new int16_t *[NUM_SOURCES]} {
    teensyMAC(clientMAC);
    clientIP[3] = clientMAC[5];

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        audioBlock[ch] = new int16_t[AUDIO_BLOCK_SAMPLES];
    }
}

NetJUCEClient::~NetJUCEClient() {
    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        delete[] audioBlock[ch];
    }
    delete[] audioBlock;
}


bool NetJUCEClient::begin() {
    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  clientMAC[0], clientMAC[1], clientMAC[2], clientMAC[3], clientMAC[4], clientMAC[5]);

    EthernetClass::begin(clientMAC, clientIP);

    receiveTimer = 0;

//    //PLL:
//    int fs = 44150.f;
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

//    send();
}

void NetJUCEClient::receive() {
    if (!connected) {
        memset(packetBuffer, 0, sizeof packetBuffer);
        return;
    }

    int packetSize;

    if (useCircularBuffer) {
        while ((packetSize = udp.parsePacket()) > 0) {
            ++receivedCount;
            receiveTimer = 0;

            udp.read(packetBuffer, packetSize);

//            if (receivedCount > 0 && receivedCount % 10000 <= 1) {
//                hexDump(packetBuffer, packetSize);
//            }

            if (receiveHeader) {
                auto headerIn{reinterpret_cast<PacketHeader *>(packetBuffer)};
                auto bytesPerChannel{headerIn->BufferSize * headerIn->BitResolution};

                if (receivedCount > 0 && receivedCount % 10000 <= 1) {
//                    Serial.println(*headerIn);
                    Serial.printf("SeqNumber: %d, BufferSize: %d, NumChannels: %d\n",
                                  headerIn->SeqNumber,
                                  headerIn->BufferSize,
                                  headerIn->NumChannels);
                    Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
                    Serial.printf("Seq: %d\n", headerIn->SeqNumber);
                    hexDump(packetBuffer, packetSize, true);
                }

                // Assume 16-bit; TODO: should generalise
                const int16_t *audio[headerIn->NumChannels];
                for (int ch = 0; ch < headerIn->NumChannels; ++ch) {
                    audio[ch] = reinterpret_cast<int16_t *>(packetBuffer + PACKET_HEADER_SIZE + ch * bytesPerChannel);
                }
                audioBuffer.write(audio, headerIn->BufferSize);
            } else {
                auto bytesPerFrame{AUDIO_BLOCK_SAMPLES * sizeof(int16_t)};

                // Assume 16-bit; TODO: should generalise
                const int16_t *audio[NUM_SOURCES];
                for (int ch = 0; ch < NUM_SOURCES; ++ch) {
                    audio[ch] = reinterpret_cast<int16_t *>(packetBuffer + ch * bytesPerFrame);
                }
                audioBuffer.write(audio, AUDIO_BLOCK_SAMPLES);
            }
        }
    } else {
        auto didRead{false};

        if ((packetSize = udp.parsePacket()) > 0) {
            ++receivedCount;
            didRead = true;

//            if (packetSize != 128) Serial.printf("Packet size: %d\n", packetSize);
            udp.read(packetBuffer, packetSize);
            receiveTimer = 0;
        }

        if (0 == didRead) {
            Serial.println(F("Zero packets read."));
            memset(packetBuffer, 0, sizeof packetBuffer);
        } else {
            auto headerIn{reinterpret_cast<PacketHeader *>(packetBuffer)};
            if (headerIn->SeqNumber % 10000 <= 1) {
                Serial.printf(F("Seq. number %d\n"), headerIn->SeqNumber);
                hexDump(packetBuffer, packetSize);
            }
        }
    }

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
        udp.stop();
        audioBuffer.clear();
        connected = false;
        receiveTimer = 0;
    }
}

void NetJUCEClient::doAudioOutput() {
    if (useCircularBuffer) {
        audioBuffer.read(audioBlock, AUDIO_BLOCK_SAMPLES);
        if (receivedCount > 0 && receivedCount % 10000 <= 1) {
            Serial.println(F("Channel 1"));
            hexDump(reinterpret_cast<uint8_t *>(audioBlock[0]), 64);
        }
    }

    audio_block_t *outBlock[NUM_SOURCES];
    auto channelFrameSize{AUDIO_BLOCK_SAMPLES * sizeof(int16_t)};

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        outBlock[ch] = allocate();
        if (outBlock[ch]) {
            // Copy the samples to the output block.
            if (useCircularBuffer) {
                memcpy(outBlock[ch]->data, audioBlock[ch], channelFrameSize);
            } else {
                memcpy(outBlock[ch]->data,
                       receiveHeader ?
                       reinterpret_cast<int16_t *>(packetBuffer + PACKET_HEADER_SIZE + channelFrameSize * ch) :
                       reinterpret_cast<int16_t *>(packetBuffer + channelFrameSize * ch),
                       channelFrameSize);
            }
            // Finish up.
            transmit(outBlock[ch], ch);
            release(outBlock[ch]);
        }
    }
}

void NetJUCEClient::hexDump(const uint8_t *buffer, int length, bool doHeader) const {
    int word{doHeader ? 10 : 0}, row{0};
    if (doHeader)Serial.print(F("HEAD:"));
    for (const uint8_t *p = buffer; word < length + (doHeader ? 10 : 0); ++p, ++word) {
        if (word % 16 == 0) {
            if (word != 0) Serial.print(F("\n"));
            Serial.printf(F("%04x: "), row);
            ++row;
        } else if (word % 2 == 0) {
            Serial.print(F(" "));
        }
        Serial.printf(F("%02x "), *p);
    }
    Serial.println(F("\n"));
}

void NetJUCEClient::send() {
    if (!connected) return;

    ++header.SeqNumber;
}

