//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_CIRCULARBUFFER_H
#define NETJUCE_TEENSY_CIRCULARBUFFER_H

#define CIRCULAR_BUFFER_SIZE (AUDIO_BLOCK_SAMPLES * 4)

#include <Arduino.h>
#include "SmoothedValue.h"

template<typename T>
class CircularBuffer {
public:
    enum class DebugMode {
        NONE,
        RW_DELTA_VISUALISER,
    };

    CircularBuffer(uint8_t numChannels, uint16_t length, DebugMode debugMode = DebugMode::NONE);

    ~CircularBuffer();

    void write(const T **data, uint16_t len);

    void read(T **bufferToFill, uint16_t len);

//    int getWriteIndex();
//
//    float getReadPosition();

    void clear();

//    void printStats();

private:
    enum OperationType {
        UNKNOWN,
        READ,
        WRITE
    };
    static constexpr uint16_t BLOCKS_PER_READ_INCREMENT_UPDATE{1000};
    static constexpr uint8_t VISUALISER_LENGTH{100};
    const uint32_t kStatInterval{2500};
    const uint8_t kNumChannels;
    const uint16_t kLength;
    const float kFloatLength;
    const std::pair<float, float> kRwDeltaThresh;

//    float getReadWriteDelta();
//
//    void setReadPosIncrement();
//
//    T interpolateCubic(T *channelData, uint16_t readIdx, float alpha);
//
//    uint16_t wrapIndex(uint16_t index, uint16_t length);

    T **buffer;
    uint16_t writeIndex{0}, readIndex{0};
    float readPos{0.f};
    SmoothedValue<float> readPosIncrement{1.f};
    uint64_t numBlockReads{0}, numBlockWrites{0}, numSampleWrites{0}, numSampleReads{0};
    uint32_t blocksReadSinceLastUpdate{0}, blocksWrittenSinceLastUpdate{0};
    float readPosAllTime{0.f};
    OperationType lastOp{UNKNOWN};
    uint8_t consecutiveOpCount{1};
    elapsedMillis statTimer{0};
    elapsedMillis debugTimer{100};
    char visualiser[VISUALISER_LENGTH + 1]{};
    DebugMode debugMode{DebugMode::NONE};
};


template
class CircularBuffer<int16_t>;

#endif //NETJUCE_TEENSY_CIRCULARBUFFER_H
