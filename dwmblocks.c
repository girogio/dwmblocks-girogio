#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifndef NO_X
#include <X11/Xlib.h>
#endif

#ifdef __OpenBSD__
#define SIGPLUS			SIGUSR1+1
#define SIGMINUS		SIGUSR1-1
#else
#define SIGPLUS			SIGRTMIN
#define SIGMINUS		SIGRTMIN
#endif

#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		50
#define MIN( a, b ) ( ( a < b) ? a : b )
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
    char* icon;
    char* command;
    unsigned int interval;
    unsigned int signal;
} Block;

#ifndef __OpenBSD__
void dummysighandler(int num);
#endif

unsigned int gcd(int, int);
void getcmds(int);
void getsigcmds(unsigned int);
void setupsignals();
void sighandler(int);
int getstatus(char *, char *);
void statusloop();
void termhandler();
void pstdout();

#ifndef NO_X
void setroot();
static void (*writestatus) () = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*writestatus) () = pstdout;
#endif

#include "blocks.h"

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][STATUSLENGTH];
static int statusContinue = 1;
static int returnStatus = 0;

unsigned int gcd(int a, int b)
{
    int temp;
    while (b > 0) {
        temp = a % b;

        a = b;
        b = temp;
    }
    return a;
}

/* Opens process *cmd and stores output in *output */
void getcmd(const Block *block, char *output)
{
    strcpy(output, block->icon);
    FILE *cmdf = popen(block->command, "r");
    if (!cmdf)
        return;
    int i = strlen(block->icon);
    fgets(output+i, CMDLENGTH-i-delimLen, cmdf);
    i = strlen(output);
    if (i == 0) {
        // Return if block and command output are both empty
        pclose(cmdf);
        return;
    }
    if (delim[0] != '\0') {
        // Only chop off newline if one is present at the end
        i = output[i-1] == '\n' ? i-1 : i;
        strncpy(output+i, delim, delimLen);
    }
    else
        output[i++] = '\0';
    pclose(cmdf);
}

void getcmds(int time)
{
    const Block* current;
    for (unsigned int i = 0; i < LENGTH(blocks); i++) {
        current = blocks + i;
        if ((current->interval != 0 && time % current->interval == 0) || time == -1)
            getcmd(current,statusbar[i]);
    }
}

void getsigcmds(unsigned int signal)
{
    const Block *current;
    for (unsigned int i = 0; i < LENGTH(blocks); i++) {
        current = blocks + i;
        if (current->signal == signal)
            getcmd(current,statusbar[i]);
    }
}

void setupsignals()
{
#ifndef __OpenBSD__
    /* Initialize all real time signals with dummy handler */
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        signal(i, dummysighandler);
#endif

    for (unsigned int i = 0; i < LENGTH(blocks); i++) {
        if (blocks[i].signal > 0)
            signal(SIGMINUS+blocks[i].signal, sighandler);
    }

}

int getstatus(char *str, char *last)
{
    strcpy(last, str);
    str[0] = '\0';
    for (unsigned int i = 0; i < LENGTH(blocks); i++)
        strcat(str, statusbar[i]);
    str[strlen(str)-strlen(delim)] = '\0';
    return strcmp(str, last); // 0 if they are the same
}

#ifndef NO_X
void setroot()
{
    if (!getstatus(statusstr[0], statusstr[1])) // Only set root if text has changed.
        return;
    XStoreName(dpy, root, statusstr[0]);
    XFlush(dpy);
}

int setupX()
{
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "dwmblocks: Failed to open display\n");
        return 0;
    }
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    return 1;
}
#endif

void pstdout()
{
    if (!getstatus(statusstr[0], statusstr[1])) // Only write out if text has changed.
        return;
    printf("%s\n",statusstr[0]);
    fflush(stdout);
}


void statusloop()
{
    unsigned int interval = -1;
    for (int i = 0; i < LENGTH(blocks); i++) {
        if (blocks[i].interval)
            interval = gcd(blocks[i].interval, interval);
    }
	unsigned int tosleep = interval;
    setupsignals();
    int i = 0;
    getcmds(-1);
	writestatus();
    while (statusContinue) {
		if ((tosleep = sleep(tosleep)))
			continue;

		tosleep = interval;
		getcmds(i);
        writestatus();
		i += interval;
    }

    // first figure out the default wait interval by finding the
    // greatest common denominator of the intervals
    /*unsigned int interval = -1;*/
    /*for (int i = 0; i < LENGTH(blocks); i++) {*/
    /*if (blocks[i].interval) {*/
    /*interval = gcd(blocks[i].interval, interval);*/
    /*}*/
    /*}*/
    /*unsigned int i = 0;*/
    /*int interrupted = 0;*/
    /*const struct timespec sleeptime = {interval, 0};*/
    /*struct timespec tosleep = sleeptime;*/
    /*getcmds(-1);*/
    /*while (1)*/
    /*{*/
    /*// sleep for tosleep (should be a sleeptime of interval seconds) and put what was left if interrupted back into tosleep*/
    /*interrupted = nanosleep(&tosleep, &tosleep);*/
    /*// if interrupted then just go sleep again for the remaining time*/
    /*if (interrupted == -1) {*/
    /*continue;*/
    /*}*/
    /*// if not interrupted then do the calling and writing*/
    /*getcmds(i);*/
    /*writestatus();*/
    /*// then increment since its actually been a second (plus the time it took the commands to run)*/
    /*i += interval;*/
    /*// set the time to sleep back to the sleeptime of 1s*/
    /*tosleep = sleeptime;*/
    /*}*/
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

void sighandler(int signum)
{
    getsigcmds(signum-SIGPLUS);
    writestatus();
}

void termhandler()
{
    statusContinue = 0;
}

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) { // Handle command line arguments
        if (!strcmp("-d",argv[i]))
            strncpy(delim, argv[++i], delimLen);
        else if (!strcmp("-p",argv[i]))
            writestatus = pstdout;
    }
#ifndef NO_X
    if (!setupX())
        return 1;
#endif
    delimLen = MIN(delimLen, strlen(delim));
    delim[delimLen++] = '\0';
    signal(SIGTERM, termhandler);
    signal(SIGINT, termhandler);
    statusloop();
#ifndef NO_X
    XCloseDisplay(dpy);
#endif
    return 0;
}
