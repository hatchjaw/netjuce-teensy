//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"

NetJUCEClient::NetJUCEClient(IPAddress &networkAdapterIPAddress,
                             IPAddress &multicastIPAddress,
                             uint16_t remotePortNumber,
                             uint16_t localPortNumber,
                             DebugMode debugModeToUse) :
        AudioStream{NUM_SOURCES, new audio_block_t *[NUM_SOURCES]},
        adapterIP{networkAdapterIPAddress},
//        clientIP{networkAdapterIPAddress},
//        gatewayIP{networkAdapterIPAddress},
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

//    threads.setSliceMicros(100);
}

NetJUCEClient::~NetJUCEClient() {
    for (int ch = 0; ch < NUM_SOURCES; ++ch) {
        delete[] audioBlock[ch];
    }
    delete[] audioBlock;
}


bool NetJUCEClient::begin() {
    Serial.printf("DEBUG MODE: %d\n", debugMode);

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
//    clientIP[3] = mac[5];
//
//    gatewayIP[3] = 1;

    joined = true;

    return true;
}

bool NetJUCEClient::isConnected() const {
    return joined;
}

//void NetJUCEClient::connect(uint connectTimeoutMs) {
//    if (!active) {
//        Serial.println(F("Client is not connected to any Teensy audio objects."));
//        delay(connectTimeoutMs);
//        return;
//    }
//
//    Serial.print(F("Joining multicast group at "));
//    Serial.print(multicastIP);
//    Serial.println(F("... "));
//
////    joined = sender.beginMulticast(multicastIP, localPort + 1);
////    if (joined) Serial.println("Began multicast");
////    sender.begin(localPort + 1);
//    joined = udp.listenMulticast(multicastIP, localPort);
//
//    if (joined) {
//        Serial.print("Success! UDP Listening on IP: ");
//        Serial.print(Ethernet.localIP());
//        Serial.printf(" port %d\n", localPort);
//
//        udp.onPacket([this](AsyncUDPPacket packet) {
//            handleIncomingPacket(packet);
////            Serial.print("UDP Packet Type: ");
////            Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
////            Serial.print(", From: ");
////            Serial.print(packet.remoteIP());
////            Serial.print(":");
////            Serial.print(packet.remotePort());
////            Serial.print(", To: ");
////            Serial.print(packet.localIP());
////            Serial.print(":");
////            Serial.print(packet.localPort());
////            Serial.print(", Length: ");
////            Serial.print(packet.length());
////            Serial.print(", Data: ");
////            Serial.write(packet.data(), packet.length());
////            Serial.println();
//        });
//
//        receiveTimer = 0;
//        receivedCount = 0;
//        outgoingPacket.reset();
//    } else {
//        Serial.println(F("Failed to join multicast group."));
//        delay(connectTimeoutMs);
//    }
//}

void NetJUCEClient::update(void) {
    checkConnectivity();

    doAudioOutput();

    send();

    adjustClock();
}

void NetJUCEClient::adjustClock() {
    if (driftCheckTimer > 10000 && !peers.empty()) {
        driftCheckTimer = 0;
        auto peer{peers.find(adapterIP)};
        if (peer != peers.end()) {
            auto drift{1.f};
            {
//                Threads::Scope m(readLock);
                drift = peer->second->getDriftRatio(true);
            }

            // PLL:
            auto fs = AUDIO_SAMPLE_RATE_EXACT * drift;
            Serial.printf("Setting sample rate: %.7f", fs);
            // PLL between 27*24 = 648MHz und 54*24=1296MHz
            int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
            int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

            double C = ((double) fs * 256 * n1 * n2) / 24000000;
            int c0 = C;
            int c2 = 10000;
            int c1 = C * c2 - (c0 * c2);
            Serial.printf(" (C %.9f, nfact %d nmult %d ndiv %d)\n\n", C, c0, c1, c2);
//            CCM_ANALOG_PLL_AUDIO &= CCM_ANALOG_PLL_AUDIO_BYPASS;
//            CCM_ANALOG_PLL_AUDIO &= CCM_ANALOG_PLL_AUDIO_POWERDOWN; // Switch off PLL
            set_audioClock(c0, c1, c2, true);
        }
    }
}

