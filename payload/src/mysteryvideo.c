// Approved 05/01/2025
// No extra includes
// TODO support hot plugging devices eventually.

#include "mysteryvideo.h"
#include "assets/mysteryimage.h"
#include "mystery.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <glob.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// Structs
struct card_context
{
    int fd;
    drmModeRes *resources;
    drmModeCrtc *crtc;
};
struct monitor_context
{
    struct card_context *card;
    drmModeConnector *connector;
    drmModeModeInfo *mode;
    uint32_t width;
    uint32_t height;
    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;
    uint32_t *framebuffer;
    uint32_t framebufferFd;
};

// Render the mystery image to a monitor
uint32_t get_pixel(uint32_t x, uint32_t y)
{
    return mysteryimage_buffer[(mysteryimage_width * y) + x];
}
uint32_t offset = 0;
void render(struct monitor_context *monitor)
{
    for (uint32_t y = 0; y < monitor->height; y++)
    {
        for (uint32_t x = 0; x < monitor->width; x++)
        {
            uint32_t scaledX = (x * mysteryimage_width) / monitor->width;
            uint32_t scaledY = (y * mysteryimage_height) / monitor->height;
            uint32_t mysteryPixel = get_pixel(scaledX, scaledY);

            monitor->framebuffer[(y * monitor->width) + x] = mysteryPixel;
        }
    }
    offset = (offset + 50) % monitor->width;
}

// Enumerating graphics cards
struct card_context **enum_cards()
{
    // Use glob to enum /dev/dri/card*
    glob_t cardsGlob = {};
    if (glob("/dev/dri/card*", 0, NULL, &cardsGlob) != 0)
    {
        printf("VIDEO - ERROR: Failed to enumerate /dev/dri/card*.\n");
        fflush(stdout);
        globfree(&cardsGlob);
        return NULL;
    }

    // Alloc enough space for the max number of cards
    int cardCount = 0;
    struct card_context **cards = (struct card_context **)malloc(sizeof(void *) * (cardsGlob.gl_pathc + 1));

    // Build the list of cards
    for (size_t i = 0; i < cardsGlob.gl_pathc; i++)
    {
        // Create a new card
        struct card_context *card = (struct card_context *)malloc(sizeof(struct card_context));
        memset(card, 0, sizeof(struct card_context));

        // Open card file descriptor
        card->fd = open(cardsGlob.gl_pathv[i], O_RDWR | O_CLOEXEC);
        if (card->fd < 0)
        {
            printf("VIDEO - ERROR: Failed to open %s.\n", cardsGlob.gl_pathv[i]);
            fflush(stdout);
            free(card);
            continue;
        }

        // Open card resources
        card->resources = drmModeGetResources(card->fd);
        if (card->resources == NULL)
        {
            printf("VIDEO - ERROR: drmModeGetResources failed.\n");
            fflush(stdout);
            close(card->fd);
            free(card);
            continue;
        }

        // Select random crtc for card (there is likely only one anyways)
        card->crtc = drmModeGetCrtc(card->fd, card->resources->crtcs[0]);
        if (card->crtc == NULL)
        {
            printf("VIDEO - ERROR: drmModeGetCrtc failed.\n");
            fflush(stdout);
            drmModeFreeResources(card->resources);
            close(card->fd);
            free(card);
            continue;
        }

        printf("Bound to video on %s.\n", cardsGlob.gl_pathv[i]);
        fflush(stdout);

        // Add the new card to the list of cards
        cards[cardCount] = card;
        cardCount++;
    }
    cards[cardCount] = NULL;

    // Cleanup and return
    globfree(&cardsGlob);
    return cards;
}
void free_cards(struct card_context **cards)
{
    for (int i = 0; cards[i] != NULL; i++)
    {
        struct card_context *card = cards[i];
        drmModeFreeCrtc(card->crtc);
        drmModeFreeResources(card->resources);
        close(card->fd);
        free(card);
    }
    free(cards);
}

// Enumerating connectors aka monitors
struct monitor_context **enum_monitors(struct card_context **cards)
{
    // Count the max number of monitors
    int monitorCountMax = 0;
    for (int i = 0; cards[i] != NULL; i++)
    {
        monitorCountMax += cards[i]->resources->count_connectors;
    }

    // Alloc space for the max number of monitors
    int monitorCount = 0;
    struct monitor_context **monitors = (struct monitor_context **)malloc(sizeof(void *) * (monitorCountMax + 1));

