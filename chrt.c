#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <getopt.h>
#include <locale.h>
#include <libintl.h>
#include <errno.h>
#include <sys/file.h>
#include <stdio.h>

/* control struct */
struct chrt_ctl {
    pid_t pid;
    int policy;

    uint64_t runtime;
    uint64_t deadline;
    uint64_t period;

    unsigned int all_tasks: 1,
    reset_on_fork: 1,
    altered: 1,
    verbose: 1;
};

#ifndef CLOSE_EXIT_CODE
#define CLOSE_EXIT_CODE EXIT_FAILURE
#endif

static inline void close_stdout(void) {
    if (close_stream(stdout) != 0 && !(errno == EPIPE)) {
        if (errno)
            warn(_("write error"));
        else
            warnx(_("write error"));

        _exit(CLOSE_EXIT_CODE);
    }

    if (close_stream(stderr) != 0)
        _exit(CLOSE_EXIT_CODE);
}
int main(int argc, char **argv) {
    struct chrt_ctl _ctl = {
        .pid = -1, .policy = SCHED_RR }, *ctl = &_ctl;
    int c;

    static const struct option longpts[] = {
        {"all-tasks", no_argument, NULL, 'a'},
        {"batch", no_argument, NULL, 'b'},
        {"deadline", no_argument, NULL, 'd'},
        {"fifo", no_argument, NULL, 'f'},
        {"idle", no_argument, NULL, 'i'},
        {"pid", no_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {"max", no_argument, NULL, 'm'},
        {"other", no_argument, NULL, 'o'},
        {"rr", no_argument, NULL, 'r'},
        {"sched-runtime", required_argument, NULL, 'T'},
        {"sched-period", required_argument, NULL, 'P'},
        {"sched-deadline", required_argument, NULL, 'D'},
        {"reset-on-fork", no_argument, NULL, 'R'},
        {"verbose", no_argument, NULL, 'v'},
        {"version", no_argument, NULL, 'V'},
        {NULL, no_argument, NULL, 0}
    };

    setlocale(LC_ALL, "");
    // bindtextdomain(PACKAGE, LOCALEDIR);
    bindtextdomain("hello", getenv("PWD"));
    // textdomain(PACKAGE)
    textdomain("hello");
    atexit(close_stdout);
}