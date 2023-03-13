//
// Created by tar on 3/5/23.
//

#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(uint8_t numChannels, uint16_t length, DebugMode debugMode) :
        kNumChannels{numChannels},
        kLength{length},
        kFloatLength{static_cast<float>(length)},
        kRwDeltaThresh(kFloatLength * .15f, kFloatLength * .45f),
        buffer{new T *[numChannels]},
        debugMode{debugMode} {

    for (int ch = 0; ch < kNumChannels; ++ch) {
        buffer[ch] = new T[kLength];
    }

    clear();

    if (debugMode == DebugMode::RW_DELTA_VISUALISER) {
        // Set up the rw-delta visualiser.
        memset(visualiser, '-', VISUALISER_LENGTH);
        auto normLoThresh = static_cast<int>(roundf(100.f * (1.f - (kRwDeltaThresh.first / kFloatLength))));
        auto normHiThresh = static_cast<int>(roundf(100.f * (1.f - (kRwDeltaThresh.second / kFloatLength))));
        visualiser[normLoThresh] = '<';
        visualiser[normHiThresh] = '>';
    }
}


template<typename T>
CircularBuffer<T>::~CircularBuffer() {
    for (int i = 0; i < kNumChannels; ++i) {
        delete buffer[i];
    }
    delete[] buffer;
}

template<typename T>
void CircularBuffer<T>::clear() {
    // Why doesn't this work?
//    memset(buffer[0], 0, kNumChannels * kLength * sizeof(T));

    for (int ch = 0; ch < kNumChannels; ++ch) {
        for (int s = 0; s < kLength; ++s) {
            buffer[ch][s] = 0;
        }
    }

    readPos = kFloatLength * .25f;
    writeIndex = 0;
    readIndex = kLength * .5f;
    numBlockReads = 0;
    numBlockWrites = 0;
    numSampleWrites = 0;
    numSampleReads = 0;
    readPosAllTime = 0.f;
    readPosIncrement.set(1., true);
}

template<typename T>
void CircularBuffer<T>::write(const T **data, uint16_t len) {
//    Serial.printf("write start -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
    for (int n = 0; n < len; ++n, ++writeIndex, ++numSampleWrites) {
        if (writeIndex == kLength) {
            writeIndex = 0;
        }

        for (int ch = 0; ch < kNumChannels; ++ch) {
            buffer[ch][writeIndex] = data[ch][n];
        }
    }
//    Serial.printf("write end -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
}

template<typename T>
void CircularBuffer<T>::read(T **bufferToFill, uint16_t len) {
//    Serial.printf("read start -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
    for (uint16_t n = 0; n < len; ++n, ++readIndex) {
        if (readIndex == kLength) {
            readIndex = 0;
        }

        for (int ch = 0; ch < kNumChannels; ++ch) {
            bufferToFill[ch][n] = buffer[ch][readIndex];
        }
    }
//    Serial.printf("read end -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
}