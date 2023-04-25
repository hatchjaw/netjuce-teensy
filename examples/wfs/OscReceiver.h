//
// Created by tar on 11/04/23.
//

#ifndef NETJUCE_TEENSY_OSCRECEIVER_H
#define NETJUCE_TEENSY_OSCRECEIVER_H

#include "Component.h"
#include <functional>
#include <NativeEthernet.h>
#include <OSCBundle.h>

class OscReceiver : public Component {
public:
    using Component::Component;

    void loop() override;

    virtual size_t printTo(Print& p) const override;

private:
    bool init() override;

    void parseModule(OSCMessage &msg, int addrOffset);

    void parseSpacing(OSCMessage &msg, int addrOffset);

    void parsePosition(OSCMessage &msg, int addrOffset);

    EthernetUDP udp;
    OSCBundle bundleIn;
    OSCMessage messageIn;
};


#endif //NETJUCE_TEENSY_OSCRECEIVER_H
