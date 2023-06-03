//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"
#include <imxrt_hw.h>

NetJUCEClient::NetJUCEClient(IPAddress &networkAdapterIPAddress,
                             IPAddress &multicastIPAddress,
                             uint16_t remotePortNumber,
                             uint16_t localPortNumber,
                             DebugMode debugModeToUse) :
        AudioStream{NUM_SOURCES, new audio_block_t *[NUM_SOURCES]},
        adapterIP{networkAdapterIPAddress},
        clientIP{networkAdapterIPAddress},
        multicastIP{multicastIPAddress},
        remotePort{remotePortNumber},
        localPort{localPortNumber},
        audioBlock{new int16_t *[NUM_SOURCES]},
        outgoingPacket(NUM_SOURCES, AUDIO_BLOCK_SAMPLES, AUDIO_SAMPLE_RATE_EXACT),
        incomingPacket(NUM_SOURCES, AUDIO_BLOCK_SAMPLES, AUDIO_SAMPLE_RATE_EXACT),
        debugMode(debugModeToUse) {

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        audioBlock[ch] = new int16_t[AUDIO_BLOCK_SAMPLES];
    }

    fs.onChange = [this](double newSampleRate) {
        // PLL4:
        // Defaults are: DIV = 28, NUM = 2240, DENOM = 10000
        // PLL4 MAIN CLOCK = 24000000 * (DIV + NUM/DENOM) = 677.3592 MHz
        // 677.3592 MHz /4 /15 = 11.2896 MHz = 256 * 44100

        int sai1PRED = 4;
        // This is lifted directly from output_i2s.cpp.
        int sai1PODF = 1 + (24000000 * 27) / (newSampleRate * 256 * sai1PRED); // Should evaluate to 15
        double C = (newSampleRate * 256 * sai1PRED * sai1PODF) / 24000000;
        int div = C;
        int denom = 10000;
        int num = C * denom - (div * denom);

        if (abs(num - 2240) > 1000) {
            Serial.println("Drift ratio outside acceptable bounds; resetting.");
            server->second->resetDriftRatio();
            sampleRate = AUDIO_SAMPLE_RATE_EXACT;
        } else {
            // Attempt to decrease the error between the new fraction and the
            // target sample rate.
            auto iterations{0};
            auto error{0.0};
            auto ddiv{static_cast<double>(div)}, dnum{static_cast<double>(num)};
            do {
                error = C - (ddiv + dnum / static_cast<double>(denom));
                if (error > 0) {
                    denom--;
                } else {
                    denom++;
                }
            } while (++iterations < 10 && fabs(error) > 1e-5);

            Serial.printf("Setting sample rate: %.7f", newSampleRate);
            Serial.printf(" (C %.9f, div %d num %d denom %d)\n", C, div, num, denom);
            set_audioClock(div, num, denom, true);
        }
    };
}

NetJUCEClient::~NetJUCEClient() {
    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        delete[] audioBlock[ch];
    }
    delete[] audioBlock;
}


bool NetJUCEClient::begin() {
    Serial.printf("DEBUG MODE: %d\n", debugMode);

    Ethernet.macAddress(mac);
    Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
    clientIP[2] = mac[4];
    clientIP[3] = mac[5];

    Ethernet.onLinkState([this](bool state) {
        Serial.printf("[Ethernet] Link %s\n", state ? "ON" : "OFF");

        if (state) {
            Serial.print("IP: ");
            Serial.println(Ethernet.localIP());

            ready = true;
        }
    });

    Ethernet.begin(clientIP, netmask, gatewayIP);
}

bool NetJUCEClient::isConnected() const {
    return joined;
}

void NetJUCEClient::connect(uint connectTimeoutMs) {
    if (!active) {
        Serial.println(F("Client is not connected to any Teensy audio objects."));

        return;
    }

    if (!ready) {
        delay(connectTimeoutMs);
        return;
    }

    Serial.print(F("NetJUCEClient: Joining multicast group at "));
    Serial.print(multicastIP);
    Serial.printf(F(", listening on port %d... "), localPort);

    joined = socket.beginMulticast(multicastIP, localPort);

    if (joined) {
        Serial.println(F("Success!"));
        receiveTimer = 0;
        receivedCount = 0;
        peerCheckTimer = 0;
        outgoingPacket.reset();
    } else {
        Serial.println(F("Failed."));
        delay(connectTimeoutMs);
    }
}

void NetJUCEClient::update(void) {
    doAudioOutput();

    handleAudioInput();
}

void NetJUCEClient::loop() {
    receive();

    checkConnectivity();

    send();

    adjustClock();
}

