//
// Created by tar on 12/04/23.
//

#ifndef NETJUCE_TEENSY_AUDIOMANAGER_H
#define NETJUCE_TEENSY_AUDIOMANAGER_H

#include "Component.h"
#include <vector>
#include <Audio.h>
#include "WFS/WFS.h"
#include <NetJUCEClient.h>
#include <Utils.h>

class AudioManager : public Component {
public:
    AudioManager(ProgramContext &);

    void loop() override;

    virtual size_t printTo(Print& p) const override;

private:
    bool init() override;

    static constexpr int USAGE_REPORT_INTERVAL{5000};
    elapsedMillis usageReportTimer;

    AudioControlSGTL5000 audioShield;
    AudioOutputI2S out;
    NetJUCEClient njc;
    WFS wfs;
    std::vector<std::unique_ptr<AudioConnection>> patchCords;
};


#endif //NETJUCE_TEENSY_AUDIOMANAGER_H
