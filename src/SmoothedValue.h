//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_SMOOTHEDVALUE_H
#define NETJUCE_TEENSY_SMOOTHEDVALUE_H


#include <Arduino.h>
#include <functional>
#include <Utils.h>

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

/**
 * A low-pass filter to smooth parameter changes. See Faust's si.smooth.
 * @tparam T
 */
template<typename T>
class SmoothedValue_V2 {
public:
    explicit SmoothedValue_V2(T initialValue, float smoothness, T threshold = 1e-9) :
            x(initialValue),
            yPrev(initialValue),
            deltaThreshold(threshold),
            s(Utils::clamp(smoothness, 0.f, 1.f)) {}

    void set(T targetValue) {
        x = targetValue;

        if (onSet != nullptr) {
            onSet(x);
        }
    }

    T getNext() {
        T y;
        if (abs(yPrev - x) < deltaThreshold) {
            yPrev = x;
            y = yPrev;
        } else {
            // 1-pole lowpass: y[n] = (1 - s) * x[n] + s * y[n - 1]
            auto ts{static_cast<T>(s)};
            y = (1.f - ts) * x + ts * yPrev;
//            Serial.printf("y = %.9f; yPrev = %.9f\n", y, yPrev);
            yPrev = y;

            if (onChange != nullptr) {
                onChange(y);
            }
        }

        return y;
    }

    std::function<void(T newValue)> onSet;
    std::function<void(T currentValue)> onChange;

private:
    T x, yPrev, deltaThreshold;
    float s;
};

#endif //NETJUCE_TEENSY_SMOOTHEDVALUE_H
