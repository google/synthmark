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

#include <cstdint>
#include <math.h>
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/TimingAnalyzer.h"
#include "AudioSinkBase.h"
#include "tools/LogTool.h"
#include "SynthMarkResult.h"
#include "IAudioSinkCallback.h"

/**
 * Base class for running a test.
 */
class TestHarnessBase : public IAudioSinkCallback {
public:
    TestHarnessBase(AudioSinkBase *audioSink,
                    SynthMarkResult *result,
                    LogTool *logTool = NULL)
    : mSynth()
    , mAudioSink(NULL)
    , mTimer()
    , mLogTool(NULL)
    , mResult(result)
    , mSampleRate(SYNTHMARK_SAMPLE_RATE)
    , mSamplesPerFrame(2)
    , mNumVoices(8)
    , mFrameCounter(0)
    , mDelayNotesOnUntilFrame(0)
    , mNoteCounter(0)

    {
        mAudioSink = audioSink;

        if (!logTool) {
            mLogTool = new LogTool(this);
        } else {
            mLogTool = logTool;
        }
    }

    virtual ~TestHarnessBase() {
        if (mLogTool && mLogTool->getOwner() == this) {
            delete(mLogTool);
            mLogTool = NULL;
        }
    }

    // Customize the test by defining these virtual methods.
    virtual void onBeginMeasurement() = 0;

    virtual int32_t onBeforeNoteOn() = 0;

    virtual void onEndMeasurement() = 0;

    int32_t open(int32_t sampleRate,
            int32_t samplesPerFrame,
            int32_t framesPerRender,
            int32_t framesPerBurst
            ) {
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
        if (framesPerRender > SYNTHMARK_FRAMES_PER_RENDER) {
            mLogTool->log("ERROR in open, framesPerRender = %d > %d\n",
                framesPerRender, SYNTHMARK_FRAMES_PER_RENDER);
            return -1;
        }
        if (framesPerBurst < 8) {
            mLogTool->log("ERROR in open, framesPerBurst = %d < 8\n", framesPerRender);
            return -1;
        }
        mSampleRate = sampleRate;
        mSamplesPerFrame = samplesPerFrame;
        mFramesPerBurst = framesPerBurst;

        mSynth.setup(sampleRate, SYNTHMARK_MAX_VOICES);
        return mAudioSink->open(sampleRate, samplesPerFrame, framesPerBurst);
    }

    // This is called by the AudioSink in a loop.
    virtual audio_sink_callback_result_t renderAudio(float *buffer,
                                                     int32_t numFrames) override {
        // mLogTool->log("renderAudio() callback called\n");
        int32_t result;
        if (mFrameCounter >= mFramesNeeded) {
            return CALLBACK_FINISHED;
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
                        return CALLBACK_ERROR;
                    }
                    result = mSynth.allNotesOn(getCurrentNumVoices());
                    if (result < 0) {
                        mLogTool->log("renderAudio() allNotesOn() returned %d\n", result);
                        mResult->setResultCode(result);
                        return CALLBACK_ERROR;
                    }
                    mBurstCountdown = mBurstsOn;
                    mAreNotesOn = true;
                }
            }
            mBurstCountdown--;
        }

        // Gather timing information.
        // mLogTool->log("renderAudio() call the synthesizer\n");
        mTimer.markEntry();
        mSynth.renderStereo(buffer, numFrames);
        // Ideally we should write the data earlier than when it is read,
        // by a full buffer's worth of time.
        int64_t fullFramePosition = mAudioSink->getFramesWritten()
                                    - mAudioSink->getBufferSizeInFrames()
                                    - mFramesPerBurst;
        int64_t idealTime = mAudioSink->convertFrameToTime(fullFramePosition);
        mTimer.markExit(idealTime);

        mFrameCounter += numFrames;
        mLogTool->setVar1(mFrameCounter);

        return CALLBACK_CONTINUE;
    }

    /**
     * Perform a SynthMark measurement. Results are stored in the SynthMarkResult object
     * passed in the constructor.  Audio may be rendered in a background thread.
     *
     * @param seconds - duration of test
     */
    void measure(double seconds) {

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
            return;
        }

        // Run the test or wait for it to finish.
        result = mAudioSink->runCallbackLoop();
        if (result < 0) {
            mLogTool->log("ERROR runCallbackLoop() failed, returned %d\n", result);
            mResult->setResultCode(result);
            return;
        }

        mAudioSink->stop();
        onEndMeasurement();

        mLogTool->log("SCHED_FIFO %sused\n", mAudioSink->wasSchedFifoUsed() ? "" : "NOT ");
        mLogTool->log("bufferSizeInFrames     = %6d\n", mAudioSink->getBufferSizeInFrames());
        mLogTool->log("bufferCapacityInFrames = %6d\n", mAudioSink->getBufferCapacityInFrames());
        mLogTool->log("sampleRate             = %6d\n", mAudioSink->getSampleRate());
        mLogTool->log("measured CPU load      = %6.2f%%\n", mTimer.getDutyCycle() * 100);
    }

    int32_t close() {
        return mAudioSink->close();
    }

    void setNumVoices(int32_t numVoices) {
        mNumVoices = numVoices;
    }

    int32_t getNumVoices() {
        return mNumVoices;
    }

    virtual int32_t getCurrentNumVoices() {
        return mNumVoices;
    }

    int32_t getNoteCounter() {
        return mNoteCounter;
    }

    void setDelayNotesOn(int32_t delayInSeconds){
        mDelayNotesOnUntilFrame = (int32_t)(delayInSeconds * mSampleRate);
    }

    const char *getName() {
        return mTestName.c_str();
    }

protected:
    Synthesizer    mSynth;
    AudioSinkBase *mAudioSink;
    TimingAnalyzer mTimer;
    LogTool       *mLogTool;
    SynthMarkResult *mResult;
    std::string    mTestName;
    int32_t        mSampleRate;
    int32_t        mSamplesPerFrame;
    int32_t        mFramesPerBurst;  // number of frames read by hardware at one time
    int32_t        mNumVoices;
    int32_t        mFrameCounter;
    int32_t        mFramesNeeded;
    int32_t        mDelayNotesOnUntilFrame;
    int32_t        mNoteCounter;

    // Variables for turning notes on and off.
    bool          mAreNotesOn;
    int32_t       mBurstCountdown;
    int32_t       mBurstsOn;
    int32_t       mBurstsOff;
};


#endif // SYNTHMARK_SYNTHMARK_HARNESS_H
