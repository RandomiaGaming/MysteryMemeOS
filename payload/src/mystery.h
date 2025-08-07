#ifndef MYSTERY_HELPER_H
#define MYSTERY_HELPER_H

#include <stdbool.h>

extern bool exit_requested;
void ignore_sig(int sig);
void exit_sig(int sig);
int main(int argc, char **argv);

#endif // MYSTERY_HELPER_H