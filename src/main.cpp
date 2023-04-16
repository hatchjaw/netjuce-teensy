#include <Audio.h>
#include <AudioStream.h>
#include <NetJUCEClient.h>
#include <Utils.h>

IPAddress multicastIP{226, 6, 38, 226};
IPAddress adapterIP{192, 168, 10, 10};
uint16_t localPort{DEFAULT_LOCAL_PORT};
uint16_t remotePort{DEFAULT_REMOTE_PORT}; // Use same port for promiscuous mode; all clients intercommunicate.

//AudioOutputUSB usb;
AudioControlSGTL5000 audioShield;

AudioOutputI2S out;
NetJUCEClient client{adapterIP, multicastIP, remotePort, localPort,
                     DebugMode::HEXDUMP_RECEIVE};

AudioConnection patchCord1(client, 0, out, 0);
AudioConnection patchCord2(client, 1, out, 1);
AudioConnection patchCord3(client, 0, client, 0);
AudioConnection patchCord4(client, 1, client, 1);

elapsedMillis usageReportTimer;
constexpr uint32_t USAGE_REPORT_INTERVAL{5000};

void setup() {
    while (!Serial);

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
    if (!client.isConnected()) {
        client.connect(2500);
    } else {
        client.loop();

        if (usageReportTimer > USAGE_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            usageReportTimer = 0;
        }
    }
}