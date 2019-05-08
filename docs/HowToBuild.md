# How to Build SynthMark

Instructions for building the app or the command line executable.

## How to Build the Android GUI App

* Start Android Studio 2.2.1 or later
* Click “Open an existing Android Studio project”
* Select pathname “synthmark/android”
* Click OK button
* Then build and run as usual

## Command Line Interface

### Building the Command for Running on Android

If you have not already, then install the NDK support for Android Studio.

#### Build using the NDK.

    cd synthmark/android_mk
	  ndk-build

If ndk-build cannot be found then add the NDK tools to your PATH then try again. On Linux, this might work:

    export PATH=$PATH:$HOME/Android/Sdk/ndk-bundle

On Mac OSX, this might work:

    export PATH=$PATH:$HOME/Library/Android/sdk/ndk-bundle

Push the executable program to your Android device. 
The “jni/Application.mk” file determines the target architecture, eg. “arm64-v8a”.

    adb root
    adb remount
    adb push libs/arm64-v8a/synthmark   /system/bin/.

### Building the Command for Running on Linux

    cd synthmark
    make -f linux/Makefile
