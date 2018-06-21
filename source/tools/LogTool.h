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


#define LOGTOOL_BUFFER_SIZE (10 * 1024) //max single log that can be added at once
#define LOGTOOL_PREFIX_MAX 10
class LogTool
{
public:
    LogTool(void * owner = NULL, std::ostream *os = NULL)
        : mEnabled(true)
        , mOwner(owner)
        , mOStream(os)
        , mVar1(0)
    {
        setPrefix("[Log] ");
    }
    virtual ~LogTool() {}

    virtual int32_t log(const char* format, ...) {
        if (!mEnabled)
            return 0;
        int32_t r = 0;
        va_list args;
        va_start(args, format);
        if (mOStream) {
            snprintf(mBuffer, LOGTOOL_BUFFER_SIZE, "%s", mPrefix);
            r = vsnprintf(mBuffer, LOGTOOL_BUFFER_SIZE, format, args);
            if (r > 0) {
                mOStream->write(mBuffer, r);
            }
        } else {
            printf("%s", mPrefix);
            r = std::vprintf(format, args);
        }
        va_end(args);
        return r;
    }

    void * getOwner() {
        return mOwner;
    }

    void setPrefix(const char* prefix) {
        strncpy(mPrefix, prefix, LOGTOOL_PREFIX_MAX);
    }

    void setStream(std::ostream * os) {
        mOStream = os;
    }

    void setEnabled(bool enabled) {
        mEnabled = enabled;
    }

    bool getEnabled() {
        return mEnabled;
    }

    void setVar1(int value) {
        mVar1 = value;
    }

    int getVar1() {
        return mVar1;
    }

private:
    bool mEnabled;
    void * mOwner;
    std::ostream *mOStream;
    char mBuffer[LOGTOOL_BUFFER_SIZE + 1];
    char mPrefix[LOGTOOL_PREFIX_MAX + 1];
    int mVar1;    //user assigned variable.
};

#endif //SYNTHMARK_LOGTOOL_H
