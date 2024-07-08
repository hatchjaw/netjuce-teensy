# netjuce-teensy

Client implementation, for the 
[Teensy 4.1](https://www.pjrc.com/store/teensy41.html) microcontroller 
development board, of a networked audio system designed, primarily, to support 
distributed spatial audio applications. 
Desktop-based server counterpart can be found at 
<https://github.com/hatchjaw/netjuce>.

## Requirements

Build and configuration are facilitated by [PlatformIO](https://platformio.org/)
(tested with v6.1.15).

To use `scripts/upload.sh`, install `tycmd`, which is part of the 
[TyTools](https://koromix.dev/tytools) suite by 
[Niels Martignène](https://github.com/Koromix).

### Hardware

- Teensy 4.1
  - Teensy 4.x [Audio shield](https://www.pjrc.com/store/teensy3_audio.html)
  - Teensy 4.1 [Ethernet kit](https://www.pjrc.com/store/ethernet_kit.html)
    - Also tested with the SparkFun
      [RJ45 MagJack Breakout](https://www.sparkfun.com/products/13021)
      (Assembly info [here](https://forum.pjrc.com/index.php?threads/teensy-4-1-ethernet-interface-and-smb-server.61309/))
- An ethernet switch
- CAT5E or CAT6 ethernet cables
- (Up to) sixteen loudspeakers
  - This limitation is imposed by the server and control software, which, at
    the time of writing, expects a maximum of eight connected clients.
- Adaptors to provide speaker input from the audio shield's 3.5 mm stereo jack
  output.
- Batteries, or a USB hub and micro-USB cables.

## Build

To build an environment (see [Available Environments](#available-environments))
and upload it to a Teensy:

```shell
pio run -t upload -e [environment]
```

Alternatively, with `tytools` installed, you can build an environment and 
upload it to any/all connected Teensies:

```shell
./scripts/upload.sh [environment]
```

If not provided, `environment` defaults to `basic`.

### Usage

See server
[Network setup](https://github.com/hatchjaw/netjuce?tab=readme-ov-file#network-setup)
for information on how to configure your network.

Once uploaded Teensy will reboot and, if `WAIT_FOR_SERIAL` is not defined (see 
[the relevant build flag](#wait_for_serial)), it will attempt to connect to a 
UDP multicast group and read UDP packets originating from the server.

To reboot all connected Teensies:

```shell
./scripts/reboot.sh
```

## Available Environments

The following environments are defined in [platformio.ini](platformio.ini):

### `basic`

A basic networked audio client implementation. Receives audio channels reaching 
a UDP multicast group, and delivers the first two channels to Teensy's analog 
audio outputs.

### `synctest`

An environment to facilitate the taking of one-way and round-trip transmission
time measurements. The client expects to receive a 16-bit, 
unit-amplitude-increment, unipolar sawtooth wave from the server. It returns 
two channels to the multicast group:
1. The unaltered incoming sawtooth wave, to be subtracted from itself upon 
   reaching the server again, thus giving and approximate round-trip time;
2. The difference (in samples) between the incoming sawtooth wave, and an 
   equivalent waveform generated on the client, providing an approximate 
   clock-drift measurement.

### `wfs`

A distributed Wave Field Synthesis algorithm. See the readme for the 
[WFS plugin](https://github.com/hatchjaw/netjuce/tree/main/src/wfs-plugin)
as described in the server repository for further usage information.

## Build-flag API

Various client settings can be altered via build flags. To apply such a 
change, find (or create) the appropriate environment in 
[platformio.ini](platformio.ini), navigate to the `build_flags` section and 
use the flags described below to specify CMake cache variables, e.g.

```ini
...
build_flags:
    -DAUDIO_BLOCK_SAMPLES=32
...
```

### `WAIT_FOR_SERIAL`

If defined (no value is required), the client will wait at the beginning of 
Teensy's `setup()` function until a serial connection is established.

### `AUDIO_BLOCK_SAMPLES`

Specify Teensy's audio block size. If not provided, defaults to 128.

NB. strictly it would be more accurate to refer to _frames_ rather than samples,
but this is the naming convention used by the Teensy Audio Library.

### `DEFAULT_LOCAL_PORT`

UDP port on which to receive packets. Default: 6664.

### `DEFAULT_REMOTE_PORT`

UDP port to which to send packets. Default: 14841.

### `NUM_SOURCES`

The number of monophonic audio sources/channels to send, and to expect to 
receive. Default: 2.

### `REPORT_USAGE`

If defined, Teensy will write an audio memory and processor usage report to
serial at five second intervals.

### `RESAMPLING_MODE`

Resampling options for the client's circular buffer. One of:

#### `NO_RESAMPLE`

Simply playback incoming samples at the rate at which they are received, with
no compensation for jitter or clock drift. In this naïve mode of operation it
is likely that buffer read and write pointers will overlap and audible 
distortion will result.

#### `TRUNCATE`

Use a fractional read-position increment, adjusting the increment relative to
the delta between the read position and write pointer. The client takes a 
_delay-locked loop_[^1] approach with the aim of keeping the delta within a 
sensible interval by reducing the read-position increment if the delta becomes 
too small, and increasing the increment if the delta becomes too large. 

[^1]: See Adriaensen, Fons. ‘Using a DLL to Filter Time’. Linux Audio Conference. Karlsruhe, Germany, 2005.

An output sample is found by removing the fractional part of the read-position
and reading from the buffer at the corresponding integer index. 

#### `INTERPOLATE` (default)

As above, but use cubic Lagrange interpolation to yield an output sample.

### `DO_CLOCK_ADJUST`

If defined, perform periodic adjustments to the speed of Teensy's
Synchronous Audio Interface (SAI1) clock. These adjustments are made by 
comparing the number of buffer writes to buffer reads in an arbitrary period of
time (not configurable at present; currently 2 seconds). In principle, if more 
samples (or buffers worth of samples to be precise) are read (used for audio 
output) than written (received from the server), then the client is running 
faster than the server and its clock speed should be reduced; if more writes 
than reads occur, the client clock speed should be increased. In practice, 
the temporal resolution afforded by using the network as a source of time in 
this way is rather coarse, even with a small `AUDIO_BLOCK_SAMPLES` of 32 (or 
even 16). If jitter is momentarily very significant, the clock adjustment
strategy may enter into oscillatory behaviour, failing to converge on a 
sensible average rate.

### `BUFFER_DEBUG_MODE`

One of `NO_BUFFER_DEBUG` (default), `RW_DELTA_METER`, which prints a static
read-write delta visualisation to serial, and `RW_DELTA_HISTORY`, which prints
each visualiser entry on a new line.

### `TRANSMISSION_DEBUG_MODE`

One of `NO_TRANSMISSION_DEBUG`, `HEXDUMP_RECEIVE`, which prints a hexdump of two
consecutive incoming packets every 10,000 packets, `HEXDUMP_SEND`, which does
similar for outgoing packets, and `HEXDUMP_AUDIO_OUT`, which prints the
data to be used for Teensy's two output audio channels.

## C++ API

Most of the above settings are specifiable to the constructor of the 
`NetJUCEClient` class, which accepts a single parameter, an instance of the 
`ClientSettings` struct.

```c++
NetJUCEClient(const ClientSettings &settings);
```

For instance, in `main.cpp`, one can create an instance of `ClientSettings`, and
pass that to an instance of `NetJUCEClient`:

```c++
ClientSettings settings{
        // Network adapter IP address.
        {192, 168, 10, 10},
        // Multicast group IP address.
        {224, 4, 224, 4}
};

NetJUCEClient client{settings};
```

Alternatively, one could achieve the above in `setup()` with:

```c++
ClientSettings s;
s.adapterIP = {192,168,10,10};
s.multicastIP = {244,4,244,4};
s.localPort = 6664;
s.remotePort = 14841;
s.resamplingMode = INTERPOLATE;
s.bufferDebugMode = RW_DELTA_HISTORY;
s.transmissionDebugMode = HEXDUMP_RECEIVE;
s.doClockAdjust = true;
c = new NetJUCEClient(s);
```
