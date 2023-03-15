#include <Audio.h>
#include <NetJUCEClient.h>

#define BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)
#define WAIT_INFINITE() BLOCK_WITH_FORCED_SEMICOLON(while (true) yield();)

IPAddress multicastIP{226, 6, 38, 226};
IPAddress adapterIP{192, 168, 10, 10};
uint16_t localPort{DEFAULT_LOCAL_PORT};
uint16_t remotePort{DEFAULT_REMOTE_PORT};

AudioControlSGTL5000 audioShield;

AudioOutputI2S out;
NetJUCEClient client{adapterIP, multicastIP, remotePort, localPort,
                     DebugMode::HEXDUMP_SEND};

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
        if (usageReportTimer > USAGE_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            usageReportTimer = 0;
        }
    }
}