//void NetJUCEClient::receive() {
////    Threads::Scope m(readLock);
//
//    if (!joined) return;
//
////    int packetSize;
////
////    while ((packetSize = socket.parsePacket()) > 0) {
////        ++receivedCount;
////        connected = true;
////        receiveTimer = 0;
////
////        socket.read(packetBuffer, packetSize);
////
////        auto remoteIP{socket.remoteIP()};
////        auto port{socket.remotePort()};
////        auto rawIP{static_cast<uint32_t>(remoteIP)};
////
////        auto headerIn{reinterpret_cast<DatagramAudioPacket::PacketHeader *>(packetBuffer)};
////        auto bytesPerChannel{headerIn->BufferSize * headerIn->BitResolution};
////
////        // There's about to be at least one peer so confirm connectedness.
////        if (peers.empty( {
////            driftCheckTimer = 0;
////            connected = true;
////        }
////
////        incomingPacket.fromRawPacketData(remoteIP, port, packetBuffer);
////        auto iter{peers.find(rawIP)};
////        // If an unknown peer...
////        if (iter == peers.end()) {
////            // ...insert it.
////            iter = peers.insert(std::make_pair(rawIP, std::make_unique<NetAudioPeer>(incomingPacket))).first;
////            auto o{iter->second->getOrigin()};
////            Serial.print("\nPeer ");
////            Serial.print(o.IP);
////            Serial.printf(":%" PRIu16, o.Port);
////            Serial.print(" connected.\n\n");
////        }
////        iter->second->handlePacket(incomingPacket);
////
////        // TODO: move into Packet or Peer class?
////        if (debugMode >= DebugMode::HEXDUMP_RECEIVE && receivedCount > 0 && receivedCount % 10000 <= 1) {
////            Serial.println("Connected peers:");
////            for (auto &peer: peers) {
////                Serial.println(IPAddress{peer.first});
////            }
////            Serial.printf("RECEIVE: SeqNumber: %d, BufferSize: %d, NumChannels: %d\n",
////                          headerIn->SeqNumber,
////                          headerIn->BufferSize,
////                          headerIn->NumChannels);
////            Serial.print("From: ");
////            Serial.print(socket.remoteIP());
////            Serial.printf(":%d\n", socket.remotePort());
////            Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
////            hexDump(packetBuffer, packetSize, true);
////        }
////    }
//}

void NetJUCEClient::checkConnectivity() {
//    Threads::Scope m(readLock);

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
                peers.erase(it);
                connected = !peers.empty();
            }
        }
    }

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
//        udp.close(); // Called by listenMulticast(), so probably no need here
        peers.clear();
        receiveTimer = 0;
        // TODO: fix this ambiguous situation
        joined = false;
        connected = false;
    }
}

