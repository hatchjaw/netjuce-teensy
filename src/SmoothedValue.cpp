//
// Created by tar on 3/5/23.
//

#include "SmoothedValue.h"

template<typename T>
SmoothedValue<T>::SmoothedValue(T initialValue) {
    target = initialValue;
    current = target;
}

template<typename T>
void SmoothedValue<T>::set(T targetValue, bool skipSmoothing) {
    target = targetValue;
    if (skipSmoothing) {
        current = target;
    }
}

template<typename T>
T SmoothedValue<T>::getNext() {
    if (current != target) {
        auto delta = target - current;
        auto absDelta = abs(delta);
        if (absDelta < THRESHOLD) {
            current = target;
        } else {
            current += MULTIPLIER * delta;
        }
    }

    return current;
}

template<typename T>
T &SmoothedValue<T>::getCurrent() {
    return current;
}