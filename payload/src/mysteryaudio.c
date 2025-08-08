#include "../build/assets_h/mysterysong_wav.h"
#include "mysteryaudio.h"
#include "mystery.h"
#include <alsa/asoundlib.h>



// Structs
struct device_context
{
    int cardId;
    int deviceId;
    snd_pcm_t *handle;
    uint32_t buffer[4096];
    uint32_t position;
};

// Basic control flow
struct device_context **devices = NULL;
void audio_init()
{
    // Count the max number of devices
    int deviceCountMax = 0;
    int cardId = -1;
    while (snd_card_next(&cardId) == 0 && cardId >= 0)
    {
        char *cardName = sformat("hw:%d", cardId);
        snd_ctl_t *ctl = NULL;
        if (snd_ctl_open(&ctl, cardName, 0) != 0)
        {
            free(cardName);
            continue;
        }
        free(cardName);

        int deviceId = -1;
        while (snd_ctl_pcm_next_device(ctl, &deviceId) == 0 && deviceId >= 0)
        {
            deviceCountMax++;
        }
    }

    // Alloc space for the max number of devices
    int deviceCount = 0;
    devices = (struct device_context **)malloc(sizeof(void *) * (deviceCountMax + 1));

    cardId = -1;
    while (snd_card_next(&cardId) == 0 && cardId >= 0)
    {
        char *cardName = sformat("hw:%d", cardId);
        printf("Found card %s.\n", cardName);
        fflush(stdout);
        snd_ctl_t *ctl = NULL;
        if (snd_ctl_open(&ctl, cardName, 0) != 0)
        {
            printf("AUDIO - ERROR: snd_ctl_open failed.\n");
            fflush(stdout);
            free(cardName);
            continue;
        }
        free(cardName);

        int deviceId = -1;
        while (snd_ctl_pcm_next_device(ctl, &deviceId) == 0 && deviceId >= 0)
        {
            struct device_context *device = (struct device_context *)malloc(sizeof(struct device_context));
            memset((void *)device, 0, sizeof(struct device_context));

            device->cardId = cardId;
            device->deviceId = deviceId;

            char *deviceName = sformat("plughw:%d,%d", device->cardId, device->deviceId);
            printf("Found device %s.\n", deviceName);
            fflush(stdout);
            if (snd_pcm_open(&device->handle, deviceName, SND_PCM_STREAM_PLAYBACK, 0) != 0)
            {
                printf("AUDIO - ERROR: snd_pcm_open failed.\n");
                fflush(stdout);
                free(deviceName);
                free(device);
                continue;
            }
            free(deviceName);

            // Set device params (no need for free this is on stack)
            snd_pcm_hw_params_t *params;
            snd_pcm_hw_params_alloca(&params);
            snd_pcm_hw_params_any(device->handle, params);

            for (int fmt = 0; fmt <= SND_PCM_FORMAT_LAST; ++fmt)
            {
                if (snd_pcm_hw_params_test_format(device->handle, params, (snd_pcm_format_t)fmt) == 0)
                {
                    printf("Format supported: %s\n", snd_pcm_format_name((snd_pcm_format_t)fmt));
                    fflush(stdout);
                }
            }
            unsigned int rates[] = {8000, 16000, 22050, 32000, 44100, 48000, 96000, 192000};
            for (int i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i)
            {
                if (snd_pcm_hw_params_test_rate(device->handle, params, rates[i], 0) == 0)
                {
                    printf("Rate supported: %u Hz\n", rates[i]);
                    fflush(stdout);
                }
            }
            for (unsigned int ch = 1; ch <= 8; ++ch)
            {
                if (snd_pcm_hw_params_test_channels(device->handle, params, ch) == 0)
                {
                    printf("Channels supported: %u\n", ch);
                    fflush(stdout);
                }
            }

            snd_pcm_hw_params_set_access(device->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
            snd_pcm_hw_params_set_format(device->handle, params, SND_PCM_FORMAT_S32_LE);
            snd_pcm_hw_params_set_channels(device->handle, params, mysterysong_channels);
            int dir = 0;
            unsigned int samplerate = mysterysong_samplerate;
            snd_pcm_hw_params_set_rate_near(device->handle, params, &samplerate, &dir);
            if (snd_pcm_hw_params(device->handle, params) != 0)
            {
                printf("AUDIO - ERROR: snd_pcm_hw_params failed.\n");
                fflush(stdout);
                snd_pcm_close(device->handle);
                free(device);
                continue;
            }

            // Prepare PCM device
            if (snd_pcm_prepare(device->handle) != 0)
            {
                printf("AUDIO - ERROR: snd_pcm_prepare failed.\n");
                fflush(stdout);
                snd_pcm_close(device->handle);
                free(device);
                continue;
            }

            // Add new device to list
            devices[deviceCount] = device;
            deviceCount++;
        }

        snd_ctl_close(ctl);
    }

    devices[deviceCount] = NULL;
}
void audio_update()
{
    for (int i = 0; devices[i] != NULL; i++)
    {
        struct device_context *device = devices[i];

        // Populate buffer with up to date data
        for (int i = 0; i < 4096; i++)
        {
            device->buffer[i] = mysterysong_buffer[(device->position + i) % mysterysong_length];
        }

        // Write data from buffer to device
        int framesWritten = snd_pcm_writei(device->handle, device->buffer, 4096);
        if (framesWritten < 0)
        {
            printf("AUDIO - ERROR: snd_pcm_writei failed trying to recover...\n");
            fflush(stdout);
            if (snd_pcm_recover(device->handle, framesWritten, 0) != 0)
            {
                printf("AUDIO - ERROR: snd_pcm_recover failed.\n");
                fflush(stdout);
            }
        }
        else
        {
            device->position = (device->position + framesWritten) % mysterysong_length;
        }
    }
}
void audio_cleanup()
{
    for (int i = 0; devices[i] != NULL; i++)
    {
        struct device_context *device = devices[i];

        snd_pcm_close(device->handle);

        free(device);
        device = NULL;
    }
    free(devices);
    devices = NULL;
}