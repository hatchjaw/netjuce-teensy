//
// Created by tar on 12/04/23.
//

#include "AudioManager.h"

AudioManager::AudioManager(ProgramContext &c) :
        Component(c),
        njc(c.serverIP, c.multicastIP, c.remotePort, c.localPort, DebugMode::NONE) {}

size_t AudioManager::printTo(Print &p) const {
    return p.print("AudioManager");
}

bool AudioManager::init() {
//    patchCords.push_back(std::make_unique<AudioConnection>(njc, 0, out, 0));
//    patchCords.push_back(std::make_unique<AudioConnection>(njc, 1, out, 1));

    for (int i = 0; i < context.numSources; ++i) {
        // Network inputs to WFS inputs.
        patchCords.push_back(std::make_unique<AudioConnection>(njc, i, wfs, i));
        // Network inputs to network outputs.
        patchCords.push_back(std::make_unique<AudioConnection>(njc, i, njc, i));
    }

    // WFS outputs to I2S outputs.
    patchCords.push_back(std::make_unique<AudioConnection>(wfs, 0, out, 0));
    patchCords.push_back(std::make_unique<AudioConnection>(wfs, 1, out, 1));

    AudioMemory(32);
    audioShield.enable();
    audioShield.volume(.8);

    context.moduleID.onChange = [this](int value) { wfs.setParamValue("moduleID", value); };
    context.speakerSpacing.onChange = [this](float value) { wfs.setParamValue("spacing", value); };
    for (auto &sp: context.sourcePositions) {
        // If smoothing in Faust with si.smoo:
        sp.second.onSet = [this, sp](double value) {
//            Serial.printf("Updating %s: %f\n", sp.first.c_str(), value);
            wfs.setParamValue(sp.first, value);
        };
        // If smoothing outside of Faust:
//        sp.second.onChange = [this, sp](double value) {
////            Serial.printf("%s changed: %.9f\n", sp.first.c_str(), value);
////            Utils::clamp(value, 0.f, 1.f);
//            wfs.setParamValue(sp.first, value);
//        };
    }

    return true;
}

void AudioManager::loop() {
    if (!njc.isConnected()) {
        njc.connect(2500);
    } else {
        njc.loop();
    }

    if (usageReportTimer > USAGE_REPORT_INTERVAL) {
        Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                      AudioMemoryUsage(),
                      AudioProcessorUsage());
        usageReportTimer = 0;
    }
}
