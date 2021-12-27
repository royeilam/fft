#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <portaudio.h>

/**************************************************
 * Defines
 **************************************************/
#define DBG_PRINT(fmt, ...) \
printf("[DEBUG::%s.%d] " fmt,__FUNCTION__ ,__LINE__, ##__VA_ARGS__);\
fflush(stdout)

#define SAMPLE_RATE 1000
#define RECORD_TIME 5
#define DATA_SIZE (SAMPLE_RATE * RECORD_TIME + 1)

typedef int32_t record_data_t;


static record_data_t record_data[DATA_SIZE] = { 0 };

static struct RecordCBData
{
    bool stop_record;
    record_data_t * data;
    size_t frame_count;
    const size_t max_frames; 
    double sample_rate;
    double record_time;
} record_cb_data = 
{
    .stop_record = false,
    .data = record_data,
    .frame_count = 0,
    .max_frames = DATA_SIZE,
    .sample_rate = SAMPLE_RATE,
    .record_time = RECORD_TIME,
};

static int audio_in_cb( const void *input, void *output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData )
{
    (void)timeInfo;
    (void)output;
    (void)statusFlags;
    (void)userData;

    if (input && record_cb_data.frame_count < record_cb_data.max_frames)
    {
        size_t diff = record_cb_data.max_frames - record_cb_data.frame_count;
        if (frameCount > diff)
        {
            frameCount -= diff;
        }

        for (unsigned long i = 0; i < frameCount; i++)
        {
            record_cb_data.data[record_cb_data.frame_count++] = ((record_data_t *)input)[i];
        }

        if (record_cb_data.max_frames == record_cb_data.frame_count)
        {
            return paComplete;
        }
    }

    if (record_cb_data.stop_record)
    {
        return paComplete;
    }

    return paContinue;
}

static int pa_init( PaStream ** const stream, const double fs,
                    PaStreamCallback * const callback,
                    void * const user_data)
{
    PaError pa_err_code;
    const unsigned long FRAMES_PER_BUFFER = 1024;
    const PaDeviceInfo * device_info = NULL;
    PaStreamParameters pa_input_parameters;

    DBG_PRINT("Before Initializing PortAudio\n");
    if ( (pa_err_code = Pa_Initialize()) != paNoError )
    {
        fprintf(stderr, "Cannot initialize PortAudio library.\n"
                "Error: %s\n", Pa_GetErrorText(pa_err_code));
        return -1;
    }

    pa_input_parameters.device = Pa_GetDefaultInputDevice(),
    pa_input_parameters.channelCount = 1,
    pa_input_parameters.sampleFormat = paInt32,
    pa_input_parameters.hostApiSpecificStreamInfo = NULL,
    device_info = Pa_GetDeviceInfo(pa_input_parameters.device);
    if (device_info == NULL)
    {
        fprintf(stderr, "[PortAudio]: Cannot open device information for read\n");
        return -1;
    }
    pa_input_parameters.suggestedLatency = device_info->defaultHighInputLatency;

    DBG_PRINT("Start Recording\n");
    pa_err_code = Pa_OpenStream
    (
        stream,               // Stream
        &pa_input_parameters, // Input Parameters
        NULL,                 // Output Parameters
        fs,                   // Sample Rate
        FRAMES_PER_BUFFER,    // Number of samples (frames) to be passed oppon calling CB
        paNoFlag,             // Control Flags
        callback,             // Callback function pointer
        user_data             // User data
    );

    if (pa_err_code != paNoError)
    {
        fprintf(stderr, "Cannot opean stream for record\n"
                "Error: %s\n", Pa_GetErrorText(pa_err_code));
        return -1;
    }

    return 0;

}

static int write_data_to_file(const char file_name[],
                               record_data_t data[], size_t data_len)
{
    FILE * file_handler = NULL;

    if (file_name == NULL || data == NULL || data_len == 0)
    {
        return EINVAL;
    }

    file_handler = fopen(file_name, "w");
    if (file_handler == NULL)
    {
        return errno;
    }

    fwrite(data, sizeof(*data), data_len, file_handler);

    fclose(file_handler);

    return 0;
}

int main(int argc, char * argv[])
{
    PaError pa_err_code;
    PaStream * stream;
    (void)argc;
    (void)argv;

    DBG_PRINT("Before calling pa_init()\n");
    if (pa_init(&stream, record_cb_data.sample_rate, audio_in_cb, NULL))
    {
        return EXIT_FAILURE;
    }

    Pa_StartStream(stream);

    while ((pa_err_code = Pa_IsStreamActive(stream)) == 1)
    {
        Pa_Sleep(RECORD_TIME * 1000 + 500);
        record_cb_data.stop_record = true;
    }

    if ( (pa_err_code = Pa_CloseStream(stream)) != paNoError)
    {
        fprintf(stderr, "Cannot close PortAudio stream.\n"
                "Error: %s\n", Pa_GetErrorText(pa_err_code));
    }

    DBG_PRINT("Stopped Recording\n");
    if ( (pa_err_code = Pa_Terminate()) != paNoError )
    {
        fprintf(stderr, "Error closing Portable Audio library.\n"
                "Error: %s\n", Pa_GetErrorText(pa_err_code));
        return EXIT_FAILURE;
    }

    if (write_data_to_file("test_file.raw",
                           record_cb_data.data,
                           record_cb_data.frame_count))
    {
        fprintf(stderr, "Error writing to file\n");
    }

    return EXIT_SUCCESS;
}
