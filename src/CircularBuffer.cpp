//
// Created by tar on 3/5/23.
//

#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(uint8_t numChannels, uint16_t length, ReadMode readModeToUse,
                                  DebugMode debugModeToUse) :
        kNumChannels{numChannels},
        kLength{length},
        kFloatLength{static_cast<float>(length)},
        // TODO: make adjustable; check that first is less than second
//        kRwDeltaThresh(kFloatLength * .225f, kFloatLength * .475f),
        kRwDeltaThresh(kFloatLength * .2f, kFloatLength * .5f),
        buffer{new T *[numChannels]},
        debugMode{debugModeToUse},
        readMode{readModeToUse} {

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
        memset(buffer[ch], 0, kLength * sizeof(T));
    }

    readPos = kFloatLength * .25f;
    writeIndex = 0;
    readIndex = kLength * .5f;
    numBlockReads = 0;
    numBlockWrites = 0;
    numSampleWrites = 0;
    numSampleReads = 0;
    readPosAllTime = 0.f;
    readPosIncrement.set(1.);//, true);
    blocksReadSinceLastUpdate = 0;
    blocksWrittenSinceLastUpdate = 0;
    driftRatio = 1.f;
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

//    if (readMode == ReadMode::RESAMPLE) {
    ++numBlockWrites;
    ++blocksWrittenSinceLastUpdate;
//    }
//    Serial.printf("write end -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
}

template<typename T>
void CircularBuffer<T>::read(T **bufferToFill, uint16_t len) {
    if (readMode >= RESAMPLE_TRUNCATE) {
//        setReadPosIncrement();
        auto initialReadPos{readPos};

        for (int n{0}; n < len; ++n) {
            if (readPos >= kFloatLength) {
                readPos -= kFloatLength;
            }

            auto readPosIntPart{0.f}, alpha{modff(readPos, &readPosIntPart)};
            auto readPosInt{static_cast<int>(readPosIntPart)};
            // For each channel, get the next sample, interpolated (or
            // truncated) around readPos.
            for (int ch{0}; ch < kNumChannels; ++ch) {
                bufferToFill[ch][n] = readMode == RESAMPLE_INTERPOLATE ?
                                      interpolateCubic(buffer[ch], readPosInt, alpha) :
                                      buffer[ch][readPosInt];
            }

            // Try to keep read position a consistent, safe distance behind
            // write index.
            auto rwDelta{getReadWriteDelta()};

            if (rwDelta < kRwDeltaThresh.first) {
                readPosIncrement.set(rwDelta / kRwDeltaThresh.first);
            } else if (rwDelta > kRwDeltaThresh.second) {
                readPosIncrement.set(rwDelta / kRwDeltaThresh.second);
            } else {
                readPosIncrement.set(1.f);
//                readPosIncrement.set(driftRatio);
            }

            auto increment{readPosIncrement.getNext()};
            readPos += increment;

            // Visualise the state of the read-write delta.
            if (debugMode == DebugMode::RW_DELTA_VISUALISER && (n == 0 || n == len - 1) && debugTimer > 5000) {
                auto r{static_cast<int>(roundf(100.f * (1.f - (rwDelta / kFloatLength))))};
                auto temp{visualiser[r]};
                visualiser[r] = '#';
                Serial.printf("%s %3f (+%.8f)\n", visualiser, rwDelta, increment);
                visualiser[r] = temp;
            }
        }

        if (0 == numBlockWrites) {
            readPos = initialReadPos;
            readPosAllTime = 0.;
        } else {
            ++numBlockReads;
            ++blocksReadSinceLastUpdate;
        }
    } else {
        //    Serial.printf("read start -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
        auto initialReadPos{readIndex};

        for (uint16_t n = 0; n < len; ++n, ++readIndex) {
            if (readIndex == kLength) {
                readIndex = 0;
            }

            for (int ch = 0; ch < kNumChannels; ++ch) {
                bufferToFill[ch][n] = buffer[ch][readIndex];
            }

            int rwDelta;
            if (readIndex > writeIndex) {
                rwDelta = writeIndex + kLength - readIndex;
            } else {
                rwDelta = writeIndex - readPos;
            }

            // Visualise the state of the read-write delta.
            if (debugMode == DebugMode::RW_DELTA_VISUALISER && n % 8 == 0 && debugTimer > 5000) {
                auto r{static_cast<int>(roundf(100.f * (1.f - (static_cast<float>(rwDelta) / kFloatLength))))};
                auto temp{visualiser[r]};
                visualiser[r] = '#';
                Serial.printf("%s %d (+%.8f)\n", visualiser, rwDelta, 1.f);
                visualiser[r] = temp;
            }
        }

        if (0 == numBlockWrites) {
            readPos = initialReadPos;
            readPosAllTime = 0.;
        } else {
            ++numBlockReads;
            ++blocksReadSinceLastUpdate;
        }
        //    Serial.printf("read end -- writeIndex: %d; readIndex: %d\n", writeIndex, readIndex);
    }
}

