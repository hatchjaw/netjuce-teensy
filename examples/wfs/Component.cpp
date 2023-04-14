//
// Created by tar on 12/04/23.
//

#include "Component.h"

void Component::printError() {
    Serial.println(error);
}

bool Component::begin() {
    auto result{init()};

    Serial.print(*this);

    if (result) {
        Serial.printf(": began successfully.\n");
    } else {
        Serial.printf(": failed to begin: %s\n", error);
    }

    return result;
}