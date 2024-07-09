//
// Created by tar on 3/5/23.
//

#ifndef NETJUCE_TEENSY_CIRCULARBUFFER_H
#define NETJUCE_TEENSY_CIRCULARBUFFER_H

#include <Arduino.h>
#include "ClientSettings.h"
#include "SmoothedValue.h"

template<typename T>
class CircularBuffer {
public:
    CircularBuffer(uint8_t numChannels,
                   uint16_t length,
                   const ClientSettings &settings);

    ~CircularBuffer();

    void write(const T **data, uint16_t len);

    void read(T **bufferToFill, uint16_t len);

//    int getWriteIndex();
//
//    float getReadPosition();

    void clear();

//    void printStats();

    float getDriftRatio(bool andPrint = false);

private:
    enum OperationType {
        UNKNOWN,
        READ,
        WRITE
    };
    static constexpr int BLOCKS_PER_READ_INCREMENT_UPDATE{10000};
    static constexpr uint8_t VISUALISER_LENGTH{61};
    const uint32_t kStatInterval{2500};
    const uint8_t kNumChannels;
    const uint16_t kLength;
    const float kFloatLength;
    const float kRwDeltaWindow;
    const std::pair<float, float> kRwDeltaThresh;

    float getReadWriteDelta();

    void setReadPosIncrement();

    T interpolateCubic(T *channelData, uint16_t readIdx, float alpha);

//    uint16_t wrapIndex(uint16_t index, uint16_t length);

    T **buffer;
    uint16_t writeIndex{0}, readIndex{0};
    float readPos{0.f};
    SmoothedValue_V2<double> readPosIncrement{1., READ_POS_SMOOTHNESS};
    uint64_t numBlockReads{0}, numBlockWrites{0}, numSampleWrites{0}, numSampleReads{0};
    uint32_t blocksReadSinceLastUpdate{0}, blocksWrittenSinceLastUpdate{0};
    float driftRatio{1.f};
    float readPosAllTime{0.f};
    OperationType lastOp{UNKNOWN};
    uint8_t consecutiveOpCount{1};
    elapsedMillis statTimer{0};
    elapsedMillis debugTimer{5000};
    char visualiser[VISUALISER_LENGTH + 1]{};
    BufferDebugMode debugMode;
    ResamplingMode resamplingMode;
};

template
class CircularBuffer<int16_t>;

#endif //NETJUCE_TEENSY_CIRCULARBUFFER_H
