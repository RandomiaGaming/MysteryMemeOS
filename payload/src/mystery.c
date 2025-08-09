#include "mystery.h"
#include "mysteryvideo.h"
//#include "mysteryaudio.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static void ignore_sig(int sig)
{
    (void)sig;
    return;
}
static void exit_sig(int sig)
{
    (void)sig;
    exit_requested = true;
}

volatile bool exit_requested = false;
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

    video_init();
    //audio_init();
    while (!exit_requested)
    {
        video_update();
        //audio_update();
    }
    
    printf("Mystery info - Quitting gracefully\n");
    fflush(stdout);

    video_cleanup();
    //audio_cleanup();

    return 0;
}