# How to Run SynthMark App from an Android Intent

This allows you to run SynthMark app from a shell script.
This can be used when you do not have a rooted device and cannot use the command line executable.

## Start App from Intent

The app can be started by sending a Start comment to the SynthMark class.
The app will run and the results will be written to a file.

    adb shell am start -n com.sonodroid.synthmark/.MainActivity {parameters}
    
String parameters are sent using:

    --es {parameterName} {parameterValue}

For example:

    --es test jitter

Integer parameters are sent using:

    --ei {parameterName} {parameterValue}
    
For example:

    --ei num_voices 32

## Parameters

There are two required parameters:

    --es test { voice, util, jitter, latency, auto }
    --es file {full path for resulting file}
    
There are several optional parameters. These are defined in the file NativeTest.h

    --ei sample_rate        {hertz}
    --ei samples_per_frame  {samples}
    --ei frames_per_render  {frames}
    --ei frames_per_burst   {frames}
    --ei num_seconds        {value}
    --ei note_on_delay      {seconds}
    --ei target_cpu_load    {percentage}
    --ei num_voices         {count}
    --ei num_voices_high    {count}
    --ei core_affinity      {index}

For example, a complete command might be:

    adb shell am start -n com.sonodroid.synthmark/.MainActivity \
        --es test latency \
        --es file /sdcard/test20190611.txt \
        --ei core_affinity 1 \
        --ei num_voices 16 \
        --ei num_voices_high 64
        
        
        
