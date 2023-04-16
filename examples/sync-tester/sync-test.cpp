#include <Audio.h>
#include <NetJUCEClient.h>
#include <Utils.h>
#include "SyncTester/SyncTester.h"

// Wait for a serial connection before proceeding with execution
#define WAIT_FOR_SERIAL
//#undef WAIT_FOR_SERIAL

// Local udp port on which to receive packets.
const uint16_t kLocalUdpPort = 8888;
// Remote server IP address -- should match address in IPv4 settings.
IPAddress multicastIP{226, 6, 38, 226};
IPAddress adapterIP{192, 168, 10, 10};

// Audio shield driver
AudioControlSGTL5000 audioShield;
AudioOutputI2S out;

NetJUCEClient client{adapterIP, multicastIP};
SyncTester st;

// Send input from server back to server.
AudioConnection patchCord10(client, 0, client, 0);
// Send input from server to audio output.
AudioConnection patchCord20(client, 0, out, 0);
// Send input to synchronicity tester.
AudioConnection patchCord30(client, 0, st, 0);
// Combine with generated sawtooth.
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

    client.setDebugMode(DebugMode::HEXDUMP_SEND);
}

void loop() {
    if (!client.isConnected()) {
        client.connect(2500);
    } else {
        client.loop();

        if (performanceReport > PERF_REPORT_INTERVAL) {
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
