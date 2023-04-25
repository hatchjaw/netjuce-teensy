//
// Created by Tommy Rushton on 22/02/2023.
//

#include "NetJUCEClient.h"
#include <TeensyID.h>
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

    teensyMAC(mac);

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
    clientIP[2] = mac[4];
    clientIP[3] = mac[5];

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
    Serial.printf("DEBUG MODE: %d\n", debugMode);

    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    EthernetClass::begin(mac, clientIP);

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
    return joined;
}

void NetJUCEClient::connect(uint connectTimeoutMs) {
    if (!active) {
        Serial.println(F("Client is not connected to any Teensy audio objects."));
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
//    receive();

//    checkConnectivity();

    doAudioOutput();

    handleAudioInput();

//    getAudioAndSend();

//    adjustClock();
}

void NetJUCEClient::loop() {
    receive();

    checkConnectivity();

    send();

    adjustClock();
}

void NetJUCEClient::adjustClock() {
    if (driftCheckTimer > 10000 && !peers.empty()) {
        driftCheckTimer = 0;

        // Try to cache the map iterator for the server.
        if (server == peers.end()) {
            server = peers.find(adapterIP);
        }
        if (server != peers.end()) {
            auto drift = server->second->getDriftRatio(true);

            // PLL4:
            // Defaults are: DIV = 28, NUM = 2240, DENOM = 10000
            // PLL4 MAIN CLOCK = 24000000 * (DIV + NUM/DENOM) = 677.3592 MHz
            // 677.3592 MHz /4 /15 = 11.2896 MHz = 256 * 44100
            // But this doesn't quite make sense, as it will tend toward a
            // situation where writes == reads, thus fs = 44100, which it isn't,
            // at least not relative to the server, which is treated as
            // authoritative. At best, it's a reactive system; maybe that's
            // enough for now.
            //
            // Also, if a client is disconnected and reconnected before it has
            // time to recognise that it's not still on the network, reads
            // continue to accumulate while writes stay static, and an
            // inaccurate drift ratio results.
            auto fs = AUDIO_SAMPLE_RATE_EXACT * drift;
            Serial.printf("Setting sample rate: %.7f", fs);
            int sai1PRED = 4;
            // This is lifted directly from output_i2s.cpp.
            int sai1PODF = 1 + (24000000 * 27) / (fs * 256 * sai1PRED); // Should evaluate to 15

            double C = ((double) fs * 256 * sai1PRED * sai1PODF) / 24000000;
            int div = C;
            int denom = 10000;
            int num = C * denom - (div * denom);
            Serial.printf(" (C %.9f, div %d num %d denom %d)\n\n", C, div, num, denom);
            set_audioClock(div, num, denom, true);
        }
    }
}

void NetJUCEClient::receive() {
    if (!joined) return;

    int packetSize;

    while ((packetSize = socket.parsePacket()) > 0) {
        ++receivedCount;
//        connected = true;
        receiveTimer = 0;

        socket.read(packetBuffer, packetSize);

        auto remoteIP{socket.remoteIP()};
        auto port{socket.remotePort()};
        auto rawIP{static_cast<uint32_t>(remoteIP)};

        auto headerIn{reinterpret_cast<DatagramAudioPacket::PacketHeader *>(packetBuffer)};
        auto bytesPerChannel{headerIn->BufferSize * headerIn->BitResolution};

        // There's about to be at least one peer so confirm connectedness.
        auto shouldSetConnected{peers.empty()};

        incomingPacket.fromRawPacketData(remoteIP, port, packetBuffer);
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
                          headerIn->BufferSize,
                          headerIn->NumChannels);
            Serial.print("From: ");
            Serial.print(socket.remoteIP());
            Serial.printf(":%d\n", socket.remotePort());
            Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
            hexDump(packetBuffer, packetSize, true);
        }

        // Set connectedness _after_ first packet has been handled and peer is
        // set up correctly, to avoid a race condition with doAudioOutput().
        // NB. doesn't work.
        if (shouldSetConnected) {
            driftCheckTimer = 0;
            peerCheckTimer = 0;
            connected = true;
        }
    }
}

void NetJUCEClient::checkConnectivity() {
//    Serial.print("peerCheckTimer: ");
//    Serial.print(peerCheckTimer);
//    Serial.printf(peers.empty() ? ", empty\n" : ", not empty\n");
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
        // Try to cache the map iterator for the server.
        if (server == peers.end()) {
            server = peers.find(adapterIP);
        }

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
            // Send zeros.
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
