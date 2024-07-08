//
// Created by tar on 3/5/23.
//

#include "CircularBuffer.h"

template<typename T>
CircularBuffer<T>::CircularBuffer(uint8_t numChannels,
                                  uint16_t length,
                                  const ClientSettings &settings) :
        kNumChannels{numChannels},
        kLength{length},
        kFloatLength{static_cast<float>(length)},
        kRwDeltaWindow(1.f/3.f),
        kRwDeltaThresh(
                kFloatLength * (.5f - (kRwDeltaWindow / 2.f)),
                kFloatLength * (.5f + (kRwDeltaWindow / 2.f))
        ),
        buffer{new T *[numChannels]},
        debugMode{settings.bufferDebugMode},
        resamplingMode{settings.resamplingMode} {

    for (int ch = 0; ch < kNumChannels; ++ch) {
        buffer[ch] = new T[kLength];
    }

    clear();

    if (debugMode > NO_BUFFER_DEBUG) {
        // Set up the rw-delta visualiser.
        memset(visualiser, '-', VISUALISER_LENGTH);
        visualiser[0] = '|';
        visualiser[VISUALISER_LENGTH - 1] = '|';
        if (resamplingMode > NO_RESAMPLE) {
            auto normLoThresh = static_cast<int>(roundf(100.f * (1.f - (kRwDeltaThresh.first / kFloatLength))));
            auto normHiThresh = static_cast<int>(roundf(100.f * (1.f - (kRwDeltaThresh.second / kFloatLength))));
            visualiser[normLoThresh] = '<';
            visualiser[normHiThresh] = '>';
        }
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
//    memset(*buffer, 0, kNumChannels * kLength * sizeof(T));

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
    auto d1{0}, d2{0};

    if (resamplingMode >= TRUNCATE) {
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
                bufferToFill[ch][n] = resamplingMode == INTERPOLATE ?
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
            if (debugMode > NO_BUFFER_DEBUG && debugTimer > 5000) {
                if (n == 0) {
                    d1 = static_cast<int>(roundf(100.f * (1.f - (rwDelta / kFloatLength))));
                } else if (n == len - 1) {
                    d2 = static_cast<int>(roundf(100.f * (1.f - (rwDelta / kFloatLength))));

                    auto temp1{visualiser[d1]}, temp2{visualiser[d2]};
                    visualiser[d1] = '#';
                    visualiser[d2] = '#';
                    Serial.printf("%s%d %s %3f (+%.8f)%s",
                                  debugMode == RW_DELTA_METER ? "\r" : "",
                                  kLength, visualiser, rwDelta, increment,
                                  debugMode == RW_DELTA_HISTORY ? "\n" : "");
                    visualiser[d1] = temp1;
                    visualiser[d2] = temp2;
                }
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
            if (debugMode > NO_BUFFER_DEBUG && debugTimer > 5000) {
                if (n == 0) {
                    d1 = static_cast<int>(roundf(100.f * (1.f - (static_cast<float>(rwDelta) / kFloatLength))));
                } else if (n == len - 1) {
                    d2 = static_cast<int>(roundf(100.f * (1.f - (static_cast<float>(rwDelta) / kFloatLength))));

                    auto temp1{visualiser[d1]}, temp2{visualiser[d2]};
                    visualiser[d1] = '#';
                    visualiser[d2] = '#';
                    Serial.printf("%s%d %s %d (+1 NO_RESAMPLE)%s",
                                  debugMode == RW_DELTA_METER ? "\r" : "",
                                  kLength, visualiser, rwDelta,
                                  debugMode == RW_DELTA_HISTORY ? "\n" : "");
                    visualiser[d1] = temp1;
                    visualiser[d2] = temp2;
                }
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
//        debugTimer = 0;
        Serial.printf("%swrites %" PRIu64 ":%" PRIu64 " reads - drift ratio: %.7f\n",
                      debugMode == RW_DELTA_METER ? "\n" : "",
                      numBlockWrites, //blocksWrittenSinceLastUpdate,
                      numBlockReads, //blocksReadSinceLastUpdate,
                      driftRatio);
    }
    numBlockWrites = 0;
    numBlockReads = 0;
    return driftRatio;
}
