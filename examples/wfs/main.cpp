//
// Created by tar on 11/04/23.
//

#include <Utils.h>
#include <vector>
#include <memory>
#include "Component.h"
#include "ContextManager.h"
#include "AudioManager.h"
#include "EthernetManager.h"
#include "OscReceiver.h"

#define WAIT_FOR_SERIAL

IPAddress multicastIP{226, 6, 38, 226};
IPAddress adapterIP{192, 168, 10, 10};

ProgramContext context;
std::vector<std::unique_ptr<Component>> components;

//elapsedMicros e;

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (CrashReport) {
        Serial.println(CrashReport);
        CrashReport.clear();
    }

    context.multicastIP = multicastIP;
    context.serverIP = adapterIP;
    context.remotePort = DEFAULT_REMOTE_PORT;
    context.localPort = DEFAULT_LOCAL_PORT;
    context.audioPort = DEFAULT_LOCAL_PORT;
    context.oscPort = DEFAULT_LOCAL_PORT + 1;
    context.moduleID = 0;
    context.numSources = NUM_SOURCES;

    components.push_back(std::make_unique<ContextManager>(context));
    components.push_back(std::make_unique<EthernetManager>(context));
    components.push_back(std::make_unique<AudioManager>(context));
    components.push_back(std::make_unique<OscReceiver>(context));

    for (auto &c: components) {
        if (!c->begin()) {
            WAIT_INFINITE();
        }
    }
}

void loop() {
//    Serial.println(e);
//    e = 0;

    for (auto &c: components) {
        c->loop();
    }
}