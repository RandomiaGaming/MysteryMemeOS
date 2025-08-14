#include "mysteryaudio.h"
#include "../build/assets_h/mysterysong_raw.h"
#include <alloca.h> // Required by asoundlib.h on musl
#include <alsa/asoundlib.h>
#include <stdbool.h>

static uint32_t get_sample_rate()
{
    return *(uint32_t *)(mysterysong_raw_buffer + 0);
}
static uint32_t get_channel_count()
{
    return *(uint32_t *)(mysterysong_raw_buffer + 4);
}
static uint32_t get_sample_count()
{
    return *(uint32_t *)(mysterysong_raw_buffer + 12);
}
static uint32_t *get_sample_ptr(uint32_t index)
{
    return (uint32_t *)(mysterysong_raw_buffer + 12 + (index * 4));
}

// Structs
struct device_context
{
    int card_id;
    int device_id;
    snd_pcm_t *handle;
    uint32_t position;
};

// Basic control flow
static int device_count = 0;
static struct device_context *devices[256];
void audio_init()
{
    int error_code = 0;
    int card_id = -1;
    while (true)
    {
        snd_ctl_t *ctl = NULL;
        char card_name[256];

        if ((error_code = snd_card_next(&card_id)) != 0)
        {
            printf("Audio error - snd_card_next failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        if (card_id < 0)
        {
            break;
        }

        sprintf(card_name, "hw:%d", card_id);

        printf("Found card %s.\n", card_name);
        fflush(stdout);

        if ((error_code = snd_ctl_open(&ctl, card_name, 0)) != 0)
        {
            printf("Audio error - snd_ctl_open failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }

        int device_id = -1;
        while (true)
        {
            struct device_context *device = NULL;

            if ((error_code = snd_ctl_pcm_next_device(ctl, &device_id)) != 0)
            {
                printf("Audio error - snd_ctl_pcm_next_device failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }
            if (device_id < 0)
            {
                break;
            }

            device = (struct device_context *)calloc(1, sizeof(struct device_context));
            device->card_id = card_id;
            device->device_id = device_id;

            char device_name[256];
            sprintf(device_name, "hw:%d,%d", device->card_id, device->device_id);

            printf("Found device %s.\n", device_name);
            fflush(stdout);

            if ((error_code = snd_pcm_open(&device->handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) != 0)
            {
                printf("Audio error - snd_pcm_open failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            snd_pcm_hw_params_t *params;
            snd_pcm_hw_params_alloca(&params);
            snd_pcm_hw_params_any(device->handle, params);

            if ((error_code = snd_pcm_hw_params_set_access(device->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) != 0)
            {
                printf("Audio error - snd_pcm_hw_params_set_access failed for SND_PCM_ACCESS_RW_INTERLEAVED - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            if ((error_code = snd_pcm_hw_params_set_format(device->handle, params, SND_PCM_FORMAT_S16_LE)) != 0)
            {
                printf("Audio error - snd_pcm_hw_params_set_format failed for SND_PCM_FORMAT_S32_LE - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            if ((error_code = snd_pcm_hw_params_set_channels(device->handle, params, get_channel_count())) != 0)
            {
                printf("Audio error - snd_pcm_hw_params_set_channels failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            int dir = 0;
            unsigned int samplerate = get_sample_rate();
            if ((error_code = snd_pcm_hw_params_set_rate_near(device->handle, params, &samplerate, &dir)) != 0 || samplerate != get_sample_rate())
            {
                printf("Audio error - snd_pcm_hw_params_set_rate_near failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            if ((error_code = snd_pcm_hw_params(device->handle, params)) != 0)
            {
                printf("Audio error - snd_pcm_hw_params failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            if ((error_code = snd_pcm_prepare(device->handle)) != 0)
            {
                printf("Audio error - snd_pcm_prepare failed - %s\n", snd_strerror(error_code));
                fflush(stdout);
                goto cleanup_device;
            }

            devices[device_count] = device;
            device_count++;
            device = NULL;

        cleanup_device:
        if(device != NULL) {
            if (device->handle != NULL) {
                snd_pcm_close(device->handle);
            }
            free(device);
        }
        }

    cleanup_card:
        if (ctl != NULL)
        {
            snd_ctl_close(ctl);
        }
    }
}
void audio_update()
{
    for (int i = 0; i < device_count; i++)
    {
        struct device_context *device = devices[i];

        uint32_t frames_to_write = get_sample_count() - device->position;
        if (frames_to_write > 4096)
        {
            frames_to_write = 4096;
        }

        int frames_written = snd_pcm_writei(device->handle, get_sample_ptr(device->position), frames_to_write);
        if (frames_written < 0)
        {
            printf("Audio error - snd_pcm_writei failed - %s - Falling back to snd_pcm_recover\n", strerror(errno));
            fflush(stdout);
            if (snd_pcm_recover(device->handle, frames_written, 0) != 0)
            {
                printf("Audio error - snd_pcm_recover failed - %s\n", strerror(errno));
                fflush(stdout);
            }
        }
        else
        {
            device->position = (device->position + frames_written) % get_sample_count();
        }
    }
}
void audio_cleanup()
{
}