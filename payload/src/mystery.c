#include "mystery.h"
#include "mysteryvideo.h"
#include "mysteryaudio.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static void ignore_sig(int sig)
{
    (void)sig;
    return;
}
static void exit_sig(int sig)
{
    (void)sig;
    exit_requested = true;
    exit(7);
}

bool exit_requested = false;
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    for (int i = 0; i < _NSIG; i++)
    {
        if (i == SIGINT)
        {
            signal(i, exit_sig);
        }
        else
        {
            signal(i, ignore_sig);
        }
    }

    audio_init();
    video_init();
    while (!exit_requested)
    {
        audio_update();
        video_update();
    }
    audio_cleanup();
    video_cleanup();

    return 0;
}