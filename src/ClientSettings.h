//
// Created by tar on 04/07/24.
//

#ifndef NETJUCE_TEENSY_CLIENT_SETTINGS_H
#define NETJUCE_TEENSY_CLIENT_SETTINGS_H

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES 128
#endif

#ifndef NUM_SOURCES
#define NUM_SOURCES 2
#endif

#ifndef MULTICAST_IP
#define MULTICAST_IP "224.4.224.4"
#endif

#ifndef DEFAULT_REMOTE_PORT
#define DEFAULT_REMOTE_PORT 14841
#endif

#ifndef DEFAULT_LOCAL_PORT
#define DEFAULT_LOCAL_PORT 6664
#endif

#ifndef REPORT_USAGE
#define REPORT_USAGE false
#else
#define REPORT_USAGE true
#endif

#ifndef DO_CLOCK_ADJUST
#define DO_CLOCK_ADJUST false
#else
#define DO_CLOCK_ADJUST true
#endif

#ifndef RESAMPLING_MODE
#define RESAMPLING_MODE INTERPOLATE
#endif

#ifndef BUFFER_DEBUG_MODE
#define BUFFER_DEBUG_MODE NO_BUFFER_DEBUG
#endif

#ifndef TRANSMISSION_DEBUG_MODE
#define TRANSMISSION_DEBUG_MODE NO_TRANSMISSION_DEBUG
#endif

#include <IPAddress.h>

enum ResamplingMode
{
    /**
     * Just read from the buffer sample by sample.
     */
    NO_RESAMPLE,
    /**
     * Use a fractional read-position increment but discard the fractional part
     * of the read pointer.
     */
    TRUNCATE,
    /**
     * Use a fractional read-position increment and interpolate around the read
     * pointer.
     */
    INTERPOLATE
};

enum BufferDebugMode
{
    /**
     * No debug output.
     */
    NO_BUFFER_DEBUG,
    /**
     * Visualise the behaviour of the read position over serial;
     * overwrite previous lines.
     */
    RW_DELTA_METER,
    /**
     * Visualise the behaviour of the read position over serial;
     * write lines sequentially.
     */
    RW_DELTA_HISTORY
};

enum TransmissionDebugMode
{
    /**
     * No debug output.
     */
    NO_TRANSMISSION_DEBUG,
    /**
     * Periodically hexdump received packets.
     */
    HEXDUMP_RECEIVE,
    /**
     * Periodically hexdump outgoing packets.
     */
    HEXDUMP_SEND,
    /**
     * Periodically hexdump audio output.
     */
    HEXDUMP_AUDIO_OUT
};

struct ClientSettings : public Printable
{
    ClientSettings() = default;

    ClientSettings(const IPAddress &adapterIp,
                   const IPAddress &multicastIp,
                   uint16_t localPort = DEFAULT_LOCAL_PORT,
                   uint16_t remotePort = DEFAULT_REMOTE_PORT,
                   bool doClockAdjust = DO_CLOCK_ADJUST,
                   ResamplingMode resamplingMode = RESAMPLING_MODE,
                   BufferDebugMode bufferDebugMode = BUFFER_DEBUG_MODE,
                   TransmissionDebugMode transmissionDebugMode = TRANSMISSION_DEBUG_MODE) :
            adapterIP(adapterIp),
            multicastIP(multicastIp),
            localPort(localPort),
            remotePort(remotePort),
            doClockAdjust(doClockAdjust),
            resamplingMode(resamplingMode),
            bufferDebugMode(bufferDebugMode),
            transmissionDebugMode(transmissionDebugMode) {}

    size_t printTo(Print &p) const override
    {
        size_t size{0};
        size += p.print("Server IP: ");
        size += p.print(adapterIP);
        size += p.println();
        size += p.print("Multicast IP: ");
        size += p.print(multicastIP);
        size += p.println();
        return size + p.printf(
                "Local port: %" PRIu16 "\n"
                "Remote port: %" PRIu16 "\n"
                "Clock adjustments: %s\n"
                "Resampling mode: %s\n"
                "Buffer debug mode: %s\n"
                "Transmission debug mode: %s\n",
                localPort,
                remotePort,
                doClockAdjust ? "On" : "Off",
                resamplingMode == INTERPOLATE ? "Interpolate" : (resamplingMode == TRUNCATE ? "Truncate" : "None"),
                bufferDebugMode == RW_DELTA_HISTORY ? "Read-write delta history" : (
                        bufferDebugMode == RW_DELTA_METER ? "Read-write delta meter" : "None"),
                transmissionDebugMode == HEXDUMP_AUDIO_OUT ? "Hexdump audio output" : (
                        transmissionDebugMode == HEXDUMP_SEND ? "Hexdump outgoing packets" : (
                                transmissionDebugMode == HEXDUMP_RECEIVE ? "Hexdump incoming packets" : "None"))
        );
    }

    /**
     * IP address of the network adapter, i.e. the server.
     */
    IPAddress adapterIP;
    /**
     * IP address of the multicast group.
     */
    IPAddress multicastIP;
    /**
     * Port on which to receive packets.
     */
    uint16_t localPort;
    /**
     * Port to which to send packets.
     */
    uint16_t remotePort;
    /**
     * Mode to use when resampling the circular buffer. Default: INTERPOLATE.
     * @see ResamplingMode
     */
    ResamplingMode resamplingMode;
    /**
     * Debug mode for the circular buffer. Default: NO_BUFFER_DEBUG.
     * @see BufferDebugMode
     */
    BufferDebugMode bufferDebugMode;
    /**
     * Debug mode for the audio client. Default: NO_TRANSMISSION_DEBUG.
     * @see TransmissionDebugMode
     */
    TransmissionDebugMode transmissionDebugMode;
    /**
     * Enable/disable clock adjustments.
     */
    bool doClockAdjust;
};

#endif //NETJUCE_TEENSY_CLIENT_SETTINGS_H
