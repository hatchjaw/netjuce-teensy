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
//        audioBuffer{NUM_SOURCES, CIRCULAR_BUFFER_SIZE},
        audioBlock{new int16_t *[NUM_SOURCES]},
        outgoingPacket(NUM_SOURCES, AUDIO_BLOCK_SAMPLES, AUDIO_SAMPLE_RATE_EXACT),
        incomingPacket(NUM_SOURCES, AUDIO_BLOCK_SAMPLES, AUDIO_SAMPLE_RATE_EXACT),
        debugMode(debugModeToUse) {

    teensyMAC(mac);

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
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

    Serial.print(F("Joining multicast group at "));
    Serial.print(multicastIP);
    Serial.printf(F(", listening on port %d... "), localPort);

    joined = socket.beginMulticast(multicastIP, localPort);

    if (joined) {
        Serial.println(F("Success!"));
        receiveTimer = 0;
        receivedCount = 0;
        outgoingPacket.reset();
    } else {
        Serial.println(F("Failed."));
        delay(connectTimeoutMs);
    }
}

void NetJUCEClient::update(void) {
    receive();

    checkConnectivity();

    doAudioOutput();

    send();

    // Adjust clock
    if (driftCheckTimer > 10000 && !peers.empty()) {
        driftCheckTimer = 0;
        auto peer{peers.find(adapterIP)};
        if (peer != peers.end()) {
            auto drift{peer->second->getDriftRatio(true)};

            //PLL:
            int fs = static_cast<int>(roundf(AUDIO_SAMPLE_RATE_EXACT * drift));
            Serial.printf("Setting sample rate: %d", fs);
            // PLL between 27*24 = 648MHz und 54*24=1296MHz
            int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
            int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

            double C = ((double) fs * 256 * n1 * n2) / 24000000;
            int c0 = C;
            int c2 = 10000;
            int c1 = C * c2 - (c0 * c2);
            Serial.printf(" (nfact %d nmult %d ndiv %d)\n", c0, c1, c2);
            set_audioClock(c0, c1, c2, true);
        }
    }
}

void NetJUCEClient::receive() {
    if (!joined) return;

    int packetSize;

    while ((packetSize = socket.parsePacket()) > 0) {
        ++receivedCount;
        connected = true;
        receiveTimer = 0;

        socket.read(packetBuffer, packetSize);

        auto remoteIP{socket.remoteIP()};
        auto port{socket.remotePort()};
        auto rawIP{static_cast<uint32_t>(remoteIP)};

        auto headerIn{reinterpret_cast<DatagramAudioPacket::PacketHeader *>(packetBuffer)};
        auto bytesPerChannel{headerIn->BufferSize * headerIn->BitResolution};

//        // Naive approach
//        // Assume 16-bit; TODO: should generalise
//        const int16_t *audio[headerIn->NumChannels];
//        for (int ch = 0; ch < headerIn->NumChannels; ++ch) {
//            audio[ch] = reinterpret_cast<int16_t *>(packetBuffer + PACKET_HEADER_SIZE + ch * bytesPerChannel);
//        }
//        audioBuffer.write(audio, headerIn->BufferSize);

//        // Using map of buffers:
//        auto it{audioBuffers.find(rawIP)};
//        if (it == audioBuffers.end()) {
//            connected = true;
//            it = audioBuffers.insert(std::make_pair(remoteIP, new CircularBuffer<int16_t>(NUM_SOURCES,
//                                                                                          CIRCULAR_BUFFER_SIZE))).first;
//        }
//        it->second->write(audio, headerIn->BufferSize);

        // Using map of Peers:
//        DatagramAudioPacket packet{remoteIP, port, packetBuffer};
        incomingPacket.fromRawPacketData(remoteIP, port, packetBuffer);
        auto iter{peers.find(rawIP)};
        if (iter == peers.end()) { // If an unknown peer...
            // ... there's definitely at least one peer now.
            connected = true;
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
            Serial.print(socket.remoteIP());
            Serial.printf(":%d\n", socket.remotePort());
            Serial.printf("Bytes per channel: %d\n", bytesPerChannel);
            hexDump(packetBuffer, packetSize, true);
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
                peers.erase(it);
                connected = !peers.empty();
            }
        }
    }

    if (receiveTimer > kReceiveTimeoutMs) {
        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), kReceiveTimeoutMs);
        socket.stop();
//        audioBuffer.clear();
//        for (auto b: audioBuffers) {
//            delete b.second;
//        }
//        audioBuffers.clear();
        peers.clear();
        receiveTimer = 0;
        // TODO: fix this ambiguous situation
        joined = false;
        connected = false;
    }
}

void NetJUCEClient::doAudioOutput() {
//    if (connected) audioBuffer.read(audioBlock, AUDIO_BLOCK_SAMPLES);
//    if (connected) audioBuffers.begin()->second->read(audioBlock, AUDIO_BLOCK_SAMPLES);
//    if (connected) peers.begin()->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
    if (connected) {
        // Do output if the server is found. (Could use any available peer...)
        auto peer{peers.find(adapterIP)};
        if (peer != peers.end()) {
            peer->second->getNextAudioBlock(audioBlock, AUDIO_BLOCK_SAMPLES);
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
    if (!joined || !connected) return;

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
    socket.beginPacket(multicastIP, remotePort);
    socket.write(outgoingPacket.getData(), outgoingPacket.getSize());
    socket.endPacket();

    if (debugMode >= DebugMode::HEXDUMP_SEND && outgoingPacket.getSeqNumber() % 10000 <= 1) {
        Serial.printf("SEND: SeqNumber: %d\n", outgoingPacket.getSeqNumber());
        hexDump(outgoingPacket.getData(), static_cast<int>(outgoingPacket.getSize()), true);
    }

    outgoingPacket.incrementSeqNumber();
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
