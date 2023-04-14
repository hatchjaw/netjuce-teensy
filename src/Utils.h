//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_UTILS_H
#define NETJUCE_TEENSY_UTILS_H

#define BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)
#define WAIT_INFINITE() BLOCK_WITH_FORCED_SEMICOLON(while (true) yield();)

#include <Arduino.h>

class Utils {
public:
    static void clamp(float &value, float min, float max) {
        if (value < min) {
            Serial.printf("value %f < min %f \n", value, min);
            value = min;
        } else if (value > max) {
            Serial.printf("value %f > max %f \n", value, max);
            value = max;
        }
    }
};

#endif //NETJUCE_TEENSY_UTILS_H
