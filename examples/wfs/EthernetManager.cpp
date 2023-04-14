//
// Created by tar on 12/04/23.
//

#include "EthernetManager.h"

bool EthernetManager::init() {
    teensyMAC(mac);

    localIP = context.serverIP;

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
    localIP[2] = mac[4];
    localIP[3] = mac[5];

    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    EthernetClass::begin(mac, localIP);

    if (EthernetClass::linkStatus() != LinkON) {
        sprintf(error, "Ethernet link could not be established.");
//        Serial.println("Ethernet link could not be established.");
        return false;
    } else {
        Serial.print(F("IP: "));
        Serial.println(EthernetClass::localIP());
        return true;
    }
}

size_t EthernetManager::printTo(Print &p) const {
    return p.print("EthernetManager");
}