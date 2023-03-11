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

    T &getCurrent();

private:
    static constexpr T MULTIPLIER{.05f}, THRESHOLD{1e-5};
    T current, target;
};

template
class SmoothedValue<float>;

#endif //NETJUCE_TEENSY_SMOOTHEDVALUE_H
