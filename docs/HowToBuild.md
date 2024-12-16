# How to Build SynthMark

The SynthMark app is [available on the Play Store](https://play.google.com/store/apps/details?id=com.mobileer.synthmark).
But if you want the latest version or the command line executable then continue.

Instructions for building the app for Android
or the command line executable for Android or Linux.

## How to Build the Android GUI App

* Start Android Studio 2.2.1 or later
* Click “Open an existing Android Studio project”
* Select pathname “synthmark/android”
* Click OK button
* Then build and run as usual

## Building the Command Line Executable for Android

If you have not already, then install the NDK support for Android Studio.
Then enter:

    cd synthmark/android_mk
    ndk-build

If ndk-build cannot be found then add the NDK tools to your PATH then try again. 

### Linux

On Linux, this might work:

    export PATH=$PATH:$HOME/Android/Sdk/ndk-bundle

### Mac OS

On Mac OSX, this used to work:

    export PATH=$PATH:$HOME/Library/Android/sdk/ndk-bundle

But recently I had to use 'ls' to find a recent version of the NDK.

    ls $HOME/Library/Android/Sdk/ndk

Look for the most recent version and then set the PATH to use that version.

    export PATH=$PATH:/Users/philburk/Library/Android/sdk/ndk/25.1.8937393/

### Installing SynthMark

Push the executable program to your Android device. 
The “jni/Application.mk” file determines the target architecture, eg. “arm64-v8a”.

    adb root
    adb remount -R
    adb push libs/arm64-v8a/synthmark   /system/bin/.

## Building the Command Line Executable for Linux

    cd synthmark
    make -f linux/Makefile
