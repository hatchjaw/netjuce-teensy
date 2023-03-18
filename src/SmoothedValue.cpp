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
//            Serial.printf("current (%f) = target (%f)\n", current, target);
        } else {
            current += MULTIPLIER * delta;
//            Serial.printf("target = %.9f, current += %.1f * %.9f = %.9f\n", target, MULTIPLIER, delta, current);
        }
    }

    return static_cast<T>(current);
}

template<typename T>
T SmoothedValue<T>::getCurrent() {
    return static_cast<T>(current);
}