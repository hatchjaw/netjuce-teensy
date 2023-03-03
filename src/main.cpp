#include <Audio.h>
#include <NetJUCEClient.h>
//#include <NativeEthernet.h>
//#include <QNEthernet.h>

#define BLOCK_WITH_FORCED_SEMICOLON(x) do { x } while (false)
#define WAIT_INFINITE() BLOCK_WITH_FORCED_SEMICOLON(while (true) yield();)

//using namespace qindesign::network;

IPAddress multicastIP{226, 6, 38, 226};
IPAddress clientIP{192, 168, 10, 11};
IPAddress gateway{192, 168, 10, 1};
IPAddress subnetMask{255, 255, 255, 0};
uint8_t mac[]{0x04, 0xe9, 0xe5, 0x12, 0xe1, 0x92};
//EthernetUDP udp;
uint16_t localPort{18999};
uint16_t remotePort{18999};
char packetBuffer[1 << 8];
elapsedMillis elapsed;
uint timeout{5000};
bool connected{false};

AudioControlSGTL5000 audioShield;

AudioOutputI2S out;
NetJUCEClient client{multicastIP, remotePort, localPort};

AudioConnection patchCord1(client, 0, out, 0);
AudioConnection patchCord2(client, 1, out, 1);

elapsedMillis performanceReport;
constexpr uint32_t PERF_REPORT_INTERVAL{5000};

void setup() {
    while (!Serial);

    if (CrashReport) {  // Print any crash report
        Serial.println(CrashReport);
        CrashReport.clear();
    }

//    // Print the MAC address
//    uint8_t mac[6];
//    Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
//    Serial.printf(F("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
//                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

//    if (!Ethernet.begin(clientIP, subnetMask, gateway)) {
//        Serial.println(F("Failed to start Ethernet"));
//        WAIT_INFINITE();
//    } else {
//        Serial.println(F("Ethernet started."));
//    }

//    EthernetClass::begin(mac, clientIP, subnetMask, gateway);

//    Serial.print(F("IP: "));
//    Serial.println(EthernetClass::localIP());

    AudioMemory(32);
    audioShield.enable();
    audioShield.volume(.5);

    elapsed = 0;

    if (!client.begin()) {
        WAIT_INFINITE();
    }
}

void loop() {
//    Serial.printf(F("Status: %s, elapsed = "), connected ? F("connected") : F("polling"));
//    Serial.println(elapsed);

//    if (!connected) {
//        Serial.print(F("Joining multicast group at "));
//        Serial.print(multicastIP);
//        Serial.printf(F(":%d... "), localPort);
//
//        auto joined = udp.beginMulticast(multicastIP, localPort);
//
//        if (joined) {
//            Serial.println(F("Success!"));
//            connected = true;
//        } else {
//            Serial.println(F("Failed."));
//            delay(timeout);
//            return;
//        }
//    }

//    auto packetSize{0};
//    while ((packetSize = udp.parsePacket()) > 0) {
//        auto isSmall{packetSize < 50};
//
//        Serial.printf(F("Received packet from "));
//        IPAddress remote = udp.remoteIP();
//        for (int i = 0; i < 4; i++) {
//            Serial.print(remote[i], DEC);
//            if (i < 3) {
//                Serial.print(F("."));
//            }
//        }
//        Serial.printf(F(":%d, size %d bytes\n"), udp.remotePort(), packetSize);
//
//        // read the packet into packetBuffer
//        isSmall && Serial.print(F("Contents: "));
//        while (0 < udp.read(packetBuffer, packetSize)) {
//            isSmall && Serial.print(packetBuffer);
//            memset(packetBuffer, 0, sizeof packetBuffer);
//        }
//        isSmall && Serial.println();
//
//        elapsed = 0;
//    }

//    if (elapsed > timeout) {
//        Serial.printf(F("Nothing received for %d ms. Stopping.\n"), timeout);
//        udp.stop();
//        connected = false;
//        elapsed = 0;
//    }

    if (!client.isConnected()) {
        client.connect(2500);
    } else {
        if (performanceReport > PERF_REPORT_INTERVAL) {
            Serial.printf("Audio memory in use: %d blocks; processor %f %%\n",
                          AudioMemoryUsage(),
                          AudioProcessorUsage());
            performanceReport = 0;
        }
    }
}