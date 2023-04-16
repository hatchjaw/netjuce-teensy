//
// Created by tar on 11/04/23.
//

#include "OscReceiver.h"


size_t OscReceiver::printTo(Print &p) const {
    return p.print("OscReceiver");
}

bool OscReceiver::init() {
    auto result{1 == udp.beginMulticast(context.oscMulticastIP, context.oscPort)};

    if (!result) {
        sprintf(error, "OscReceiver: Failed to join multicast group.");
    }

    return result;
}

void OscReceiver::loop() {
    int size;
    // Not clear whether parameter changes sometimes cause Teensy to crash
    // due to Ethernet getting overloaded, or because of Serial.print debugging
    // from different interrupts.
    if ((size = udp.parsePacket()) > 0) {
//    if (recvInterval > 100 && (size = udp.parsePacket()) > 0) {
        recvInterval = 0;
//        Serial.printf("Packet size: %d\n", size);
        uint8_t buffer[size];
        udp.read(buffer, size);

        // Try to read as bundle
        bundleIn.empty();
        bundleIn.fill(buffer, size);
        if (!bundleIn.hasError() && bundleIn.size() > 0) {
//            Serial.printf("OSCBundle::size: %d\n", bundleIn.size());

            bundleIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
            bundleIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
        } else {
            // Try as message
            messageIn.empty();
            messageIn.fill(buffer, size);
            if (!messageIn.hasError() && messageIn.size() > 0) {
//                Serial.printf("OSCMessage::size: %d\n", messageIn.size());

                messageIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
                messageIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
            }
        }
    }
}

void OscReceiver::parsePosition(OSCMessage &msg, int addrOffset) {
    // Get the source index and coordinate axis, e.g. "0/x"
    char path[20];
    msg.getAddress(path, addrOffset + 1);
    // Rough-and-ready check to prevent attempting to set an invalid source
    // position.
//    auto sourceIdx{atoi(path)};
//    if (sourceIdx >= jtc.getNumChannels()) {
//        Serial.printf("Invalid source index: %d\n", sourceIdx);
//        return;
//    }
    // Get the coordinate value (0-1).
    auto pos = msg.getFloat(0);
    Serial.printf("Setting \"%s\": %f\n", path, pos);
    // Set the parameter.
//    auto it{context.sourcePositions.find(path)};
//    if (it == context.sourcePositions.end()) {
//        context.sourcePositions.insert(std::make_pair(path, SmoothedValue<float>{pos}));
//    } else {
//        it->second.set(pos);
//    }

    auto it{context.sourcePositions.find(path)};
    if (it != context.sourcePositions.end()) {
        it->second.set(pos);
    }
}

void OscReceiver::parseModule(OSCMessage &msg, int addrOffset) {
    char ipString[15];
    IPAddress ip;
    msg.getString(0, ipString, 15);
    ip.fromString(ipString);
    if (ip == EthernetClass::localIP()) {
        char id[2];
        msg.getAddress(id, addrOffset + 1);
        auto numericID = strtof(id, nullptr);
        Serial.printf("Setting module ID: %f\n", numericID);
        context.moduleID = numericID;
    }
}
