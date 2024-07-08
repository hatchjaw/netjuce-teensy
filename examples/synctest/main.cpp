#include <Audio.h>
#include <NetJUCEClient.h>
#include <Utils.h>
#include "SyncTester/SyncTester.h"

// Wait for a serial connection before proceeding with execution
//#define WAIT_FOR_SERIAL
//#undef WAIT_FOR_SERIAL

ClientSettings settings{
        // Remote server IP address -- should match address in IPv4 settings.
        {192, 168, 10, 10},
        // Multicast group IP address.
        {224, 4, 224, 4}
};

// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

NetJUCEClient client{settings};
SyncTester st;

// Send input from server back to server.
AudioConnection patchCord10(client, 0, client, 0);
// Send input from server to audio output.
AudioConnection patchCord20(client, 0, out, 0);
// Send input to synchronicity tester.
AudioConnection patchCord30(client, 0, st, 0);
// Combine with generated sawtooth and send to output.
AudioConnection patchCord40(st, 0, out, 1);
// Send synchronicity measure back to server.
AudioConnection patchCord50(st, 0, client, 1);

elapsedMillis performanceReport;
const uint32_t PERF_REPORT_INTERVAL = 5000;

//region Forward declarations
void startAudio();
//endregion

void setup() {
#ifdef WAIT_FOR_SERIAL
    while (!Serial);
#endif

    if (CrashReport) {  // Print any crash report
        Serial.println(CrashReport);
        CrashReport.clear();
    }

    Serial.printf("Sampling rate: %f\n", AUDIO_SAMPLE_RATE_EXACT);
    Serial.printf("Audio block samples: %d\n", AUDIO_BLOCK_SAMPLES);

    AudioMemory(32);
    startAudio();

    if (!client.begin()) {
        Serial.println("Failed to initialise client.");
        WAIT_INFINITE();
    }
}

void loop() {
    if (!client.isConnected()) {
        client.connect(2500);
    } else {
        client.loop();

        if (REPORT_USAGE && performanceReport > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            performanceReport = 0;
        }
    }
}


void startAudio() {
    audioShield.enable();
    // "...0.8 corresponds to the maximum undistorted output for a full scale
    // signal. Usually 0.5 is a comfortable listening level."
    // https://www.pjrc.com/teensy/gui/?info=AudioControlSGTL5000
    audioShield.volume(.8);

    audioShield.audioProcessorDisable();
    audioShield.autoVolumeDisable();
    audioShield.dacVolumeRampDisable();
}
