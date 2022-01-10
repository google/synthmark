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

#ifndef SYNTHMARK_SYNTHMARK_HARNESS_H
#define SYNTHMARK_SYNTHMARK_HARNESS_H

#include <cmath>
#include <cstdint>

#include "AudioSinkBase.h"
#include "BinCounter.h"
#include "HostTools.h"
#include "IAudioSinkCallback.h"
#include "SynthMark.h"
#include "SynthMarkResult.h"
#include "synth/Synthesizer.h"
#include "tools/CpuAnalyzer.h"
#include "tools/LogTool.h"
#include "tools/ITestHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/TestHarnessBase.h"
#include "HostThreadFactory.h"
#include "TestHarnessParameters.h"

constexpr int JITTER_BINS_PER_MSEC  = 10;
constexpr int JITTER_MAX_MSEC       = 100;

/**
 * Base class for running a test.
 */
class TestHarnessBase : public TestHarnessParameters, public IAudioSinkCallback  {
public:
    TestHarnessBase(AudioSinkBase *audioSink,
                    SynthMarkResult *result,
                    LogTool &logTool)
    : TestHarnessParameters(audioSink, result, logTool)
    , mSynth()
    , mTimer()
    , mCpuAnalyzer()
    , mSampleRate(kSynthmarkSampleRate)
    , mSamplesPerFrame(2)
    , mFrameCounter(0)
    , mDelayNotesOnUntilFrame(0)
    , mNoteCounter(0)
    {
    }

    void setupHistograms() {
        // set resolution and size of histogram
        int32_t nanosPerMilli = (int32_t) (SYNTHMARK_NANOS_PER_SECOND /
                                           SYNTHMARK_MILLIS_PER_SECOND);
        mNanosPerBin = nanosPerMilli / JITTER_BINS_PER_MSEC;
        int32_t numBins = JITTER_MAX_MSEC * JITTER_BINS_PER_MSEC;
        mTimer.setupHistograms(mNanosPerBin, numBins);
    }

    // Customize the test by defining these virtual methods.
    virtual void onBeginMeasurement() {}

    // TODO Use Result type return value.
    virtual int32_t onBeforeNoteOn() { return 0; }

    virtual void onEndMeasurement() {}

