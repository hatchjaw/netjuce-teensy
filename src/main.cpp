#include <Audio.h>
#include <NetJUCEClient.h>
#include <AsyncUDP_Teensy41.h>

#define BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)
#define WAIT_INFINITE() BLOCK_WITH_FORCED_SEMICOLON(while (true) yield();)

#if !( defined(CORE_TEENSY) && defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41) )
//#error Only Teensy 4.1 supported
#endif

#define ASYNC_UDP_TEENSY41_DEBUG_PORT       Serial

// Debug Level from 0 to 4
//#define _ASYNC_UDP_TEENSY41_LOGLEVEL_       4

#define SHIELD_TYPE     "Teensy4.1 QNEthernet"

#if (_ASYNC_UDP_TEENSY41_LOGLEVEL_ > 3)
#warning Using QNEthernet lib for Teensy 4.1. Must also use Teensy Packages Patch or error
#endif

#define ASYNC_UDP_TEENSY41_VERSION_MIN_TARGET      "AsyncUDP_Teensy41 v1.2.1"
#define ASYNC_UDP_TEENSY41_VERSION_MIN             1002001

namespace qn = qindesign::network;

byte mac[6];
IPAddress multicastIP{226, 6, 38, 226},
        adapterIP{192, 168, 10, 10},
        gateway{192, 168, 10, 1},
        clientIP{gateway},
        netmask{255, 255, 255, 0};
uint16_t localPort{DEFAULT_LOCAL_PORT};
uint16_t remotePort{DEFAULT_REMOTE_PORT}; // Use same port for promiscuous mode; all clients intercommunicate.

AsyncUDP udp;

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

    qn::Ethernet.onLinkState([](bool state) {
        Serial.printf("[Ethernet] Link %s\n", state ? "ON" : "OFF");

        if (state) {
            Serial.print(F("Connected! IP address: "));
            Serial.println(qn::Ethernet.localIP());

            if (!udp.listenMulticast(multicastIP, localPort)) {
                Serial.println("Failed to join multicast group.");
                WAIT_INFINITE();
            } else {
                Serial.println("Joined multicast group.");
                udp.onPacket([](AsyncUDPPacket packet) {
                    auto ip{packet.remoteIP()};
                    client.handlePacket(ip, packet.remotePort(), packet.data(), packet.length());
                });
            }
        }
    });

    Serial.print("\nStart AsyncUDPMulticastServer on ");
    Serial.println(BOARD_NAME);
    Serial.println(ASYNC_UDP_TEENSY41_VERSION);

    qn::Ethernet.macAddress(mac);
    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    clientIP[3] = mac[5];
    Serial.println("Initialize Ethernet using static IP");
    qn::Ethernet.begin(clientIP, netmask, gateway);

    if (!client.begin()) {
        WAIT_INFINITE();
    }
}

void loop() {
    if (!client.isConnected()) {
        Serial.print("Restarting ethernet... ");
        if(!qn::Ethernet.begin(clientIP, netmask, gateway)){
            Serial.println("failed.");
            delay(2500);
        } else {
            Serial.println("ready.");
            if (!client.begin()) {
                WAIT_INFINITE();
            }
        }
    } else {
        if (usageReportTimer > USAGE_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            usageReportTimer = 0;
        }
    }
}