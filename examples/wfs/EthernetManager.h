//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_ETHERNETMANAGER_H
#define NETJUCE_TEENSY_ETHERNETMANAGER_H

#include "Component.h"
#include <TeensyID.h>

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
};


#endif //NETJUCE_TEENSY_ETHERNETMANAGER_H
