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

#ifndef SYNTHMARK_LOGTOOL_H
#define SYNTHMARK_LOGTOOL_H

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include "ByteFIFO.h"

#define LOGTOOL_BUFFER_SIZE (1024) //max log line size
#define LOGTOOL_FIFO_SIZE (32 * 1024) // FIFO for storing messages until the foreground reads them

class LogTool
{
public:
    LogTool()
        : mFIFO(LOGTOOL_FIFO_SIZE)
        , mVar1(0)
    {
    }
    virtual ~LogTool() {}

    virtual int32_t log(const char* format, ...) {
        int numWritten = snprintf(mBuffer, LOGTOOL_BUFFER_SIZE, "%8d, ", mVar1);
        va_list args;
        va_start(args, format);
        numWritten += vsnprintf(&mBuffer[numWritten],
                                LOGTOOL_BUFFER_SIZE - numWritten,
                                format,
                                args);
        va_end(args);

        if (numWritten > 0) {
            // If the buffer is full then logs will be lost.
            mFIFO.write(mBuffer, numWritten); // returns immediately
        }
        return numWritten;
    }

    void setVar1(int value) {
        mVar1 = value;
    }

    int getVar1() {
        return mVar1;
    }

    bool hasLogs() {
        return mFIFO.getAvailableToRead() > 0;
    }

    std::string readLog() {
        return mFIFO.readLog();
    }

private:
    ByteFIFO      mFIFO;
    char          mBuffer[LOGTOOL_BUFFER_SIZE + 1]; // for writing
    int           mVar1;    //user assigned variable.
};

#endif //SYNTHMARK_LOGTOOL_H
