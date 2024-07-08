//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_ETHERNETMANAGER_H
#define NETJUCE_TEENSY_ETHERNETMANAGER_H

#include <QNEthernet.h>
#include "Component.h"

using namespace qindesign::network;

class EthernetManager : public Component {
public:
    using Component::Component;

    void loop() override {};

    virtual size_t printTo(Print &p) const override;

private:
    bool init() override;

    /**
     * MAC address to assign to Teensy's ethernet shield.
     */
    byte mac[6]{};
    /**
     * IP to assign to Teensy.
     */
    IPAddress localIP;
    IPAddress gatewayIP{192, 168, 10, 1}, netmask{255, 255, 255, 0};
};


#endif //NETJUCE_TEENSY_ETHERNETMANAGER_H
