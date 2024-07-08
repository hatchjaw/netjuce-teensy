//
// Created by tar on 11/04/23.
//

#include "OscReceiver.h"


size_t OscReceiver::printTo(Print &p) const {
    return p.print("OscReceiver");
}

bool OscReceiver::init() {
//    while(!context.ethernetReady) {
//        Serial.println("Waiting for ethernet...");
//        delay(1000);
//    }

    auto result{1 == udp.beginMulticast(context.clientSettings.multicastIP, context.oscPort)};

    if (!result) {
        sprintf(error, "OscReceiver: Failed to join multicast group.");
    } else {
        Serial.print("OscReceiver: joined multicast group at ");
        Serial.print(context.clientSettings.multicastIP);
        Serial.printf(", listening on port %d\n", context.oscPort);
    }

    return result;
}

void OscReceiver::loop() {
    int size;

    if ((size = udp.parsePacket()) > 0) {
//        Serial.printf("OSC packet size: %d\n", size);
        uint8_t buffer[size];
        udp.read(buffer, size);

        // Try to read as bundle
        bundleIn.empty();
        bundleIn.fill(buffer, size);
        if (!bundleIn.hasError() && bundleIn.size() > 0) {
//            Serial.printf("OSCBundle::size: %d\n", bundleIn.size());

            bundleIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
            bundleIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
            bundleIn.route("/spacing", [this](OSCMessage &msg, int addrOffset) { parseSpacing(msg, addrOffset); });
        } else {
            // Try as message
            messageIn.empty();
            messageIn.fill(buffer, size);
            if (!messageIn.hasError() && messageIn.size() > 0) {
//                Serial.printf("OSCMessage::size: %d\n", messageIn.size());

                messageIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
                messageIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
                messageIn.route("/spacing", [this](OSCMessage &msg, int addrOffset) { parseSpacing(msg, addrOffset); });
            }
        }
    }
}

void OscReceiver::parsePosition(OSCMessage &msg, int addrOffset) {
    // Get the source index and coordinate axis, e.g. "0/x"
    char path[20];
    msg.getAddress(path, addrOffset + 1);
    // Get the coordinate value (0-1).
    auto pos = msg.getFloat(0);
//    Serial.printf("Receiving \"%s\": %f\n", path, pos);
    // Set the parameter.
    auto it{context.sourcePositions.find(path)};
    if (it != context.sourcePositions.end()) {
        it->second.set(pos);
    }
}

void OscReceiver::parseSpacing(OSCMessage &msg, int addrOffset) {
    auto spacing = msg.getFloat(0);
    Serial.printf("Receiving \"spacing\": %f\n", spacing);
    context.speakerSpacing = spacing;
}

/**
 * TODO: maybe switch to uint32 for representing IPs
 * (dependent on whether this is practical in JUCE).
 * @param msg
 * @param addrOffset
 */
void OscReceiver::parseModule(OSCMessage &msg, int addrOffset) {
    char ipString[15];
    IPAddress ip;
    msg.getString(0, ipString, 15);
    ip.fromString(ipString);
    if (ip == Ethernet.localIP()) {
        char id[2];
        msg.getAddress(id, addrOffset + 1);
        auto numericID = strtof(id, nullptr);
        Serial.printf("Receiving module ID: %f\n", numericID);
        context.moduleID = numericID;
    }
}
