//
// Created by tar on 11/04/23.
//

#ifndef NETJUCE_TEENSY_COMPONENT_H
#define NETJUCE_TEENSY_COMPONENT_H

#include <typeinfo>
#include <ProgramContext.h>

class Component : public Printable {
public:
    explicit Component(ProgramContext &c) : context(c) {};

    virtual ~Component() = default;

    bool begin();

    virtual void loop() = 0;

    virtual void printError();

    virtual size_t printTo(Print& p) const = 0;

protected:
    ProgramContext &context;
    char error[128];

private:
    virtual bool init() = 0;
};


#endif //NETJUCE_TEENSY_COMPONENT_H
