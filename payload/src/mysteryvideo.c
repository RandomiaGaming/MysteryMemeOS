#include "mysteryvideo.h"
#include "../build/assets_h/mysteryimage_raw.h"
#include "mystery.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <glob.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <errno.h>
#include <stdio.h>

// Structs
struct binding
{
    uint32_t connector_id;
    uint32_t crct_id;
};
struct card
{
    char *path;
    int fd;
    drmModeRes *resources;
    int crtc_count;
    drmModeCrtc **crtcs;
    int encoder_count;
    drmModeEncoder **encoders;
    int connector_count;
    drmModeConnector **connectors;
    int binding_count;
    struct binding *bindings;
};
struct renderer
{
    struct card *card;
    uint32_t connector_id;
    uint32_t crtc_id;

    drmModeModeInfo mode;
    uint32_t width;
    uint32_t height;

    bool showing_b;

    uint32_t framebuffer_a_handle;
    uint64_t framebuffer_a_size;
    uint32_t *framebuffer_a;
    uint32_t framebuffer_a_id;

    uint32_t framebuffer_b_handle;
    uint64_t framebuffer_b_size;
    uint32_t *framebuffer_b;
    uint32_t framebuffer_b_id;
};

// Global vars
static int card_count = 0;
static struct card *cards[32] = {};
static int renderer_count = 0;
static struct renderer *renderers[256] = {};

// Static function signatures
static void init_cards();
static void cleanup_renderer(struct renderer *renderer);
static void remove_renderer(int index);
static void update_renderers();
static uint32_t get_width();
static uint32_t get_height();
static uint32_t get_pixel(uint32_t x, uint32_t y);
static void render(struct renderer *renderer);

