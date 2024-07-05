//
// Created by tar on 04/07/24.
//

#ifndef NETJUCE_TEENSY_CLIENT_SETTINGS_H
#define NETJUCE_TEENSY_CLIENT_SETTINGS_H

#ifndef RESAMPLING_MODE
#define RESAMPLING_MODE INTERPOLATE
#endif

#ifndef BUFFER_DEBUG_MODE
#define BUFFER_DEBUG_MODE NO_BUFFER_DEBUG
#endif

#ifndef CLIENT_DEBUG_MODE
#define CLIENT_DEBUG_MODE NO_CLIENT_DEBUG
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

enum BufferDebugMode {
    /**
     * No debug output.
     */
    NO_BUFFER_DEBUG,
    /**
     * Visualise the behaviour of the read position over serial.
     */
    RW_DELTA_VISUALISER,
};

enum ClientDebugMode
{
    /**
     * No debug output.
     */
    NO_CLIENT_DEBUG,
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

struct ClientSettings
{
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
    uint16_t localPort{0};
    /**
     * Port to which to send packets.
     */
    uint16_t remotePort{0};
    /**
     * Mode to use when resampling the circular buffer. Default: INTERPOLATE.
     * @see ResamplingMode
     */
    ResamplingMode resamplingMode{RESAMPLING_MODE};
    /**
     * Debug mode for the circular buffer. Default: NO_BUFFER_DEBUG.
     * @see BufferDebugMode
     */
    BufferDebugMode bufferDebugMode{BUFFER_DEBUG_MODE};
    /**
     * Debug mode for the audio client. Default: NO_CLIENT_DEBUG.
     * @see ClientDebugMode
     */
    ClientDebugMode clientDebugMode{CLIENT_DEBUG_MODE};
};

#endif //NETJUCE_TEENSY_CLIENT_SETTINGS_H
