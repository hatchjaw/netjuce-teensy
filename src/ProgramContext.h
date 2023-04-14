//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_PROGRAMCONTEXT_H
#define NETJUCE_TEENSY_PROGRAMCONTEXT_H

#include <NativeEthernet.h>
#include <unordered_map>
#include <SmoothedValue.h>
#include <functional>

template<typename T>
struct Listenable {
    Listenable() = default;

    explicit Listenable(T v) : value(v) {};

    std::function<void(T val)> onChange;

    Listenable &operator=(T newValue) {
        if (value != newValue) {
            value = newValue;
            if (onChange != nullptr) {
                onChange(value);
            }
        }
        return *this;
    };

private:
    T value;
};

template
struct Listenable<int>;

using SourcePositionsMap = std::unordered_map<std::string, SmoothedValue<float>>;

struct ProgramContext {
    IPAddress serverIP;
    IPAddress multicastIP;
    uint16_t audioPort;
    uint16_t oscPort;
    uint16_t localPort;
    uint16_t remotePort;
    int numSources;
    Listenable<int> moduleID;
    SourcePositionsMap sourcePositions;
};

#endif //NETJUCE_TEENSY_PROGRAMCONTEXT_H
