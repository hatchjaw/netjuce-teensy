//
// Created by tar on 14/04/23.
//

#ifndef NETJUCE_TEENSY_CONTEXTMANAGER_H
#define NETJUCE_TEENSY_CONTEXTMANAGER_H

#include "Component.h"

class ContextManager : public Component {
public:
    using Component::Component;

    void loop() override;

    virtual size_t printTo(Print &p) const override;

private:
    bool init() override;
};


#endif //NETJUCE_TEENSY_CONTEXTMANAGER_H