void NetJUCEClient::doAudioOutput() {
    if (connected) {
        // Try to use the stream from the server for audio output.
        auto peer{peers.find(adapterIP)};
        if (peer != peers.end()) {
//            Threads::Scope m(readLock);

            peer->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
        } else {
            // Otherwise just use the first available peer.
//            peers.begin()->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
//            memset(audioBlock[0], 0, NUM_SOURCES * AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
            for (int ch = 0; ch < NUM_SOURCES; ++ch) {
                memset(audioBlock[ch], 0, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
            }
        }
    } else {
//        memset(audioBlock[0], 0, NUM_SOURCES * AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
        for (int ch = 0; ch < NUM_SOURCES; ++ch) {
            for (int s = 0; s < AUDIO_BLOCK_SAMPLES; ++s) {
                audioBlock[ch][s] = 0;
            }
        }
    }

    if (debugMode >= DebugMode::HEXDUMP_AUDIO_OUT && receivedCount > 0 && receivedCount % 10000 <= 1) {
        Serial.println(F("Audio out, channel 1"));
        hexDump(reinterpret_cast<uint8_t *>(audioBlock[0]), 64);
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

void NetJUCEClient::send() {
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
        }
    }

    // TODO: check for failures and such.
    outgoingPacket.writeHeader();

//    if (outgoingPacket.getSeqNumber() > 0 && outgoingPacket.getSeqNumber() % 10000 == 0) {
//        sender.beginPacket(multicastIP, remotePort);
//        sender.write(outgoingPacket.getData(), outgoingPacket.getSize());
//        sender.endPacket();

//    Serial.println("About to call writeTo()");
//    udp.writeTo(outgoingPacket.getData(), outgoingPacket.getSize(), multicastIP, remotePort);

//        uint8_t buf[]{"Anyone here?"};
//        udp.writeTo(buf, 13, IPAddress(226, 6, 38, 226), 41814);
//    }

    if (debugMode >= DebugMode::HEXDUMP_SEND && outgoingPacket.getSeqNumber() % 10000 <= 1) {
        Serial.printf("SEND: SeqNumber: %d\n", outgoingPacket.getSeqNumber());
        hexDump(outgoingPacket.getData(), static_cast<int>(outgoingPacket.getSize()), true);
    }

    outgoingPacket.incrementSeqNumber();
}

void NetJUCEClient::setDebugMode(DebugMode mode) {
    debugMode = mode;
}

void NetJUCEClient::handlePacket(IPAddress &remoteIP, uint16_t remotePortItCameFrom, uint8_t *data, size_t size) {
    if (size > 0) {
        ++receivedCount;
        connected = true;
        receiveTimer = 0;

        auto rawIP{static_cast<uint32_t>(remoteIP)};

        auto headerIn{reinterpret_cast<DatagramAudioPacket::PacketHeader *>(data)};
        auto bytesPerChannel{headerIn->BufferSize * headerIn->BitResolution};

        // There's about to be at least one peer so confirm connectedness.
        if (peers.empty()) {
            driftCheckTimer = 0;
            connected = true;
        }

        incomingPacket.fromRawPacketData(remoteIP, remotePortItCameFrom, data);
        auto iter{peers.find(rawIP)};
        // If an unknown peer...
        if (iter == peers.end()) {
            // ...insert it.
            iter = peers.insert(std::make_pair(rawIP, std::make_unique<NetAudioPeer>(incomingPacket))).first;
            auto o{iter->second->getOrigin()};
            Serial.print("\nPeer ");
            Serial.print(o.IP);
            Serial.printf(":%" PRIu16, o.Port);
            Serial.print(" connected.\n\n");
        }
        iter->second->handlePacket(incomingPacket);

        // TODO: move into Packet or Peer class?
        if (debugMode >= DebugMode::HEXDUMP_RECEIVE && receivedCount > 0 && receivedCount % 10000 <= 1) {
            Serial.println("Connected peers:");
            for (auto &peer: peers) {
                Serial.println(IPAddress{peer.first});
            }
            Serial.printf("RECEIVE: SeqNumber: %d, BufferSize: %d, NumChannels: %d\n",
                          headerIn->SeqNumber,
                          headerIn->BufferSize,
                          headerIn->NumChannels);
            Serial.print("From: ");
            Serial.print(remoteIP);
            Serial.printf(":%d\n", remotePortItCameFrom);
            Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
            hexDump(data, static_cast<int>(size), true);
        }

        // This works... why doesn't sending on the audio interrupt?
//        uint8_t buf[50]{};
//        sprintf(reinterpret_cast<char *>(buf), "Got %u bytes of data", packet.length());
//        udp.writeTo(buf, 50, IPAddress(226, 6, 38, 226), 41814);
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