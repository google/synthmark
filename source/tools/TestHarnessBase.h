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

constexpr int JITTER_BINS_PER_MSEC  = 10;
constexpr int JITTER_MAX_MSEC       = 100;

/**
 * Base class for running a test.
 */
class TestHarnessBase : public ITestHarness, IAudioSinkCallback {
public:
    TestHarnessBase(AudioSinkBase *audioSink,
                    SynthMarkResult *result,
                    LogTool *logTool = nullptr)
    : mSynth()
    , mAudioSink(audioSink)
    , mTimer()
    , mCpuAnalyzer()
    , mLogTool(nullptr)
    , mResult(result)
    , mSampleRate(kSynthmarkSampleRate)
    , mSamplesPerFrame(2)
    , mFrameCounter(0)
    , mDelayNotesOnUntilFrame(0)
    , mNoteCounter(0)
    , mNumVoices(8)
    {
        if (!logTool) {
            mLogTool = new LogTool(this);
        } else {
            mLogTool = logTool;
        }
    }

    virtual ~TestHarnessBase() {
        if (mLogTool && mLogTool->getOwner() == this) {
            delete(mLogTool);
            mLogTool = nullptr;
        }
    }

    void setupJitterRecording() {
        // set resolution and size of histogram
        int32_t nanosPerMilli = (int32_t) (SYNTHMARK_NANOS_PER_SECOND /
                                           SYNTHMARK_MILLIS_PER_SECOND);
        mNanosPerBin = nanosPerMilli / JITTER_BINS_PER_MSEC;
        int32_t numBins = JITTER_MAX_MSEC * JITTER_BINS_PER_MSEC;
        mTimer.setupJitterRecording(mNanosPerBin, numBins);
    }

    // Customize the test by defining these virtual methods.
    virtual void onBeginMeasurement() = 0;

    // TODO Use Result type return value.
    virtual int32_t onBeforeNoteOn() = 0;

    virtual void onEndMeasurement() = 0;

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
    virtual IAudioSinkCallback::Result renderAudio(float *buffer,
                                                     int32_t numFrames) override {
        // mLogTool->log("renderAudio() callback called\n");
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
                        mLogTool->log("renderAudio() onBeforeNoteOn() returned %d\n", result);
                        mResult->setResultCode(result);
                        return IAudioSinkCallback::Result::Finished;
                    }
                    int32_t currentNumVoices = getCurrentNumVoices();
                    HostCpuManager::getInstance()->setApplicationLoad(currentNumVoices,
                                                                      kSynthmarkMaxVoices);
                    result = mSynth.notesOn(currentNumVoices);
                    if (result < 0) {
                        mLogTool->log("renderAudio() allNotesOn() returned %d\n", result);
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
        // mLogTool->log("renderAudio() call the synthesizer\n");
        // Calculate time when we ideally would have woken up.
        int64_t fullFramePosition = mAudioSink->getFramesWritten()
                                    - mAudioSink->getBufferSizeInFrames()
                                    - mFramesPerBurst;
        int64_t idealTime = mAudioSink->convertFrameToTime(fullFramePosition);
        mTimer.markEntry(idealTime);
        mSynth.renderStereo(buffer, numFrames);  // DO THE MATH!
        mTimer.markExit();

        mCpuAnalyzer.recordCpu(); // at end so we have less affect on timing

        mFrameCounter += numFrames;
        mLogTool->setVar1(mFrameCounter);

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

        mLogTool->log("---- SynthMark version %d.%d ----\n", SYNTHMARK_MAJOR_VERSION,
                      SYNTHMARK_MINOR_VERSION);

        int32_t result; // Used to store the results of various operations during the test
        mFramesNeeded = (int)(mSampleRate * seconds);
        mFrameCounter = 0;
        mLogTool->setVar1(mFrameCounter);

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
            mLogTool->log("ERROR runCallbackLoop() failed, returned %d\n", result);
            mResult->setResultCode(result);
            return result;
        }

        mAudioSink->stop();
        onEndMeasurement();

        mLogTool->log("SCHED_FIFO %sused\n", mAudioSink->wasSchedFifoUsed() ? "" : "NOT ");
        mLogTool->log("bufferSizeInFrames     = %6d\n", mAudioSink->getBufferSizeInFrames());
        mLogTool->log("bufferCapacityInFrames = %6d\n", mAudioSink->getBufferCapacityInFrames());
        mLogTool->log("sampleRate             = %6d\n", mAudioSink->getSampleRate());
        mLogTool->log("measured CPU load      = %6.2f%%\n", mTimer.getDutyCycle() * 100);
        mLogTool->log("CPU affinity           = %6d\n", mAudioSink->getActualCpu());

