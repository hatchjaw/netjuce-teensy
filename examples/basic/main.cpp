#include <Audio.h>
#include "NetJUCEClient.h"
#include "Utils.h"
#include "ClientSettings.h"

// Wait for a serial connection before proceeding with execution
//#define WAIT_FOR_SERIAL
//#undef WAIT_FOR_SERIAL

ClientSettings settings{
        {192, 168, 10, 10},
        {224, 4, 224, 4}
};

//AudioOutputUSB usb;
AudioControlSGTL5000 audioShield;

AudioOutputI2S out;
NetJUCEClient client{settings};

AudioConnection patchCord1(client, 0, out, 0);
AudioConnection patchCord2(client, 1, out, 1);
AudioConnection patchCord3(client, 0, client, 0);
AudioConnection patchCord4(client, 1, client, 1);

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (CrashReport) {  // Print any crash report
        Serial.println(CrashReport);
        CrashReport.clear();
    }

    AudioMemory(32);
    audioShield.enable();
    audioShield.volume(.5);

    if (!client.begin()) {
        WAIT_INFINITE();
    }
}

void loop() {
    client.loop();
}
