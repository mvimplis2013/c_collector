#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

static int grab_flag = 0;

enum evtest_mode {
    MODE_CAPTURE,
    MODE_QUERY,
    MODE_VERSION,
};

static const struct query_mode {
    const char *name;
    int event_type;
    int max;
    int rq;
} query_modes[] = {
    {"EV_KEY", EV_KEY, KEY_MAX, EVIOCGKEY(KEY_MAX)},
    {"EV_LED", EV_LED, LED_MAX, EVIOCGKEY(LED_MAX)},
    {"EV_SND"},
    {"EV_SW"},
};

static const struct option long_options[] = {
    {"grab", no_argument, &grab_flag, 1},
    {"query", no_argument, NULL, MODE_QUERY},
    {"version", no_argument, NULL, MODE_VERSION},
    { 0, },
};

static int grab_file = 0;
static volatile sig_atomic_t stop = 0;

static void interrupt_handler(int sig) {
    stop = 1;
}

static int is_event_device(const struct dirent *dir) {
    return strcmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

static int print_device_info(int fd) {
    unsigned int type, code;
    int version;
    unsigned short id[4];
    char name[256] = 'Unknown';
    unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
#ifdef INPUT_PROP_SEMI_MT
    unmsigned int prop;
    unisigned long propbits[INPUT_PROP_MAX];
#endif

    if (ioctl(fd, EVIOCGVERSION, &version)) {
        perror("evtest: can't get version");
        return 1;
    }

    printf("Input driver version is %d.%d.%d\n",
        version >> 16, (version >> 8) & 0xff, version & 0xff);

    ioctl(fd, EVIOCGID, id);
    printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
        id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    printf("Input device name: \"%s\"\n", name);

    memset(bit, 0, sizeof(bit));
    ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
    printf("Supported events:\n");

    for (type=0; type<EV_MAX; type++) {
        if (test_bit(type, bit[0]) && type != EV_REP) {
            
        }
    }


}

static char* scan_devices(void) 
{
    struct dirent **namelist;
    int i, ndev, devnum;
    char *filename;
    int max_device = 0;

    ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
    if (ndev <= 0)
        return NULL;

    fprintf(stderr, "Available devices:\n");

    for (i=0; i<ndev; i++) {
        char fname[64];
        int fd = -1;
        char name[256] = "???";

        snprintf(fname, sizeof(fname), "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);

        fd = open(fname, O_RDONLY);
        if (fd < 0)
            continue;

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);

        fprintf(stderr, "%s: %s\n", fname, name);
        close(fd);

        sscanf(namelist[i]->d_name, "event%d", &devnum);
        if (devnum > max_device) 
            max_device = devnum;

        free(namelist[i]);
    }
       
    fprintf(stderr, "Select the device event number [0-%d]: ", max_device);
    scanf("%d", &devnum);

    if (devnum > max_device || devnum < 0) {
        return NULL;
    } 

    asprintf(&filename, "%s/%s%d", DEV_INPUT_EVENT, EVENT_DEV_NAME, devnum);
    
    return filename;
}

static int test_grab(int fd, int grab_flag) {
    int rc;

    rc = ioctl(fd, EVIOCGRAB, (void*)0);

    return rc;
}

static int usage(void) {
    printf("USAGE:\n");
    printf(" Capture mode:\n");
    printf("   %s [--grab] /dev/input/eventX\n", program_invocation_short_name);
    printf("     --grab grab the device for exclusive access\n");
    printf("\n");
    printf(" Query mode: (Check exit code)\n");
    printf("   %s --query /dev/input/eventX <type> <value>\n", program_invocation_short_name);
    printf("\n");
    printf("<type> is one of: EV_KEY, EV_SW, EV_LED, EV_SND\n");
    printf("<value> can be either be a numerical value, or the textual name of the\n");
    printf("key/switch/LED/sound being queried (e.g. SW_DOCK).\n");

    return EXIT_FAILURE;
}

static int do_capture(const char* device, int grab_flag) 
{
    int fd;
    char *filename = NULL;

    if (!device) {
        fprintf(stderr, "No device specified, trying to scan all of %s/%s*\n",
            DEV_INPUT_EVENT, EVENT_DEV_NAME);

        if (getuid() != 0)
            fprintf(stderr, "Not running as root, no devices may be available.\n");

        filename = scan_devices();
        if (!filename)
            return usage();
    } else {
        filename = strdup(device);
    }

    if (!filename)
        return EXIT_FAILURE;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        perror("evtest");

        if (errno == EACCES && getuid() != 0) 
            fprintf(stderr, "You do not have access to %s. Try "
                "running as root instead.\n", filename);

            goto error;
    }

    if (!isatty(fileno(stdout)))
        setbuf(stdout, NULL);

    if (print_device_info(fd))
        goto error;

    printf("Testing ... (interrupt to exit)\n");

    if (test_grab(fd, grab_flag)) {
        printf("************************************************\n");
        printf(" This device is grabbed by another process.\n");
        printf(" No events are available to evtest while the\n"
            " other grab is active.\n");
        printf(" In most cases, this is caused by an X drievr,\n"
            " try VT-switching and re-run evtest again.\n");
        printf(" Run the followin command to see processes with\n"
            " an open fd on this device\n"
            " \"fuser -v %s\"\n", filename);
        printf("***********************************************\n");
    }

    signal(SIGINT, interrupt_handler);
    signal(SIGTERM, interrupt_handler);

    return print_events(fd);

error:
    free(filename);
    return EXIT_FAILURE;
}

static int do_query(const char* device, const char *event_type, const char *keyname) 
{
    const struct query_mode *query_mode;
}
static int version(void) 
{
#ifndef PACKAGE_VERSION 
#define PACKAGE_VERSION "<version undefined>"
#endif
    printf("%s %s\n", program_invocation_short_name, PACKAGE_VERSION);
    return EXIT_SUCCESS;
} 

int main(int argc, char **argv) {
    const char *device = NULL;
    const char *keyname;
    const char *event_type;
    enum evtest_mode mode = MODE_CAPTURE;

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "", long_options, &option_index);

        if (c == -1)
        break;

        switch (c) {
            case 0:
                break;
            case MODE_QUERY:
                mode = c;
                break;
            case MODE_VERSION:
                return version();
            default:
                return usage();
        }
    }

    if (optind < argc) 
        device = argv[optind++];

    if (mode == MODE_CAPTURE) 
        return do_capture(device, grab_flag);

    if ((argc-optind) < 2) {
        fprintf(stderr, "Query mode requires device, type and key parameters\n");
        return usage();
    }

    event_type = argv[optind++];
    keyname = argv[optind++];

    return do_query(device, event_type, keyname);
}