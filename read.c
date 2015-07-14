#include <stdio.h>
#include <stdlib.h>

#include "portaudio.h"

static int callback(const void *input,
                    void *output,
                    unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData) {

    printf("\nHello from inside the callback!\n");
    printf("Frames per buffer: %lu\n", frameCount);
    printf("Stream callback invoked at time: %f\n", timeInfo->currentTime);
    printf("First sample of input at time: %f\n", timeInfo->inputBufferAdcTime);
    return paComplete;
}

PaError init_stream() {
    PaError err;

    PaDeviceIndex device = Pa_GetDefaultInputDevice();
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(device);

    PaStreamParameters params;
    params.device = device;
    params.channelCount = 1;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = deviceInfo->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    PaStream *stream;
    err = Pa_OpenStream(&stream, &params, NULL, deviceInfo->defaultSampleRate,
                        0, paClipOff, callback, NULL);
    if (err != paNoError) { goto error; }

    err = Pa_StartStream(stream);
    if (err != paNoError) { goto error; }

    // TODO: set a stream-end callback
    Pa_Sleep(500);

    err = Pa_StopStream(stream);
    if (err != paNoError) { goto error; }

    return paNoError;

    error:
    return err;
}

PaError display_devices() {
    PaError err;

    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) { err = numDevices; goto error; }
    int defaultInput = Pa_GetDefaultInputDevice();
    int defaultOutput = Pa_GetDefaultOutputDevice();

    printf("%d devices found:\n", numDevices);

    for (int i = 0; i < numDevices; i++) {
        int important = (i == defaultInput) || (i == defaultOutput);
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);

        printf("Device %d: %s\n", i, deviceInfo->name);
        if (!important) { continue; }

        printf("    Channels: %d in, %d out\n", \
            deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
        printf("    Sample rate: %.0f Hz\n", deviceInfo->defaultSampleRate);

        if (i == defaultInput) {
            printf("    Non-interactive input latency:  %f s\n", \
                deviceInfo->defaultHighInputLatency);
            printf("        Interactive input latency:  %f s\n", \
                deviceInfo->defaultLowInputLatency);
        }

        if (i == defaultOutput) {
            printf("    Non-interactive output latency: %f s\n", \
                deviceInfo->defaultHighOutputLatency);
            printf("        Interactive output latency: %f s\n", \
                deviceInfo->defaultLowOutputLatency);
        }
    }
    return paNoError;

    error:
    return err;
}

int main(int argc, char const *argv[]) {
    PaError err;

    printf("Initializing PortAudio:\n");
    err = Pa_Initialize();
    if (err != paNoError) { goto error; }
    printf("%s\n", Pa_GetVersionText());

    err = display_devices();
    if (err != paNoError) { goto error; }

    err = init_stream();
    if (err != paNoError) { goto error; }

    err = Pa_Terminate();
    if (err != paNoError) { goto error; }
    return 0;

    error:
    Pa_Terminate();
    fprintf(stderr, "\nExiting due to an error.\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