void NetJUCEClient::adjustClock() {
    // Try to adjust the clock periodically.
    if (driftCheckTimer > 10000 && !peers.empty()) {
        driftCheckTimer = 0;

        // Proceed if the server is found.
        if (server != peers.end()) {
            auto drift = server->second->getDriftRatio(true);

            // Update sample rate.
            sampleRate *= drift;
            // Maybe trigger clock adjustment
            fs.set(sampleRate);
        }
    }
    fs.getNext();
}

void NetJUCEClient::receive() {
    if (!joined) return;

    int packetSize;

    if ((packetSize = socket.parsePacket()) > 0) {
        ++receivedCount;
        receiveTimer = 0;

//        if (abs(static_cast<int>(receiveInterval) - kExpectedReceiveInterval) > .75 * kExpectedReceiveInterval) {
        if (
                (receiveInterval > 3 * kExpectedReceiveInterval)
                ||
                (receiveInterval < kExpectedReceiveInterval / 3)
                ) {
            Serial.print("Receive interval ");
            Serial.print(receiveInterval);
            Serial.printf(" µs (dropped packets: %d)\n", numPacketsDropped);
        }
        receiveInterval = 0;

        socket.read(packetBuffer, packetSize);

        auto remoteIP{socket.remoteIP()};
        auto port{socket.remotePort()};
        auto rawIP{static_cast<uint32_t>(remoteIP)};

        auto headerIn{reinterpret_cast<DatagramAudioPacket::PacketHeader *>(packetBuffer)};
        auto bytesPerChannel{(1 << headerIn->BufferSize) * headerIn->BitResolution};

        // There's about to be at least one peer so confirm connectedness.
        auto shouldSetConnected{peers.empty()};

        incomingPacket.fromRawPacketData(remoteIP, port, packetBuffer);

        // Check for packet loss. (NB. this is nonsense for multiple peers)
        auto newSeqNum = incomingPacket.getSeqNumber();
        if (!shouldSetConnected && (
                (newSeqNum == 0 && prevSeqNum != UINT16_MAX) || (newSeqNum > 0 && newSeqNum - prevSeqNum != 1)
        )) {
            Serial.printf("Dropped packet. SeqNum: curr: %d, prev: %d\n", newSeqNum, prevSeqNum);
            numPacketsDropped++;
        }
        prevSeqNum = newSeqNum;


        auto iter{peers.find(rawIP)};
        // If an unknown peer...
        if (iter == peers.end()) {
            // ...insert it.
            iter = peers.insert(std::make_pair(rawIP, std::make_unique<NetAudioPeer>(incomingPacket))).first;
            auto o{iter->second->getOrigin()};
            Serial.print("\nPeer ");
//            Serial.print(o);
            Serial.print(o.IP);
            Serial.printf(":%" PRIu16, o.Port);
            Serial.print(" connected.\n\n");

            if (o.IP == adapterIP) {
                // Cache the iterator associated with the server.
                server = iter;
            }
        }
        iter->second->handlePacket(incomingPacket);

        // TODO: move into Packet or Peer class?
        if (debugMode >= DebugMode::HEXDUMP_RECEIVE && receivedCount > 0 && receivedCount % 10000 <= 1) {
            Serial.println("Connected peers:");
            for (auto &peer: peers) {
                Serial.println(IPAddress{peer.first});
            }
//            Serial.println(*headerIn);
            Serial.printf("RECEIVE: SeqNumber: %d, BufferSize: %d, NumChannels: %d\n",
                          headerIn->SeqNumber,
                          1 << headerIn->BufferSize,
                          headerIn->NumChannels);
            Serial.print("From: ");
            Serial.print(socket.remoteIP());
            Serial.printf(":%d\n", socket.remotePort());
            Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
            hexDump(packetBuffer, packetSize, true);
        }

        if (shouldSetConnected) {
            driftCheckTimer = 0;
            peerCheckTimer = 0;
            connected = true;
            sampleRate = AUDIO_SAMPLE_RATE_EXACT;
            fs.set(sampleRate);
        }
    }
}

void NetJUCEClient::checkConnectivity() {
    if (peerCheckTimer > 1000 && !peers.empty()) {
        peerCheckTimer = 0;
        for (auto it = peers.cbegin(), next = it; it != peers.cend(); it = next) {
            ++next;
            if (!it->second->isConnected()) {
                auto o{it->second->getOrigin()};
                Serial.print("\nPeer ");
                Serial.print(o.IP);
                Serial.printf(":%" PRIu16, o.Port);
                Serial.print(" disconnected.\n\n");

                if (it == server) {
                    // Clear cached server iterator.
                    server = peers.end();
                }
                peers.erase(it);
            }
        }
        connected = !peers.empty();
    }

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
        socket.stop();
        peers.clear();
        receiveTimer = 0;
        // TODO: address this ambiguity
        joined = false;
        connected = false;
    }
}