// Working with cards
static void init_cards()
{
    glob_t cards_glob = {};
    if (glob("/dev/dri/card*", 0, NULL, &cards_glob) != 0)
    {
        printf("Video error - Glob failed to enumerate /dev/dri/card*\n");
        fflush(stdout);
        goto cleanupGlob;
    }

    for (size_t i = 0; i < cards_glob.gl_pathc; i++)
    {
        struct card *card = NULL;

        for (int j = 0; j < card_count; j++)
        {
            if (strcmp(cards[j]->path, cards_glob.gl_pathv[i]) == 0)
            {
                goto cleanup;
            }
        }

        card = (struct card *)calloc(1, sizeof(struct card));

        card->path = calloc((strlen(cards_glob.gl_pathv[i]) + 1), sizeof(char));
        strcpy(card->path, cards_glob.gl_pathv[i]);

        card->fd = open(card->path, O_RDWR | O_CLOEXEC);
        if (card->fd < 0)
        {
            printf("Video error - open failed - %s\n", strerror(errno));
            fflush(stdout);
            goto cleanup;
        }

        if (drmSetMaster(card->fd) != 0)
        {
            printf("Video error - drmSetMaster failed - %s\n", strerror(errno));
            fflush(stdout);
            goto cleanup;
        }

        card->resources = drmModeGetResources(card->fd);
        if (card->resources == NULL)
        {
            printf("Video error - drmModeGetResources failed - %s\n", strerror(errno));
            fflush(stdout);
            goto cleanup;
        }

        card->crtc_count = card->resources->count_crtcs;
        card->crtcs = (drmModeCrtc **)calloc(card->crtc_count, sizeof(drmModeCrtc *));
        for (int i = 0; i < card->crtc_count; i++)
        {
            card->crtcs[i] = drmModeGetCrtc(card->fd, card->resources->crtcs[i]);
            if (card->crtcs[i] == NULL)
            {
                printf("Video error - drmModeGetConnector failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
        }

        card->encoder_count = card->resources->count_encoders;
        card->encoders = (drmModeEncoder **)calloc(card->encoder_count, sizeof(drmModeEncoder *));
        for (int i = 0; i < card->encoder_count; i++)
        {
            card->encoders[i] = drmModeGetEncoder(card->fd, card->resources->encoders[i]);
            if (card->encoders[i] == NULL)
            {
                printf("Video error - drmModeGetEncoder failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
        }

        card->connector_count = card->resources->count_connectors;
        card->connectors = (drmModeConnector **)calloc(card->connector_count, sizeof(drmModeConnector *));
        for (int i = 0; i < card->connector_count; i++)
        {
            card->connectors[i] = drmModeGetConnector(card->fd, card->resources->connectors[i]);
            if (card->connectors[i] == NULL)
            {
                printf("Video error - drmModeGetConnector failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
        }

        // TODO implament a better algorithm
        card->binding_count = card->connector_count;
        card->bindings = (struct binding *)calloc(card->connector_count, sizeof(struct binding));
        for (int i = 0; i < card->connector_count; i++)
        {
            card->bindings[i].connector_id = card->connectors[i]->connector_id;
            card->bindings[i].crct_id = card->crtcs[i]->crtc_id;
        }

        printf("Video info - Bound to card %s\n", card->path);
        fflush(stdout);

        cards[card_count] = card;
        card = NULL;
        card_count++;

    cleanup:
        if (card != NULL)
        {
            if (card->bindings != NULL)
            {
                free(card->bindings);
            }
            if (card->connectors != NULL)
            {
                for (int j = 0; j < card->connector_count; j++)
                {
                    drmModeFreeConnector(card->connectors[j]);
                }
                free(card->connectors);
            }
            if (card->encoders != NULL)
            {
                for (int j = 0; j < card->encoder_count; j++)
                {
                    drmModeFreeEncoder(card->encoders[j]);
                }
                free(card->encoders);
            }
            if (card->crtcs != NULL)
            {
                for (int j = 0; j < card->crtc_count; j++)
                {
                    drmModeFreeCrtc(card->crtcs[j]);
                }
                free(card->crtcs);
            }
            if (card->resources != NULL)
            {
                drmModeFreeResources(card->resources);
            }
            if (card->fd != 0)
            {
                close(card->fd);
            }
            if (card->path != NULL)
            {
                free(card->path);
            }
            free(card);
        }
    }

cleanupGlob:
    globfree(&cards_glob);
}

// Working with renderers
static void cleanup_renderer(struct renderer *renderer)
{
    if (renderer->framebuffer_b_id != 0)
    {
        drmModeRmFB(renderer->card->fd, renderer->framebuffer_b_id);
    }
    if (renderer->framebuffer_b != NULL)
    {
        munmap(renderer->framebuffer_b, renderer->framebuffer_b_size);
    }
    if (renderer->framebuffer_b_handle != 0)
    {
        struct drm_mode_destroy_dumb destroy_dumb_b = {};
        destroy_dumb_b.handle = renderer->framebuffer_b_handle;
        drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb_b);
    }
    if (renderer->framebuffer_a_id != 0)
    {
        drmModeRmFB(renderer->card->fd, renderer->framebuffer_a_id);
    }
    if (renderer->framebuffer_a != NULL)
    {
        munmap(renderer->framebuffer_a, renderer->framebuffer_a_size);
    }
    if (renderer->framebuffer_a_handle != 0)
    {
        struct drm_mode_destroy_dumb destroy_dumb_a = {};
        destroy_dumb_a.handle = renderer->framebuffer_a_handle;
        drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb_a);
    }
    free(renderer);
}
static void remove_renderer(int index)
{
    cleanup_renderer(renderers[index]);
    for (int i = index; i < renderer_count - 1; i++)
    {
        renderers[i] = renderers[i + 1];
    }
    renderer_count--;
}
// TODO better hotplug support and handing different monitors on the same connector.
static void update_renderers()
{
    for (int i = 0; i < card_count; i++)
    {
        for (int j = 0; j < cards[i]->binding_count; j++)
        {
            struct renderer *renderer = NULL;
            drmModeConnector *connector = NULL;

            for (int k = 0; k < renderer_count; k++)
            {
                if (strcmp(renderers[k]->card->path, cards[i]->path) == 0 && renderers[k]->connector_id == cards[i]->bindings[j].connector_id)
                {
                    goto cleanup;
                }
            }

            renderer = (struct renderer *)calloc(1, sizeof(struct renderer));
            renderer->card = cards[i];
            renderer->connector_id = cards[i]->bindings[j].connector_id;
            renderer->crtc_id = cards[i]->bindings[j].crct_id;

            connector = drmModeGetConnector(renderer->card->fd, renderer->connector_id);
            if (connector == NULL)
            {
                printf("Video error - drmModeGetConnector failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            if (connector->connection != DRM_MODE_CONNECTED)
            {
                goto cleanup;
            }

            if (connector->count_modes <= 0)
            {
                printf("Video error - drmModeConnector has no modes\n");
                fflush(stdout);
                goto cleanup;
            }
            renderer->mode = connector->modes[0];
            for (int j = 1; j < connector->count_modes; j++)
            {
                if (connector->modes[j].vdisplay * connector->modes[j].hdisplay > renderer->mode.vdisplay * renderer->mode.hdisplay)
                {
                    renderer->mode = connector->modes[j];
                }
                else if (connector->modes[j].vdisplay * connector->modes[j].hdisplay == renderer->mode.vdisplay * renderer->mode.hdisplay && connector->modes[j].vrefresh > renderer->mode.vrefresh)
                {
                    renderer->mode = connector->modes[j];
                }
            }
            renderer->width = renderer->mode.hdisplay;
            renderer->height = renderer->mode.vdisplay;

            struct drm_mode_create_dumb create_dumb_a = {};
            create_dumb_a.width = renderer->width;
            create_dumb_a.height = renderer->height;
            create_dumb_a.bpp = 32;
            create_dumb_a.flags = 0;
            if (drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb_a) != 0)
            {
                printf("Video error - DRM_IOCTL_MODE_CREATE_DUMB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            renderer->framebuffer_a_handle = create_dumb_a.handle;
            renderer->framebuffer_a_size = create_dumb_a.size;

            struct drm_mode_create_dumb create_dumb_b = {};
            create_dumb_b.width = renderer->width;
            create_dumb_b.height = renderer->height;
            create_dumb_b.bpp = 32;
            create_dumb_b.flags = 0;
            if (drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb_b) != 0)
            {
                printf("Video error - DRM_IOCTL_MODE_CREATE_DUMB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            renderer->framebuffer_b_handle = create_dumb_b.handle;
            renderer->framebuffer_b_size = create_dumb_b.size;

            struct drm_mode_map_dumb map_dumb_a = {};
            map_dumb_a.handle = renderer->framebuffer_a_handle;
            if (drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb_a) != 0)
            {
                printf("Video error - DRM_IOCTL_MODE_MAP_DUMB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }

            struct drm_mode_map_dumb map_dumb_b = {};
            map_dumb_b.handle = renderer->framebuffer_b_handle;
            if (drmIoctl(renderer->card->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb_b) != 0)
            {
                printf("Video error - DRM_IOCTL_MODE_MAP_DUMB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }

            renderer->framebuffer_a = (uint32_t *)mmap(0, renderer->framebuffer_a_size, PROT_READ | PROT_WRITE, MAP_SHARED, renderer->card->fd, map_dumb_a.offset);
            if (renderer->framebuffer_a == MAP_FAILED)
            {
                printf("Video error - mmap failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            memset((void *)renderer->framebuffer_a, 0, renderer->framebuffer_a_size);

            renderer->framebuffer_b = (uint32_t *)mmap(0, renderer->framebuffer_b_size, PROT_READ | PROT_WRITE, MAP_SHARED, renderer->card->fd, map_dumb_b.offset);
            if (renderer->framebuffer_b == MAP_FAILED)
            {
                printf("Video error - mmap failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            memset((void *)renderer->framebuffer_b, 0, renderer->framebuffer_b_size);

            if (drmModeAddFB(renderer->card->fd, renderer->width, renderer->height, 24, create_dumb_a.bpp, create_dumb_a.pitch, renderer->framebuffer_a_handle, &renderer->framebuffer_a_id) != 0)
            {
                printf("Video error - drmModeAddFB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }

            if (drmModeAddFB(renderer->card->fd, renderer->width, renderer->height, 24, create_dumb_b.bpp, create_dumb_b.pitch, renderer->framebuffer_b_handle, &renderer->framebuffer_b_id) != 0)
            {
                printf("Video error - drmModeAddFB failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }

            render(renderer);

            if (drmModeSetCrtc(renderer->card->fd, renderer->crtc_id, renderer->framebuffer_b_id, 0, 0, &renderer->connector_id, 1, &renderer->mode) != 0)
            {
                printf("Video error - drmModeSetCrtc failed - %s\n", strerror(errno));
                fflush(stdout);
                goto cleanup;
            }
            renderer->showing_b = true;

            printf("Video info - Bound to monitor %s\n", renderer->mode.name);
            fflush(stdout);

            renderers[renderer_count] = renderer;
            renderer = NULL;
            renderer_count++;

        cleanup:
            if (connector != NULL)
            {
                drmModeFreeConnector(connector);
            }
            if (renderer != NULL)
            {
                cleanup_renderer(renderer);
            }
        }
    }
}

// Loading image data
static uint32_t get_width()
{
    return *(uint32_t *)mysteryimage_raw_buffer;
}
static uint32_t get_height()
{
    return *(uint32_t *)(mysteryimage_raw_buffer + 4);
}
static uint32_t get_pixel(uint32_t x, uint32_t y)
{
    uint32_t stride = (*(uint32_t *)mysteryimage_raw_buffer) * 4;
    return *(uint32_t *)(mysteryimage_raw_buffer + (y * stride) + (x * 4) + 8);
}

// Renderring and presenting
static void render(struct renderer *renderer)
{
    uint32_t *framebuffer;
    uint32_t framebuffer_id;
    if (renderer->showing_b)
    {
        framebuffer = renderer->framebuffer_a;
        framebuffer_id = renderer->framebuffer_a_id;
    }
    else
    {
        framebuffer = renderer->framebuffer_b;
        framebuffer_id = renderer->framebuffer_b_id;
    }

    for (uint32_t x = 0; x < renderer->width; x++)
    {
        for (uint32_t y = 0; y < renderer->height; y++)
        {
            uint32_t scaledX = (x * get_width()) / renderer->width;
            uint32_t scaledY = (y * get_height()) / renderer->height;
            uint32_t mysteryPixel = get_pixel(scaledX, scaledY);
            framebuffer[(y * renderer->width) + x] = mysteryPixel;
        }
    }

    if (drmModePageFlip(renderer->card->fd, renderer->crtc_id, framebuffer_id, 0, NULL) != 0)
    {
        printf("Video error - drmModePageFlip failed - %s - falling back to drmModeSetCrtc \n", strerror(errno));
        fflush(stdout);
        if (drmModeSetCrtc(renderer->card->fd, renderer->crtc_id, framebuffer_id, 0, 0, &renderer->connector_id, 1, &renderer->mode) != 0)
        {
            printf("Video error - drmModeSetCrtc failed - %s\n", strerror(errno));
            fflush(stdout);
        }
    }

    renderer->showing_b = !renderer->showing_b;
}

// Basic control flow
void video_init()
{
    init_cards();
}
void video_update()
{
    update_renderers();
    for (int i = 0; i < renderer_count; i++)
    {
        render(renderers[i]);
    }
}
void video_cleanup()
{
    if (false)
    {
        remove_renderer(0);
    }
}