template<typename T>
float CircularBuffer<T>::getReadWriteDelta() {
    auto fWrite{static_cast<float>(writeIndex)};
    if (readPos > fWrite) {
        return fWrite + kFloatLength - readPos;
    } else {
        return fWrite - readPos;
    }
}

template<typename T>
void CircularBuffer<T>::setReadPosIncrement() {
    // Update the read increment periodically, based on the ratio of writes
    // to reads during that period.
    // If writing more blocks than reading, speed up; if reading more blocks
    // than writing, slow down.
    if (blocksReadSinceLastUpdate > 0 && blocksWrittenSinceLastUpdate >= BLOCKS_PER_READ_INCREMENT_UPDATE) {
//        driftRatio = static_cast<float>(blocksWrittenSinceLastUpdate) / static_cast<float>(blocksReadSinceLastUpdate);
        // Use all-time writes:reads instead...
        getDriftRatio(false);

        readPosIncrement.set(driftRatio);

        blocksReadSinceLastUpdate = 0;
        blocksWrittenSinceLastUpdate = 0;
    }
}

template<typename T>
T CircularBuffer<T>::interpolateCubic(T *channelData, uint16_t readIdx, float alpha) {
    int r{readIdx};
    auto rm{r - 1}, rp{r + 1}, rpp{r + 2};
    if (r == 0) {
        rm = kLength - 1;
    } else if (r == kLength - 2) {
        rpp = 0;
    } else if (r == kLength - 1) {
        rp = 0;
        rpp = 1;
    }
    auto val = static_cast<float>(channelData[rm]) * (-alpha * (alpha - 1.f) * (alpha - 2.f) / 6.f)
               + static_cast<float>(channelData[r]) * ((alpha - 1.f) * (alpha + 1.f) * (alpha - 2.f) / 2.f)
               + static_cast<float>(channelData[rp]) * (-alpha * (alpha + 1.f) * (alpha - 2.f) / 2.f)
               + static_cast<float>(channelData[rpp]) * (alpha * (alpha + 1.f) * (alpha - 1.f) / 6.f);

    return static_cast<T>(round(val));
}

template<typename T>
float CircularBuffer<T>::getDriftRatio(bool andPrint) {
    // writes > reads, the server is running fast; ratio > 1; client should speed up.
    // writes < reads, the client is running fast; ratio < 1; client should slow down.
    driftRatio = static_cast<float>(numBlockWrites) / static_cast<float>(numBlockReads);
    // ...But this assumes that buffer size is the same for server and clients,
    // which it needn't necessarily be.
    if (andPrint) {
        debugTimer = 0;
        Serial.printf("writes %" PRIu64 ":%" PRIu64 " reads - drift ratio: %.7f\n",
                      numBlockWrites, //blocksWrittenSinceLastUpdate,
                      numBlockReads, //blocksReadSinceLastUpdate,
                      driftRatio);
    }
//    numBlockWrites = 0;
//    numBlockReads = 0;
    return driftRatio;
}
