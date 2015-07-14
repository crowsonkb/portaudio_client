/*
Reads samples from the OS X default audio device, with minimal latency. It uses
portaudio (http://portaudio.com) as a simplified audio I/O library.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "portaudio.h"
#include "zmq.h"

static void *context = NULL;
static void *publisher = NULL;  // Do not touch this outside callbacks.
const char *endpoint = "tcp://*:5556";

static int callback(const void *input_,
                    void *output_,
                    unsigned long frameCount,
                    const PaStreamCallbackTimeInfo *timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData_) {

    const float *input = input_;

    int rc;
    static int zmq_setup;
    if (!zmq_setup) {
        publisher = zmq_socket(context, ZMQ_PUB);
        assert(publisher);
        rc = zmq_bind(publisher, endpoint);
        if (rc != 0) { goto error; }
        zmq_setup = 1;
    }

    size_t len = frameCount*sizeof(float)*1;
    rc = zmq_send(publisher, input, len, 0);
    if (rc != len) { goto error; }

    return paContinue;

    error:
    if (rc < 0) {
        printf("ZMQ error: %s\n", zmq_strerror(zmq_errno()));
    } else {
        printf("An unknown error occurred (ZMQ rc: %d)", rc);
    }
    return paComplete;
}

static void callback_done(void *userData) {
    zmq_unbind(publisher, endpoint);
    zmq_close(publisher);
}

PaError init_stream(PaStream **stream) {
    PaError err;

    PaDeviceIndex device = Pa_GetDefaultInputDevice();
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(device);

    PaStreamParameters params;
    params.device = device;
    params.channelCount = 1;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = deviceInfo->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(stream, &params, NULL, deviceInfo->defaultSampleRate,
                        0, paClipOff, callback, NULL);
    if (err != paNoError) { goto error; }

    err = Pa_SetStreamFinishedCallback(*stream, callback_done);
    if (err != paNoError) { goto error; }

    err = Pa_StartStream(*stream);
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
    context = zmq_ctx_new();
    assert(context);

    PaError err;

    printf("Initializing PortAudio:\n");
    err = Pa_Initialize();
    if (err != paNoError) { goto error; }
    printf("%s\n", Pa_GetVersionText());

    err = display_devices();
    if (err != paNoError) { goto error; }

    PaStream *stream;
    err = init_stream(&stream);
    if (err != paNoError) { goto error; }

    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(500);
    }

    err = Pa_Terminate();
    if (err != paNoError) { goto error; }

    zmq_ctx_destroy(context);
    return 0;

    error:
    zmq_ctx_destroy(context);
    Pa_Terminate();
    fprintf(stderr, "\nExiting due to an error.\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
