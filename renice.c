#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>

/*
 * Change the priority (the nice value) of processes or groups 
 * of processes which are already running.
 * */
static const char *idtype[] = {
    [PRIO_PROCESS]  = N_("process ID"),
};

int main(int argc, char **argv) {
    int which = PRIO_PROCESS;
    int who = 0, prio, errs = 0;
    char *endptr = NULL;

    setLocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
    atexit(close_stdout);

    argc--;
    argv++;
}
