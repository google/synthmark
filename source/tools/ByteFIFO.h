/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_CHARFIFO_H
#define ANDROID_CHARFIFO_H

#include <algorithm>
#include <atomic>
#include <cstdint>

class ByteFIFO {
public:
    ByteFIFO(int numBytes)
        : mBuffer(new uint8_t[numBytes])
        , mSize(numBytes)
    {
        // mBuffer = std::make_unique<uint8_t[]>(numBytes); // requires C++14
    }

    int write(const char *src, int32_t len) {
        int32_t numLeft = len;
        int32_t avail = availableToWriteContiguous();
        while (numLeft > 0 && avail > 0) {
            int32_t toWrite = std::min(avail, numLeft);
            memcpy(getWriteAddress(), src, toWrite);
            advanceWriteCounter(toWrite);
            src += toWrite;
            numLeft -= toWrite;
            avail = availableToWriteContiguous();
        }
        return len - numLeft;
    }

    int32_t getAvailableToRead() {
        return std::min(mSize, (int32_t) (mWriteCounter - mReadCounter));
    }

    int32_t getAvailableToWrite() {
        return mSize - getAvailableToRead();
    }

    std::string readLog() {
        int avail = availableToReadContiguous();
        std::string s = std::string((const char *)getReadAddress(), avail);
        advanceReadCounter(avail);
        return s;
    }

private:
    int32_t availableToWriteContiguous() {
        int32_t avail = getAvailableToWrite();
        int32_t sizeUntilEnd = mSize - getWritePosition();
        return std::min(avail, sizeUntilEnd);
    }

    int32_t availableToReadContiguous() {
        int32_t avail = getAvailableToRead();
        int32_t sizeUntilEnd = mSize - getReadPosition();
        return std::min(avail, sizeUntilEnd);
    }

    void advanceWriteCounter(int32_t count) {
        if (count > 0) mWriteCounter += count;
    }

    uint32_t getWritePosition() {
        return (uint32_t) ((uint64_t) mWriteCounter) % mSize;
    }

    uint8_t * getWriteAddress() {
        return &(mBuffer.get()[getWritePosition()]);
    }

    void advanceReadCounter(int32_t count) {
        if (count > 0) mReadCounter += count;
    }

    uint32_t getReadPosition() {
        return (uint32_t) ((uint64_t) mReadCounter) % mSize;
    }

    uint8_t * getReadAddress() {
        return &(mBuffer.get()[getReadPosition()]);
    }

    std::atomic<int64_t>       mWriteCounter{};
    std::atomic<int64_t>       mReadCounter{};
    std::unique_ptr<uint8_t[]> mBuffer;
    int32_t                    mSize;
};
#endif //ANDROID_CHARFIFO_H
