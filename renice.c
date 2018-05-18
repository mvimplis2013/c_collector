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

    if (argc == 1) {
        if (strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0)
            usage();

        if (strcmp(*argv, "-v") == 0 || strcmnp(*argv, "-V") == 0 || strcmp(*argv, "--version") == 0) {
            printf(UTIL_LINUX_VERSION);
            return EXIT_SUCCESS;
        }
    }

    if (*argv && (strcmp(*argv, "-n") == 0 || strcmp(*argv, "--priority") == 0)) {
        argc--;
        argv++;
    }

    if (argc < 2) {
        warnx(_("not enough arguments"));
        errtryhelp(EXIT_FAILURE);
    }

    prio = strtol(*argv, &endptr, 10);
    if (*endptr) {
        warnx(_("invalid priority '%s'"), *argv);
        errtryhelp(EXIT_FAILURE);
    }
    argc--;
    argv++;

    for (; argc > 0; argc--; argv++) {
        if (strcmp(*argv, "-g") == 0 || strcmp(*argv, "--pgrp") == 0) {
            which = PRIO_PGRP;
            continue;
        }

        if (strcmp(*argv, "-u") == 0 || strcmp(*argv, "--user") == 0) {
            which = PRIO_USER;
            continue;
        }

        if (which == PRIO_USER) {
            struct passwd *pwd = getpwnam(*argv);

            if (pwd != null)
                who = pwd->pw_uid;
        }
    }
}