        mResult->setResultCode(result);
        return result;
    }

    std::string dumpJitter() {
        const bool showDeliveryTime = false;
        std::stringstream resultMessage;
        // Print jitter histogram
        BinCounter *wakeupBins = mTimer.getWakeupBins();
        BinCounter *renderBins = mTimer.getRenderBins();
        BinCounter *deliveryBins = mTimer.getDeliveryBins();
        if (wakeupBins != NULL && renderBins != NULL && deliveryBins != NULL) {
            resultMessage << TEXT_CSV_BEGIN << std::endl;
            int32_t numBins = deliveryBins->getNumBins();
            const int32_t *wakeupCounts = wakeupBins->getBins();
            const int32_t *wakeupLast = wakeupBins->getLastMarkers();
            const int32_t *renderCounts = renderBins->getBins();
            const int32_t *renderLast = renderBins->getLastMarkers();
            const int32_t *deliveryCounts = deliveryBins->getBins();
            const int32_t *deliveryLast = deliveryBins->getLastMarkers();
            resultMessage << " bin#,  msec,"
                          << "   wakeup#,  wlast,"
                          << "   render#,  rlast";
            if (showDeliveryTime) {
                resultMessage << " delivery#,  dlast";
            }
            resultMessage << std::endl;
            for (int i = 0; i < numBins; i++) {
                if (wakeupCounts[i] > 0 || renderCounts[i] > 0
                        || (deliveryCounts[i] > 0 && showDeliveryTime)) {
                    double msec = (double) i * mNanosPerBin * SYNTHMARK_MILLIS_PER_SECOND
                                  / SYNTHMARK_NANOS_PER_SECOND;
                    resultMessage << "  " << std::setw(3) << i
                                  << ", " << std::fixed << std::setw(5) << std::setprecision(2)
                                  << msec
                                  << ", " << std::setw(9) << wakeupCounts[i]
                                  << ", " << std::setw(6) << wakeupLast[i]
                                  << ", " << std::setw(9) << renderCounts[i]
                                  << ", " << std::setw(6) << renderLast[i];
                    if (showDeliveryTime) {
                        resultMessage << ", " << std::setw(9) << deliveryCounts[i]
                              << ", " << std::setw(6) << deliveryLast[i];
                    }
                    resultMessage << std::endl;
                }
            }
            resultMessage << TEXT_CSV_END << std::endl;
        } else {
            resultMessage << "ERROR NULL BinCounter!\n";
        }
        return resultMessage.str();
    }


    void setNumVoices(int32_t numVoices) override {
        mNumVoices = numVoices;
    }

    int32_t getNumVoices() {
        return mNumVoices;
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

    const char *getName() override {
        return mTestName.c_str();
    }

    virtual int32_t open(int32_t sampleRate,
                 int32_t samplesPerFrame,
                 int32_t framesPerRender,
                 int32_t framesPerBurst) {
        if (sampleRate < 8000) {
            mLogTool->log("ERROR in open, sampleRate too low = %d < 8000\n", sampleRate);
            return -1;
        }
        if (samplesPerFrame != 2) {
            mLogTool->log("ERROR in open, samplesPerFrame = %d != 2\n", samplesPerFrame);
            return -1;
        }
        if (framesPerRender < 1) {
            mLogTool->log("ERROR in open, framesPerRender too low = %d < 1\n", framesPerRender);
            return -1;
        }
        if (framesPerRender > kSynthmarkFramesPerRender) {
            mLogTool->log("ERROR in open, framesPerRender = %d > %d\n",
                          framesPerRender, kSynthmarkFramesPerRender);
            return -1;
        }
        if (framesPerBurst < 8) {
            mLogTool->log("ERROR in open, framesPerBurst = %d < 8\n", framesPerRender);
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

protected:
    Synthesizer      mSynth;
    AudioSinkBase   *mAudioSink;
    TimingAnalyzer   mTimer;
    CpuAnalyzer      mCpuAnalyzer;
    LogTool         *mLogTool;
    SynthMarkResult *mResult;
    std::string      mTestName;

    int32_t          mSampleRate = 0;
    int32_t          mSamplesPerFrame = 0;
    int32_t          mFramesPerBurst = 0;  // number of frames read by hardware at one time
    int32_t          mFrameCounter = 0;
    int32_t          mFramesNeeded = 0;
    int32_t          mDelayNotesOnUntilFrame = 0;
    int32_t          mNoteCounter = 0;
    int32_t          mNanosPerBin = 1;

    // Variables for turning notes on and off.
    bool             mAreNotesOn = false;
    int32_t          mBurstCountdown = 0;
    int32_t          mBurstsOn = 0;
    int32_t          mBurstsOff = 0;

private:
    int32_t          mNumVoices = 0;
};

#endif // SYNTHMARK_SYNTHMARK_HARNESS_H