    // Build the list of monitors
    for (int i = 0; cards[i] != NULL; i++)
    {
        for (int j = 0; j < cards[i]->resources->count_connectors; j++)
        {
            // Create new monitor
            struct monitor_context *monitor = (struct monitor_context *)malloc(sizeof(struct monitor_context));
            memset(monitor, 0, sizeof(struct monitor_context));

            // Set monitor card
            monitor->card = cards[i];

            // Get connector for this monitor
            monitor->connector = drmModeGetConnector(monitor->card->fd, monitor->card->resources->connectors[j]);
            if (monitor->connector == NULL)
            {
                printf("VIDEO - ERROR: drmModeGetConnector failed.\n");
                fflush(stdout);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            // If connector is disconnected then bail
            if (monitor->connector->connection == DRM_MODE_DISCONNECTED)
            {
                // No error just disconnected and that's okay
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            // Select best availible mode
            monitor->mode = NULL;
            for (int k = 0; k < monitor->connector->count_modes; k++)
            {
                if (monitor->mode == NULL)
                {
                    monitor->mode = &monitor->connector->modes[k];
                    continue;
                }
                if (monitor->connector->modes[k].vdisplay * monitor->connector->modes[k].hdisplay > monitor->mode->vdisplay * monitor->mode->hdisplay)
                {
                    monitor->mode = &monitor->connector->modes[k];
                    continue;
                }
                if (monitor->connector->modes[k].vdisplay * monitor->connector->modes[k].hdisplay == monitor->mode->vdisplay * monitor->mode->hdisplay && monitor->connector->modes[k].vrefresh > monitor->mode->vrefresh)
                {
                    monitor->mode = &monitor->connector->modes[k];
                    continue;
                }
            }

            // Error if there are no display modes
            if (monitor->mode == NULL)
            {
                printf("VIDEO - ERROR: Monitor has no modes and is a pile of junk.\n");
                fflush(stdout);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            // Set width and height from mode
            monitor->width = monitor->mode->hdisplay;
            monitor->height = monitor->mode->vdisplay;

            // Create dumb create and dumb map
            monitor->create.width = monitor->width;
            monitor->create.height = monitor->height;
            monitor->create.bpp = 32;
            if (drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_CREATE_DUMB, &monitor->create) < 0)
            {
                printf("VIDEO - ERROR: DRM_IOCTL_MODE_CREATE_DUMB failed.\n");
                fflush(stdout);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            monitor->map.handle = monitor->create.handle;
            if (drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_MAP_DUMB, &monitor->map) < 0)
            {
                printf("VIDEO - ERROR: DRM_IOCTL_MODE_MAP_DUMB failed.\n");
                fflush(stdout);
                struct drm_mode_destroy_dumb destroy = {};
                destroy.handle = monitor->create.handle;
                drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            monitor->framebuffer = (uint32_t *)mmap(0, monitor->create.size, PROT_READ | PROT_WRITE, MAP_SHARED, monitor->card->fd, monitor->map.offset);
            if (monitor->framebuffer == MAP_FAILED)
            {
                printf("VIDEO - ERROR: mmap failed.\n");
                fflush(stdout);
                struct drm_mode_destroy_dumb destroy = {};
                destroy.handle = monitor->create.handle;
                drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            if (drmModeAddFB(monitor->card->fd, monitor->create.width, monitor->create.height, 24, 32, monitor->create.pitch, monitor->create.handle, &monitor->framebufferFd))
            {
                printf("VIDEO - ERROR: drmModeAddFB failed.\n");
                fflush(stdout);
                munmap(monitor->framebuffer, monitor->create.size);
                struct drm_mode_destroy_dumb destroy = {};
                destroy.handle = monitor->create.handle;
                drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
                drmModeFreeConnector(monitor->connector);
                free(monitor);
                continue;
            }

            memset((void *)monitor->framebuffer, 0, monitor->width * monitor->height * sizeof(uint32_t));

            // Add the new monitor to the list of monitors
            monitors[monitorCount] = monitor;
            monitorCount++;
        }
    }
    monitors[monitorCount] = NULL;

    // Cleanup and return
    return monitors;
}
void free_monitors(struct monitor_context **monitors)
{
    for (int i = 0; monitors[i] != NULL; i++)
    {
        struct monitor_context *monitor = monitors[i];
        drmModeRmFB(monitor->card->fd, monitor->framebufferFd);
        munmap(monitor->framebuffer, monitor->create.size);
        struct drm_mode_destroy_dumb destroy = {};
        destroy.handle = monitor->create.handle;
        drmIoctl(monitor->card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        drmModeFreeConnector(monitor->connector);
        free(monitor);
    }
    free(monitors);
}

// Basic control flow
struct card_context **cards = NULL;
struct monitor_context **monitors = NULL;
void video_init()
{
    cards = enum_cards();
    monitors = enum_monitors(cards);
}
void video_update()
{
    for (int i = 0; monitors[i] != NULL; i++)
    {
        struct monitor_context *monitor = monitors[i];

        render(monitor);

        uint32_t connector_id = monitor->connector->connector_id;
        if (drmModeSetCrtc(monitor->card->fd, monitor->card->crtc->crtc_id, monitor->framebufferFd, 0, 0, &connector_id, 1, monitor->mode))
        {
            printf("VIDEO - ERROR: drmModeSetCrtc failed.\n");
            fflush(stdout);
            continue;
        }
    }
    return;
}
void video_cleanup()
{
    free_monitors(monitors);
    monitors = NULL;
    free_cards(cards);
    cards = NULL;
}