    // Run the benchmark.
    int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) override {
        int32_t err = open(sampleRate, SAMPLES_PER_FRAME,
                           kSynthmarkFramesPerRender, framesPerBurst);
        if (err) {
            return err;
        }
        err = measure(numSeconds);
        close();
        return err;
    };

    // This is called by the AudioSink in a loop.
    virtual IAudioSinkCallback::Result onRenderAudio(float *buffer,
                                                     int32_t numFrames) override {
        // mLogTool.log("onRenderAudio() callback called\n");
        int32_t result;
        if (mFrameCounter >= mFramesNeeded) {
            return IAudioSinkCallback::Result::Finished;
        }

        // Only start turning notes on and off after the initial delay
        if (mFrameCounter >= mDelayNotesOnUntilFrame){
            // Turn notes on and off so they never stop sounding.
            if (mBurstCountdown <= 0) {
                if (mAreNotesOn) {
                    mSynth.allNotesOff();
                    mBurstCountdown = mBurstsOff;
                    mAreNotesOn = false;
                    mNoteCounter++;
                } else {
                    result = onBeforeNoteOn();
                    if (result < 0) {
                        mLogTool.log("%s() onBeforeNoteOn() returned %d\n", __func__, result);
                        mResult->setResultCode(result);
                        return IAudioSinkCallback::Result::Finished;
                    }
                    int32_t currentNumVoices = getCurrentNumVoices();
                    HostCpuManager::getInstance()->setApplicationLoad(currentNumVoices,
                                                                      kSynthmarkMaxVoices);
                    result = mSynth.notesOn(currentNumVoices);
                    if (result < 0) {
                        mLogTool.log("%s() allNotesOn() returned %d\n", __func__, result);
                        mResult->setResultCode(result);
                        return IAudioSinkCallback::Result::Finished;
                    }
                    mBurstCountdown = mBurstsOn;
                    mAreNotesOn = true;
                }
            }
            mBurstCountdown--;
        }

        // Gather timing information.
        // mLogTool.log("onRenderAudio() call the synthesizer\n");
        // Calculate time when we ideally should have woken up.
        int64_t fullFramePosition = mAudioSink->getFramesWritten()
                                    - mAudioSink->getBufferSizeInFrames()
                                    - mFramesPerBurst;
        int64_t idealTime = mAudioSink->convertFrameToTime(fullFramePosition);
        mTimer.markEntry(idealTime);
        mSynth.renderStereo(buffer, numFrames);  // DO THE MATH!
        mTimer.markExit();

        mCpuAnalyzer.recordCpu(); // at end so we have less affect on timing

        mLogTool.setVar1(mBurstCounter);

        mFrameCounter += numFrames;
        mBurstCounter++;

        return IAudioSinkCallback::Result::Continue;
    }

    /**
     * Perform a SynthMark measurement. Results are stored in the SynthMarkResult object
     * passed in the constructor.  Audio may be rendered in a background thread.
     *
     * @param seconds - duration of test
     */
    int32_t measure(double seconds) {

        assert(mResult != NULL);

        int32_t result; // Used to store the results of various operations during the test
        mFramesNeeded = (int)(mSampleRate * seconds);
        mFrameCounter = 0;
        mBurstCounter = 0;
        mLogTool.setVar1(mBurstCounter);

        // Variables for turning notes on and off.
        mAreNotesOn = false;
        mBurstCountdown = 0;
        mBurstsOn = (int) (0.2 * mSampleRate / mFramesPerBurst);
        mBurstsOff = (int) (0.3 * mSampleRate / mFramesPerBurst);

        onBeginMeasurement();

        mAudioSink->setCallback(this);

        result = mAudioSink->start();
        if (result < 0){
            mResult->setResultCode(SYNTHMARK_RESULT_AUDIO_SINK_START_FAILURE);
            return result;
        }

        // Run the test or wait for it to finish.
        result = mAudioSink->runCallbackLoop();
        if (result < 0) {
            mLogTool.log("ERROR runCallbackLoop() failed, returned %d\n", result);
            mResult->setResultCode(result);
            return result;
        }

        mAudioSink->stop();
        onEndMeasurement();

        mResult->setResultCode(result);
        return result;
    }

    std::string dumpJitter() {
        return mTimer.dumpJitter();
    }


    virtual int32_t getCurrentNumVoices() {
        return getNumVoices();
    }

    int32_t getNoteCounter() {
        return mNoteCounter;
    }

    void setDelayNoteOnSeconds(int32_t delayInSeconds) override {
        mDelayNotesOnUntilFrame = (int32_t)(delayInSeconds * mSampleRate);
    }

    const char *getName() const override {
        return mTestName.c_str();
    }

    virtual int32_t open(int32_t sampleRate,
                 int32_t samplesPerFrame,
                 int32_t framesPerRender,
                 int32_t framesPerBurst) {
        if (sampleRate < 8000) {
            mLogTool.log("ERROR in open, sampleRate too low = %d < 8000\n", sampleRate);
            return -1;
        }
        if (samplesPerFrame != 2) {
            mLogTool.log("ERROR in open, samplesPerFrame = %d != 2\n", samplesPerFrame);
            return -1;
        }
        if (framesPerRender < 1) {
            mLogTool.log("ERROR in open, framesPerRender too low = %d < 1\n", framesPerRender);
            return -1;
        }
        if (framesPerRender > kSynthmarkFramesPerRender) {
            mLogTool.log("ERROR in open, framesPerRender = %d > %d\n",
                          framesPerRender, kSynthmarkFramesPerRender);
            return -1;
        }
        if (framesPerBurst < 8) {
            mLogTool.log("ERROR in open, framesPerBurst = %d < 8\n", framesPerRender);
            return -1;
        }
        mSampleRate = sampleRate;
        mSamplesPerFrame = samplesPerFrame;
        mFramesPerBurst = framesPerBurst;

        mSynth.setup(sampleRate, kSynthmarkMaxVoices);
        return mAudioSink->open(sampleRate, samplesPerFrame, framesPerBurst);
    }

    int32_t close() {
        return mAudioSink->close();
    }

    bool isVerbose() {
        return mVerbose;
    }

    HostThreadFactory::ThreadType getThreadType() const {
        return mAudioSink->getThreadType();
    }

    void setThreadType(HostThreadFactory::ThreadType mThreadType) override {
        mAudioSink->setThreadType(mThreadType);
    }

    int32_t getFrameCount() {
        return mFrameCounter;
    }

protected:
    Synthesizer      mSynth;
    TimingAnalyzer   mTimer;
    CpuAnalyzer      mCpuAnalyzer;
    std::string      mTestName;

    int32_t          mSampleRate = 0;
    int32_t          mSamplesPerFrame = 0;
    int32_t          mFramesPerBurst = 0;  // number of frames read by hardware at one time
    int32_t          mFrameCounter = 0;
    int32_t          mFramesNeeded = 0;
    int32_t          mDelayNotesOnUntilFrame = 0;
    int32_t          mNoteCounter = 0;
    int32_t          mBurstCounter = 0;
    int32_t          mNanosPerBin = 1;

    // Variables for turning notes on and off.
    bool             mAreNotesOn = false;
    int32_t          mBurstCountdown = 0;
    int32_t          mBurstsOn = 0;
    int32_t          mBurstsOff = 0;

private:
    bool             mVerbose = false;
};

#endif // SYNTHMARK_SYNTHMARK_HARNESS_H
