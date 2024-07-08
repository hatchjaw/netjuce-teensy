//
// Created by tar on 12/04/23.
//

#include "EthernetManager.h"

bool EthernetManager::init() {
    localIP = context.clientSettings.adapterIP;

    Ethernet.macAddress(mac);
    Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // TODO: check whether resulting clientIP is the same as the server.
    // TODO: also check for collisions between clients... adjust as necessary?...
    localIP[2] = mac[4];
    localIP[3] = mac[5];

    Ethernet.onLinkState([this](bool state) {
        Serial.printf("[Ethernet] Link %s\n", state ? "ON" : "OFF");

        if (state) {
            Serial.print("IP: ");
            Serial.println(Ethernet.localIP());

            context.ethernetReady = true;
        } else {
            context.ethernetReady = false;
        }
    });

    Ethernet.begin(localIP, netmask, gatewayIP);
}

size_t EthernetManager::printTo(Print &p) const {
    return p.print("EthernetManager");
}
