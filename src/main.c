#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <portaudio.h>
#include <pthread.h>
#include "fft.h"

/**************************************************
 * Defines
 **************************************************/
#define DBG_PRINT(fmt, ...) \
printf("[DEBUG::%s.%d] " fmt,__FUNCTION__ ,__LINE__, ##__VA_ARGS__);\
fflush(stdout)

#define PA_ASSERT(func_call, fail_msg_formt, ...) \
{ \
    PaError err_code = func_call; \
    if ( err_code != paNoError) \
    { \
        fprintf(stderr, "PortAudio ERROR (%s.%d):\nPortAudio Error: %s\n" \
        fail_msg_formt, __func__, \
        __LINE__, Pa_GetErrorText(err_code), ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } \
}

#define BUFFER_SIZE 1024
#define NUM_BUFFERS 2

/**************************************************
 * Typedefs
 **************************************************/
typedef float record_data_t;

/**************************************************
 * Local variables
 **************************************************/
static struct termios orig_termios;
static record_data_t record_data[NUM_BUFFERS][BUFFER_SIZE] = { 0 };
static pthread_spinlock_t new_data_lock;

/**************************************************
 * Local functions
 **************************************************/
static void disable_raw_terminal(void)
{
    assert(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) != -1);
}

static void enable_raw_terminal(void)
{
    struct termios raw;

    assert(tcgetattr(STDIN_FILENO, &orig_termios) != -1);
    raw = orig_termios;

    // Call disable_raw_terminal function when exiting the process
    atexit(disable_raw_terminal);
    
    // Modify termios flags
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    assert(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != -1);
}

static bool check_if_q(void)
{
    ssize_t num_char;
    char c;

    num_char = read(STDOUT_FILENO, &c, 1);
    if (num_char == -1 && errno != EAGAIN) assert(0);
    if (num_char == -1)
    {
        return false;
    }

    if (c == 'q' || c == 'Q')
    {
        return true;
    }

    return false;
}

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
    int * buffer_idx = (int *)userData;
    record_data_t * data = NULL;

    *buffer_idx ^= 1;
    data = &record_data[*buffer_idx][0];

    for (unsigned long i = 0; i < frameCount; i++)
    {
        data[i] = ((record_data_t *)input)[i];
    }

    pthread_spin_unlock(&new_data_lock);
    return paContinue;
}

static int pa_init( PaStream ** const stream, const double fs,
                    PaStreamCallback * const callback,
                    void * const user_data)
{
    const unsigned long FRAMES_PER_BUFFER = 1024;
    const PaDeviceInfo * device_info = NULL;
    PaStreamParameters pa_input_parameters;

    PA_ASSERT(Pa_Initialize(),"Cannot initialize PortAudio library.");

    pa_input_parameters.device = Pa_GetDefaultInputDevice(),
    pa_input_parameters.channelCount = 1,
    pa_input_parameters.sampleFormat = paFloat32,
    pa_input_parameters.hostApiSpecificStreamInfo = NULL,
    device_info = Pa_GetDeviceInfo(pa_input_parameters.device);
    assert(device_info != NULL);
    pa_input_parameters.suggestedLatency = device_info->defaultLowInputLatency;

    PA_ASSERT(Pa_OpenStream
    (
        stream,               // Stream
        &pa_input_parameters, // Input Parameters
        NULL,                 // Output Parameters
        fs,                   // Sample Rate
        FRAMES_PER_BUFFER,    // Number of samples (frames) to be passed oppon calling CB
        paNoFlag,             // Control Flags
        callback,             // Callback function pointer
        user_data             // User data
    ),
    "Cannot opean stream for record");

    return 0;

}

static fft_input_t get_max_freq(const fft_output_t data[], const size_t length, const double fs)
{
    static const fft_input_t THRESHOLD = 100.0f;
    fft_input_t max_val;
    size_t max_val_idx;

    if (length == 0)
    {
        return -1.0;
    }

    max_val = cabsf(data[0]);
    max_val_idx = 0;

    for(size_t i = 1; i < length / 2; i++)
    {
        fft_input_t cur_abs_val = cabsf(data[i]);

        if (cur_abs_val > max_val)
        {
            max_val = cur_abs_val;
            max_val_idx = i;
        }
    }

    if (max_val < THRESHOLD)
    {
        return 0.0f;
    }

    // Tranform from index to frequency
    return (fft_input_t)max_val_idx / (length / 2) * (fs / 2);
}

int main(int argc, char * argv[])
{
    PaError pa_err_code;
    PaStream * stream;
    fft_output_t * fft_data = NULL;
    size_t fft_length = BUFFER_SIZE;
    double fs = 8E3;
    int buf_idx = 0;
    size_t temp_cnt = 0;
    (void)argc;
    (void)argv;

    enable_raw_terminal();

    fft_data = create_fft_data(&fft_length);
    assert(fft_data != NULL);

    assert(pthread_spin_init(&new_data_lock, PTHREAD_PROCESS_PRIVATE) == 0);
    if (pa_init(&stream, fs, audio_in_cb, &buf_idx))
    {
        return EXIT_FAILURE;
    }

    Pa_StartStream(stream);

    // Hide cursor
    printf("\x1b[?25l");
    while ((pa_err_code = Pa_IsStreamActive(stream)) == 1 &&
            !check_if_q() )
    {
        fft_input_t max_freq;

        fft(record_data[buf_idx], BUFFER_SIZE, fft_data, fft_length);
        max_freq = get_max_freq(fft_data, fft_length, fs);
        printf("Max freq = %5.0f, %zu\r", roundf(max_freq), temp_cnt);
        fflush(stdout);
        temp_cnt = (temp_cnt + 1) % 1000;
        pthread_spin_lock(&new_data_lock);
    }

    // show cursor
    printf("\x1b[?25h\n");

    free(fft_data);
    pthread_spin_destroy(&new_data_lock);
    PA_ASSERT(Pa_CloseStream(stream), "Cannot close PortAudio stream.");
    PA_ASSERT(Pa_Terminate(), "Error closing Portable Audio library.");

    return EXIT_SUCCESS;
}