void NetJUCEClient::doAudioOutput() {
    if (connected) {
////        Serial.println("Connected (at least one peer exists).");
//        // Try to cache the map iterator for the server.
//        if (server == peers.end()) {
////            Serial.println("Server not cached; attempting to find it.");
//            server = peers.find(adapterIP);
//        } else {
////            Serial.println("Cached server found.");
//        }

        if (server == peers.end()) {
            // Just use the first available peer.
//            peers.begin()->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
//            memset(audioBlock[0], 0, NUM_SOURCES * AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
            // (actually go with silence.)
            for (int ch = 0; ch < NUM_SOURCES; ++ch) {
                memset(audioBlock[ch], 0, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
            }
        } else {
            server->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
        }
    } else {
//        memset(audioBlock[0], 0, NUM_SOURCES * AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        for (int ch = 0; ch < NUM_SOURCES; ++ch) {
            memset(audioBlock[ch], 0, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        }
    }

    if (debugMode >= DebugMode::HEXDUMP_AUDIO_OUT && receivedCount > 0 && receivedCount % 10000 <= 1) {
        Serial.println(F("Audio out, channel 1"));
        hexDump(reinterpret_cast<uint8_t *>(audioBlock[0]), 32);
        Serial.println(F("Audio out, channel 2"));
        hexDump(reinterpret_cast<uint8_t *>(audioBlock[1]), 32);
    }

    audio_block_t *outBlock[NUM_SOURCES];
    auto channelFrameSize{AUDIO_BLOCK_SAMPLES * sizeof(int16_t)};

    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        outBlock[ch] = allocate();
        if (outBlock[ch]) {
            // Copy the samples to the output block.
            memcpy(outBlock[ch]->data, audioBlock[ch], channelFrameSize);

            // Finish up.
            transmit(outBlock[ch], ch);
            release(outBlock[ch]);
        }
    }
}

void NetJUCEClient::handleAudioInput() {
    audio_block_t *inBlock[num_inputs];
    for (int channel = 0; channel < num_inputs; channel++) {
        inBlock[channel] = receiveReadOnly(channel);
        // Only proceed if a block was returned, i.e. something is connected
        // to one of this object's input channels.
        if (inBlock[channel]) {
            outgoingPacket.writeAudioData(channel, inBlock[channel]->data);
            release(inBlock[channel]);
        } else {
            // Fill the packet with zeros.
            outgoingPacket.clearAudioData(channel);
        }
    }

    packetReady = true;
}

void NetJUCEClient::getAudioAndSend() {
    if (!joined /*|| !connected*/) return;

    // Copy audio to the outgoing packet.
    audio_block_t *inBlock[num_inputs];
    for (int channel = 0; channel < num_inputs; channel++) {
        inBlock[channel] = receiveReadOnly(channel);
        // Only proceed if a block was returned, i.e. something is connected
        // to one of this object's input channels.
        if (inBlock[channel]) {
            outgoingPacket.writeAudioData(channel, inBlock[channel]->data);
            release(inBlock[channel]);
        } else {
            // Send zeros.
            outgoingPacket.clearAudioData(channel);
        }
    }

    // TODO: check for failures and such.
    outgoingPacket.writeHeader();
    socket.beginPacket(multicastIP, remotePort);
    socket.write(outgoingPacket.getData(), outgoingPacket.getSize());
    socket.endPacket();

    if (debugMode >= DebugMode::HEXDUMP_SEND && outgoingPacket.getSeqNumber() % 10000 <= 1) {
        Serial.printf("SEND: SeqNumber: %d\n", outgoingPacket.getSeqNumber());
        Serial.print("Destination: ");
        Serial.print(multicastIP);
        Serial.printf(":%d\n", remotePort);
        hexDump(outgoingPacket.getData(), static_cast<int>(outgoingPacket.getSize()), true);
    }

    outgoingPacket.incrementSeqNumber();
}

void NetJUCEClient::send() {
    if (joined && packetReady) {
        packetReady = false;

        // TODO: check for failures and such.
        outgoingPacket.writeHeader();
        socket.beginPacket(multicastIP, remotePort);
        socket.write(outgoingPacket.getData(), outgoingPacket.getSize());
        socket.endPacket();

        if (debugMode >= DebugMode::HEXDUMP_SEND && outgoingPacket.getSeqNumber() % 10000 <= 1) {
            Serial.printf("SEND: SeqNumber: %d\n", outgoingPacket.getSeqNumber());
            Serial.print("Destination: ");
            Serial.print(multicastIP);
            Serial.printf(":%d\n", remotePort);
            hexDump(outgoingPacket.getData(), static_cast<int>(outgoingPacket.getSize()), true);
        }

        outgoingPacket.incrementSeqNumber();
    }
}

void NetJUCEClient::setDebugMode(DebugMode mode) {
    debugMode = mode;
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
