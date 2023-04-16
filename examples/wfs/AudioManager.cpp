//
// Created by tar on 12/04/23.
//

#include "AudioManager.h"

AudioManager::AudioManager(ProgramContext &c) : Component(c), njc(c) {}

size_t AudioManager::printTo(Print &p) const {
    return p.print("AudioManager");
}

bool AudioManager::init() {
//    patchCords.push_back(std::make_unique<AudioConnection>(njc, 0, out, 0));
//    patchCords.push_back(std::make_unique<AudioConnection>(njc, 1, out, 1));

    patchCords.push_back(std::make_unique<AudioConnection>(wfs, 0, out, 0));
    patchCords.push_back(std::make_unique<AudioConnection>(wfs, 1, out, 1));
    for (int i = 0; i < context.numSources; ++i) {
        patchCords.push_back(std::make_unique<AudioConnection>(njc, i, wfs, i));
        patchCords.push_back(std::make_unique<AudioConnection>(njc, i, njc, i));
    }

    AudioMemory(32);
    audioShield.enable();
    audioShield.volume(.7);

    context.moduleID.onChange = [this](int value) { wfs.setParamValue("moduleID", value); };
    for (auto &sp: context.sourcePositions) {
        sp.second.onChange = [this, sp](float value) {
//            Serial.printf("%s changed: %.9f\n", sp.first.c_str(), value);
            Utils::clamp(value, 0.f, 1.f);
            wfs.setParamValue(sp.first, value);
        };
    }

    return true;
}

void AudioManager::loop() {
    if (!njc.isConnected()) {
        njc.connect(2500);
    }

    if (usageReportTimer > USAGE_REPORT_INTERVAL) {
        Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                      AudioMemoryUsage(),
                      AudioProcessorUsage());
        usageReportTimer = 0;
    }
}