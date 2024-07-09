//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_UTILS_H
#define NETJUCE_TEENSY_UTILS_H

#include <IPAddress.h>

#define BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)
#define WAIT_INFINITE() BLOCK_WITH_FORCED_SEMICOLON(while (true) yield();)

class Utils
{
public:
    static float clamp(float value, float min, float max)
    {
        if (value < min) {
//            Serial.printf("value %f < min %f \n", value, min);
            value = min;
        } else if (value > max) {
//            Serial.printf("value %f > max %f \n", value, max);
            value = max;
        }

        return value;
    }
};

#endif //NETJUCE_TEENSY_UTILS_H
