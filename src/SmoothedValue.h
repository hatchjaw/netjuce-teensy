//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_SMOOTHEDVALUE_H
#define NETJUCE_TEENSY_SMOOTHEDVALUE_H


#include <Arduino.h>

template<typename T>
class SmoothedValue {
public:
    explicit SmoothedValue(T initialValue);

    void set(T targetValue, bool skipSmoothing = false);

    T getNext();

    T getCurrent();

private:
    static constexpr double MULTIPLIER{.1}, THRESHOLD{1e-9};
    double current, target;
};

template
class SmoothedValue<float>;

#endif //NETJUCE_TEENSY_SMOOTHEDVALUE_H
