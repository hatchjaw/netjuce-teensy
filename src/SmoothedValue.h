//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_SMOOTHEDVALUE_H
#define NETJUCE_TEENSY_SMOOTHEDVALUE_H


#include <Arduino.h>
#include <functional>

template<typename T>
class SmoothedValue {
public:
    explicit SmoothedValue(T initialValue, double multiplier = .1, double threshold = 1e-9);

    void set(T targetValue, bool skipSmoothing = false);

    T getNext();

    T getCurrent();

    std::function<void(T currentValue)> onChange;

private:
    double current, target, deltaMultiplier, deltaThreshold;
};

template
class SmoothedValue<float>;

#endif //NETJUCE_TEENSY_SMOOTHEDVALUE_H
