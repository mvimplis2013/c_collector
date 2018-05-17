#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>
#include <signal.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x) ((x)%BITS_PER_LONG)
#define BIT(x) (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG) 
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

#define NAME_ELEMENT(element) [element]=#element

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

static int grab_flag = 0;
static volatile sig_atomic_t stop = 0;

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
	{ "EV_KEY", EV_KEY, KEY_MAX, EVIOCGKEY(KEY_MAX) },
	{ "EV_LED", EV_LED, LED_MAX, EVIOCGLED(LED_MAX) },
	{ "EV_SND", EV_SND, SND_MAX, EVIOCGSND(SND_MAX) },
	{ "EV_SW",  EV_SW, SW_MAX, EVIOCGSW(SW_MAX) },
};
static const struct option long_options[] = {
    {"grab", no_argument, &grab_flag, 1},
    {"query", no_argument, NULL, MODE_QUERY},
    {"version", no_argument, NULL, MODE_VERSION},
    { 0, },
};

static const char * const events[EV_MAX +1] = {
    [0 ... EV_MAX] = NULL,
    NAME_ELEMENT(EV_SYN), NAME_ELEMENT(EV_KEY),
    NAME_ELEMENT(EV_REL), NAME_ELEMENT(EV_ABS),
    NAME_ELEMENT(EV_MSC), NAME_ELEMENT(EV_LED),
    NAME_ELEMENT(EV_SND), NAME_ELEMENT(EV_REP),
    NAME_ELEMENT(EV_FF), NAME_ELEMENT(EV_PWR),
    NAME_ELEMENT(EV_FF_STATUS), NAME_ELEMENT(EV_SW),
};

static const int maxval[EV_MAX + 1] = {
	[0 ... EV_MAX] = -1,
	[EV_SYN] = SYN_MAX,
	[EV_KEY] = KEY_MAX,
	[EV_REL] = REL_MAX,
	[EV_ABS] = ABS_MAX,
	[EV_MSC] = MSC_MAX,
	[EV_SW] = SW_MAX,
	[EV_LED] = LED_MAX,
	[EV_SND] = SND_MAX,
	[EV_REP] = REP_MAX,
	[EV_FF] = FF_MAX,
	[EV_FF_STATUS] = FF_STATUS_MAX,
};

static const char * const keys[KEY_MAX + 1] = {
	[0 ... KEY_MAX] = NULL,
	NAME_ELEMENT(KEY_RESERVED),		NAME_ELEMENT(KEY_ESC),
	NAME_ELEMENT(KEY_1),			NAME_ELEMENT(KEY_2),
	NAME_ELEMENT(KEY_3),			NAME_ELEMENT(KEY_4),
	NAME_ELEMENT(KEY_5),			NAME_ELEMENT(KEY_6),
	NAME_ELEMENT(KEY_7),			NAME_ELEMENT(KEY_8),
	NAME_ELEMENT(KEY_9),			NAME_ELEMENT(KEY_0),
	NAME_ELEMENT(KEY_MINUS),		NAME_ELEMENT(KEY_EQUAL),
	NAME_ELEMENT(KEY_BACKSPACE),		NAME_ELEMENT(KEY_TAB),
	NAME_ELEMENT(KEY_Q),			NAME_ELEMENT(KEY_W),
	NAME_ELEMENT(KEY_E),			NAME_ELEMENT(KEY_R),
	NAME_ELEMENT(KEY_T),			NAME_ELEMENT(KEY_Y),
	NAME_ELEMENT(KEY_U),			NAME_ELEMENT(KEY_I),
	NAME_ELEMENT(KEY_O),			NAME_ELEMENT(KEY_P),
	NAME_ELEMENT(KEY_LEFTBRACE),		NAME_ELEMENT(KEY_RIGHTBRACE),
	NAME_ELEMENT(KEY_ENTER),		NAME_ELEMENT(KEY_LEFTCTRL),
	NAME_ELEMENT(KEY_A),			NAME_ELEMENT(KEY_S),
	NAME_ELEMENT(KEY_D),			NAME_ELEMENT(KEY_F),
	NAME_ELEMENT(KEY_G),			NAME_ELEMENT(KEY_H),
	NAME_ELEMENT(KEY_J),			NAME_ELEMENT(KEY_K),
	NAME_ELEMENT(KEY_L),			NAME_ELEMENT(KEY_SEMICOLON),
	NAME_ELEMENT(KEY_APOSTROPHE),		NAME_ELEMENT(KEY_GRAVE),
	NAME_ELEMENT(KEY_LEFTSHIFT),		NAME_ELEMENT(KEY_BACKSLASH),
	NAME_ELEMENT(KEY_Z),			NAME_ELEMENT(KEY_X),
	NAME_ELEMENT(KEY_C),			NAME_ELEMENT(KEY_V),
	NAME_ELEMENT(KEY_B),			NAME_ELEMENT(KEY_N),
	NAME_ELEMENT(KEY_M),			NAME_ELEMENT(KEY_COMMA),
	NAME_ELEMENT(KEY_DOT),			NAME_ELEMENT(KEY_SLASH),
	NAME_ELEMENT(KEY_RIGHTSHIFT),		NAME_ELEMENT(KEY_KPASTERISK),
	NAME_ELEMENT(KEY_LEFTALT),		NAME_ELEMENT(KEY_SPACE),
	NAME_ELEMENT(KEY_CAPSLOCK),		NAME_ELEMENT(KEY_F1),
	NAME_ELEMENT(KEY_F2),			NAME_ELEMENT(KEY_F3),
	NAME_ELEMENT(KEY_F4),			NAME_ELEMENT(KEY_F5),
	NAME_ELEMENT(KEY_F6),			NAME_ELEMENT(KEY_F7),
	NAME_ELEMENT(KEY_F8),			NAME_ELEMENT(KEY_F9),
	NAME_ELEMENT(KEY_F10),			NAME_ELEMENT(KEY_NUMLOCK),
	NAME_ELEMENT(KEY_SCROLLLOCK),		NAME_ELEMENT(KEY_KP7),
	NAME_ELEMENT(KEY_KP8),			NAME_ELEMENT(KEY_KP9),
	NAME_ELEMENT(KEY_KPMINUS),		NAME_ELEMENT(KEY_KP4),
	NAME_ELEMENT(KEY_KP5),			NAME_ELEMENT(KEY_KP6),
	NAME_ELEMENT(KEY_KPPLUS),		NAME_ELEMENT(KEY_KP1),
	NAME_ELEMENT(KEY_KP2),			NAME_ELEMENT(KEY_KP3),
	NAME_ELEMENT(KEY_KP0),			NAME_ELEMENT(KEY_KPDOT),
	NAME_ELEMENT(KEY_ZENKAKUHANKAKU), 	NAME_ELEMENT(KEY_102ND),
	NAME_ELEMENT(KEY_F11),			NAME_ELEMENT(KEY_F12),
	NAME_ELEMENT(KEY_RO),			NAME_ELEMENT(KEY_KATAKANA),
	NAME_ELEMENT(KEY_HIRAGANA),		NAME_ELEMENT(KEY_HENKAN),
	NAME_ELEMENT(KEY_KATAKANAHIRAGANA),	NAME_ELEMENT(KEY_MUHENKAN),
	NAME_ELEMENT(KEY_KPJPCOMMA),		NAME_ELEMENT(KEY_KPENTER),
	NAME_ELEMENT(KEY_RIGHTCTRL),		NAME_ELEMENT(KEY_KPSLASH),
	NAME_ELEMENT(KEY_SYSRQ),		NAME_ELEMENT(KEY_RIGHTALT),
	NAME_ELEMENT(KEY_LINEFEED),		NAME_ELEMENT(KEY_HOME),
	NAME_ELEMENT(KEY_UP),			NAME_ELEMENT(KEY_PAGEUP),
	NAME_ELEMENT(KEY_LEFT),			NAME_ELEMENT(KEY_RIGHT),
	NAME_ELEMENT(KEY_END),			NAME_ELEMENT(KEY_DOWN),
	NAME_ELEMENT(KEY_PAGEDOWN),		NAME_ELEMENT(KEY_INSERT),
	NAME_ELEMENT(KEY_DELETE),		NAME_ELEMENT(KEY_MACRO),
	NAME_ELEMENT(KEY_MUTE),			NAME_ELEMENT(KEY_VOLUMEDOWN),
	NAME_ELEMENT(KEY_VOLUMEUP),		NAME_ELEMENT(KEY_POWER),
	NAME_ELEMENT(KEY_KPEQUAL),		NAME_ELEMENT(KEY_KPPLUSMINUS),
	NAME_ELEMENT(KEY_PAUSE),		NAME_ELEMENT(KEY_KPCOMMA),
	NAME_ELEMENT(KEY_HANGUEL),		NAME_ELEMENT(KEY_HANJA),
	NAME_ELEMENT(KEY_YEN),			NAME_ELEMENT(KEY_LEFTMETA),
	NAME_ELEMENT(KEY_RIGHTMETA),		NAME_ELEMENT(KEY_COMPOSE),
	NAME_ELEMENT(KEY_STOP),			NAME_ELEMENT(KEY_AGAIN),
	NAME_ELEMENT(KEY_PROPS),		NAME_ELEMENT(KEY_UNDO),
	NAME_ELEMENT(KEY_FRONT),		NAME_ELEMENT(KEY_COPY),
	NAME_ELEMENT(KEY_OPEN),			NAME_ELEMENT(KEY_PASTE),
	NAME_ELEMENT(KEY_FIND),			NAME_ELEMENT(KEY_CUT),
	NAME_ELEMENT(KEY_HELP),			NAME_ELEMENT(KEY_MENU),
	NAME_ELEMENT(KEY_CALC),			NAME_ELEMENT(KEY_SETUP),
	NAME_ELEMENT(KEY_SLEEP),		NAME_ELEMENT(KEY_WAKEUP),
	NAME_ELEMENT(KEY_FILE),			NAME_ELEMENT(KEY_SENDFILE),
	NAME_ELEMENT(KEY_DELETEFILE),		NAME_ELEMENT(KEY_XFER),
	NAME_ELEMENT(KEY_PROG1),		NAME_ELEMENT(KEY_PROG2),
	NAME_ELEMENT(KEY_WWW),			NAME_ELEMENT(KEY_MSDOS),
	NAME_ELEMENT(KEY_COFFEE),		NAME_ELEMENT(KEY_DIRECTION),
	NAME_ELEMENT(KEY_CYCLEWINDOWS),		NAME_ELEMENT(KEY_MAIL),
	NAME_ELEMENT(KEY_BOOKMARKS),		NAME_ELEMENT(KEY_COMPUTER),
	NAME_ELEMENT(KEY_BACK),			NAME_ELEMENT(KEY_FORWARD),
	NAME_ELEMENT(KEY_CLOSECD),		NAME_ELEMENT(KEY_EJECTCD),
	NAME_ELEMENT(KEY_EJECTCLOSECD),		NAME_ELEMENT(KEY_NEXTSONG),
	NAME_ELEMENT(KEY_PLAYPAUSE),		NAME_ELEMENT(KEY_PREVIOUSSONG),
	NAME_ELEMENT(KEY_STOPCD),		NAME_ELEMENT(KEY_RECORD),
	NAME_ELEMENT(KEY_REWIND),		NAME_ELEMENT(KEY_PHONE),
	NAME_ELEMENT(KEY_ISO),			NAME_ELEMENT(KEY_CONFIG),
	NAME_ELEMENT(KEY_HOMEPAGE),		NAME_ELEMENT(KEY_REFRESH),
	NAME_ELEMENT(KEY_EXIT),			NAME_ELEMENT(KEY_MOVE),
	NAME_ELEMENT(KEY_EDIT),			NAME_ELEMENT(KEY_SCROLLUP),
	NAME_ELEMENT(KEY_SCROLLDOWN),		NAME_ELEMENT(KEY_KPLEFTPAREN),
	NAME_ELEMENT(KEY_KPRIGHTPAREN), 	NAME_ELEMENT(KEY_F13),
	NAME_ELEMENT(KEY_F14),			NAME_ELEMENT(KEY_F15),
	NAME_ELEMENT(KEY_F16),			NAME_ELEMENT(KEY_F17),
	NAME_ELEMENT(KEY_F18),			NAME_ELEMENT(KEY_F19),
	NAME_ELEMENT(KEY_F20),			NAME_ELEMENT(KEY_F21),
	NAME_ELEMENT(KEY_F22),			NAME_ELEMENT(KEY_F23),
	NAME_ELEMENT(KEY_F24),			NAME_ELEMENT(KEY_PLAYCD),
	NAME_ELEMENT(KEY_PAUSECD),		NAME_ELEMENT(KEY_PROG3),
	NAME_ELEMENT(KEY_PROG4),		NAME_ELEMENT(KEY_SUSPEND),
	NAME_ELEMENT(KEY_CLOSE),		NAME_ELEMENT(KEY_PLAY),
	NAME_ELEMENT(KEY_FASTFORWARD),		NAME_ELEMENT(KEY_BASSBOOST),
	NAME_ELEMENT(KEY_PRINT),		NAME_ELEMENT(KEY_HP),
	NAME_ELEMENT(KEY_CAMERA),		NAME_ELEMENT(KEY_SOUND),
	NAME_ELEMENT(KEY_QUESTION),		NAME_ELEMENT(KEY_EMAIL),
	NAME_ELEMENT(KEY_CHAT),			NAME_ELEMENT(KEY_SEARCH),
	NAME_ELEMENT(KEY_CONNECT),		NAME_ELEMENT(KEY_FINANCE),
	NAME_ELEMENT(KEY_SPORT),		NAME_ELEMENT(KEY_SHOP),
	NAME_ELEMENT(KEY_ALTERASE),		NAME_ELEMENT(KEY_CANCEL),
	NAME_ELEMENT(KEY_BRIGHTNESSDOWN),	NAME_ELEMENT(KEY_BRIGHTNESSUP),
	NAME_ELEMENT(KEY_MEDIA),		NAME_ELEMENT(KEY_UNKNOWN),
	NAME_ELEMENT(KEY_OK),
	NAME_ELEMENT(KEY_SELECT),		NAME_ELEMENT(KEY_GOTO),
	NAME_ELEMENT(KEY_CLEAR),		NAME_ELEMENT(KEY_POWER2),
	NAME_ELEMENT(KEY_OPTION),		NAME_ELEMENT(KEY_INFO),
	NAME_ELEMENT(KEY_TIME),			NAME_ELEMENT(KEY_VENDOR),
	NAME_ELEMENT(KEY_ARCHIVE),		NAME_ELEMENT(KEY_PROGRAM),
	NAME_ELEMENT(KEY_CHANNEL),		NAME_ELEMENT(KEY_FAVORITES),
	NAME_ELEMENT(KEY_EPG),			NAME_ELEMENT(KEY_PVR),
	NAME_ELEMENT(KEY_MHP),			NAME_ELEMENT(KEY_LANGUAGE),
	NAME_ELEMENT(KEY_TITLE),		NAME_ELEMENT(KEY_SUBTITLE),
	NAME_ELEMENT(KEY_ANGLE),		NAME_ELEMENT(KEY_ZOOM),
	NAME_ELEMENT(KEY_MODE),			NAME_ELEMENT(KEY_KEYBOARD),
	NAME_ELEMENT(KEY_SCREEN),		NAME_ELEMENT(KEY_PC),
	NAME_ELEMENT(KEY_TV),			NAME_ELEMENT(KEY_TV2),
	NAME_ELEMENT(KEY_VCR),			NAME_ELEMENT(KEY_VCR2),
	NAME_ELEMENT(KEY_SAT),			NAME_ELEMENT(KEY_SAT2),
	NAME_ELEMENT(KEY_CD),			NAME_ELEMENT(KEY_TAPE),
	NAME_ELEMENT(KEY_RADIO),		NAME_ELEMENT(KEY_TUNER),
	NAME_ELEMENT(KEY_PLAYER),		NAME_ELEMENT(KEY_TEXT),
	NAME_ELEMENT(KEY_DVD),			NAME_ELEMENT(KEY_AUX),
	NAME_ELEMENT(KEY_MP3),			NAME_ELEMENT(KEY_AUDIO),
	NAME_ELEMENT(KEY_VIDEO),		NAME_ELEMENT(KEY_DIRECTORY),
	NAME_ELEMENT(KEY_LIST),			NAME_ELEMENT(KEY_MEMO),
	NAME_ELEMENT(KEY_CALENDAR),		NAME_ELEMENT(KEY_RED),
	NAME_ELEMENT(KEY_GREEN),		NAME_ELEMENT(KEY_YELLOW),
	NAME_ELEMENT(KEY_BLUE),			NAME_ELEMENT(KEY_CHANNELUP),
	NAME_ELEMENT(KEY_CHANNELDOWN),		NAME_ELEMENT(KEY_FIRST),
	NAME_ELEMENT(KEY_LAST),			NAME_ELEMENT(KEY_AB),
	NAME_ELEMENT(KEY_NEXT),			NAME_ELEMENT(KEY_RESTART),
	NAME_ELEMENT(KEY_SLOW),			NAME_ELEMENT(KEY_SHUFFLE),
	NAME_ELEMENT(KEY_BREAK),		NAME_ELEMENT(KEY_PREVIOUS),
	NAME_ELEMENT(KEY_DIGITS),		NAME_ELEMENT(KEY_TEEN),
	NAME_ELEMENT(KEY_TWEN),			NAME_ELEMENT(KEY_DEL_EOL),
	NAME_ELEMENT(KEY_DEL_EOS),		NAME_ELEMENT(KEY_INS_LINE),
	NAME_ELEMENT(KEY_DEL_LINE),
	NAME_ELEMENT(KEY_VIDEOPHONE),		NAME_ELEMENT(KEY_GAMES),
	NAME_ELEMENT(KEY_ZOOMIN),		NAME_ELEMENT(KEY_ZOOMOUT),
	NAME_ELEMENT(KEY_ZOOMRESET),		NAME_ELEMENT(KEY_WORDPROCESSOR),
	NAME_ELEMENT(KEY_EDITOR),		NAME_ELEMENT(KEY_SPREADSHEET),
	NAME_ELEMENT(KEY_GRAPHICSEDITOR), 	NAME_ELEMENT(KEY_PRESENTATION),
	NAME_ELEMENT(KEY_DATABASE),		NAME_ELEMENT(KEY_NEWS),
	NAME_ELEMENT(KEY_VOICEMAIL),		NAME_ELEMENT(KEY_ADDRESSBOOK),
	NAME_ELEMENT(KEY_MESSENGER),		NAME_ELEMENT(KEY_DISPLAYTOGGLE),
	NAME_ELEMENT(KEY_DEL_EOL),		NAME_ELEMENT(KEY_DEL_EOS),
	NAME_ELEMENT(KEY_INS_LINE),	 	NAME_ELEMENT(KEY_DEL_LINE),
	NAME_ELEMENT(KEY_FN),			NAME_ELEMENT(KEY_FN_ESC),
	NAME_ELEMENT(KEY_FN_F1),		NAME_ELEMENT(KEY_FN_F2),
	NAME_ELEMENT(KEY_FN_F3),		NAME_ELEMENT(KEY_FN_F4),
	NAME_ELEMENT(KEY_FN_F5),		NAME_ELEMENT(KEY_FN_F6),
	NAME_ELEMENT(KEY_FN_F7),		NAME_ELEMENT(KEY_FN_F8),
	NAME_ELEMENT(KEY_FN_F9),		NAME_ELEMENT(KEY_FN_F10),
	NAME_ELEMENT(KEY_FN_F11),		NAME_ELEMENT(KEY_FN_F12),
	NAME_ELEMENT(KEY_FN_1),			NAME_ELEMENT(KEY_FN_2),
	NAME_ELEMENT(KEY_FN_D),			NAME_ELEMENT(KEY_FN_E),
	NAME_ELEMENT(KEY_FN_F),			NAME_ELEMENT(KEY_FN_S),
	NAME_ELEMENT(KEY_FN_B),
	NAME_ELEMENT(KEY_BRL_DOT1),		NAME_ELEMENT(KEY_BRL_DOT2),
	NAME_ELEMENT(KEY_BRL_DOT3),		NAME_ELEMENT(KEY_BRL_DOT4),
	NAME_ELEMENT(KEY_BRL_DOT5),		NAME_ELEMENT(KEY_BRL_DOT6),
	NAME_ELEMENT(KEY_BRL_DOT7),		NAME_ELEMENT(KEY_BRL_DOT8),
	NAME_ELEMENT(KEY_BRL_DOT9),		NAME_ELEMENT(KEY_BRL_DOT10),
	NAME_ELEMENT(KEY_BATTERY),
	NAME_ELEMENT(KEY_BLUETOOTH),		NAME_ELEMENT(KEY_BRIGHTNESS_CYCLE),
	NAME_ELEMENT(KEY_BRIGHTNESS_ZERO),
	NAME_ELEMENT(KEY_DISPLAY_OFF),		NAME_ELEMENT(KEY_DOCUMENTS),
	NAME_ELEMENT(KEY_FORWARDMAIL),		NAME_ELEMENT(KEY_NEW),
	NAME_ELEMENT(KEY_KBDILLUMDOWN),		NAME_ELEMENT(KEY_KBDILLUMUP),
	NAME_ELEMENT(KEY_KBDILLUMTOGGLE), 	NAME_ELEMENT(KEY_REDO),
	NAME_ELEMENT(KEY_REPLY),		NAME_ELEMENT(KEY_SAVE),
	NAME_ELEMENT(KEY_SEND),
	NAME_ELEMENT(KEY_SCREENLOCK),		NAME_ELEMENT(KEY_SWITCHVIDEOMODE),
	NAME_ELEMENT(BTN_0),			NAME_ELEMENT(BTN_1),
	NAME_ELEMENT(BTN_2),			NAME_ELEMENT(BTN_3),
	NAME_ELEMENT(BTN_4),			NAME_ELEMENT(BTN_5),
	NAME_ELEMENT(BTN_6),			NAME_ELEMENT(BTN_7),
	NAME_ELEMENT(BTN_8),			NAME_ELEMENT(BTN_9),
	NAME_ELEMENT(BTN_LEFT),			NAME_ELEMENT(BTN_RIGHT),
	NAME_ELEMENT(BTN_MIDDLE),		NAME_ELEMENT(BTN_SIDE),
	NAME_ELEMENT(BTN_EXTRA),		NAME_ELEMENT(BTN_FORWARD),
	NAME_ELEMENT(BTN_BACK),			NAME_ELEMENT(BTN_TASK),
	NAME_ELEMENT(BTN_TRIGGER),		NAME_ELEMENT(BTN_THUMB),
	NAME_ELEMENT(BTN_THUMB2),		NAME_ELEMENT(BTN_TOP),
	NAME_ELEMENT(BTN_TOP2),			NAME_ELEMENT(BTN_PINKIE),
	NAME_ELEMENT(BTN_BASE),			NAME_ELEMENT(BTN_BASE2),
	NAME_ELEMENT(BTN_BASE3),		NAME_ELEMENT(BTN_BASE4),
	NAME_ELEMENT(BTN_BASE5),		NAME_ELEMENT(BTN_BASE6),
	NAME_ELEMENT(BTN_DEAD),			NAME_ELEMENT(BTN_C),
	NAME_ELEMENT(BTN_Z),			NAME_ELEMENT(BTN_TL),
	NAME_ELEMENT(BTN_TR),			NAME_ELEMENT(BTN_TL2),
	NAME_ELEMENT(BTN_TR2),			NAME_ELEMENT(BTN_SELECT),
	NAME_ELEMENT(BTN_START),		NAME_ELEMENT(BTN_MODE),
	NAME_ELEMENT(BTN_THUMBL),		NAME_ELEMENT(BTN_THUMBR),
	NAME_ELEMENT(BTN_TOOL_PEN),		NAME_ELEMENT(BTN_TOOL_RUBBER),
	NAME_ELEMENT(BTN_TOOL_BRUSH),		NAME_ELEMENT(BTN_TOOL_PENCIL),
	NAME_ELEMENT(BTN_TOOL_AIRBRUSH),	NAME_ELEMENT(BTN_TOOL_FINGER),
	NAME_ELEMENT(BTN_TOOL_MOUSE),		NAME_ELEMENT(BTN_TOOL_LENS),
	NAME_ELEMENT(BTN_TOUCH),		NAME_ELEMENT(BTN_STYLUS),
	NAME_ELEMENT(BTN_STYLUS2),		NAME_ELEMENT(BTN_TOOL_DOUBLETAP),
	NAME_ELEMENT(BTN_TOOL_TRIPLETAP),
	NAME_ELEMENT(BTN_GEAR_DOWN),
	NAME_ELEMENT(BTN_GEAR_UP),
};

static const char * const syns[SYN_MAX + 1] = {
    [0 ... SYN_MAX] = NULL,
    NAME_ELEMENT(SYN_REPORT),
    NAME_ELEMENT(SYN_CONFIG),
};

static const char * const * const names[EV_MAX +1] = {
    [0 ... EV_MAX] = NULL,
    [EV_SYN] = syns,    [EV_KEY] = keys,
};


static const char * const absval[6] = {"Value", "Min ", "Max ", "Fuzz ", "Flat ", "Resolution "};

static void interrupt_handler(int sig) {
    stop = 1;
}

static inline const char* typename(unsigned int type) {
    return (type <= EV_MAX && events[type]) ? events[type] : "?";
}

static inline const char * codename(unsigned int type, unsigned int code) {
    return (type <= EV_MAX && code <= maxval[type] && names[type][code]) ? names[type][code] : "?";    
}

static void print_absdata(int fd, int axis) {
    int abs[6] = {0};
    int k;

    ioctl(fd, EVIOCGABS(axis), abs);
    for (k=0; k<6; k++)
        if ((k<3) || abs[k])
            printf("  %s %6d\n", absval[k], abs[k]);
}

static void print_repdata(int fd) {
    int i;
    unsigned int rep[2];

    ioctl(fd, EVIOCGREP, rep);

    for (i=0; i<=REP_MAX; i++) {
        printf(" Repeat code %d(%s)\n", i, names[EV_REP] ? (names[EV_REP][i] ? names[EV_REP][i] : "?") : "?");
        printf("   Value %6d\n", rep[i]);
    }
}

static int is_event_device(const struct dirent *dir) {
    return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

static int print_device_info(int fd) {
    unsigned int type, code;
    int version;
    unsigned short id[4];
    char name[256] = "Unknown";
    unsigned long bit[EV_MAX][NBITS(KEY_MAX)];

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
            
            printf(" Event type %d (%s)\n", type, typename(type));
            if (type == EV_SYN) 
                continue;
            ioctl(fd, EVIOCGBIT(type, KEY_MAX), bit[type]);
            for (code=0; code<KEY_MAX; code++) {
                printf("ev_max = %d , type = %d , BIType = %d, code = %d\n", EV_MAX, type, bit[type], code);
                if (test_bit(code, bit[type])) {
                    printf(" Event Code %d (%s)\n", code, codename(type, code));
                    if (type == EV_ABS)
                        print_absdata(fd, code);
                }
            }
        }
    }

    if (test_bit(EV_REP, bit[0])) {
        printf("key repeat handling:\n");
        printf(" Repeat type %d (%s)\n", EV_REP, events[EV_REP] ? events[EV_REP] : "?");
        print_repdata(fd);
    }

    return 0;
}

static int print_events(int fd) {
    struct input_event ev[64];
    int i, rd;
    fd_set rdfs;

    FD_ZERO(&rdfs);
    FD_SET(fd, &rdfs);

    while (!stop) {
        select(fd+1, &rdfs, NULL, NULL, NULL);
        if (stop)
            break;
        rd = read(fd, ev, sizeof(ev));

        if (rd < (int)sizeof(struct input_event)) {
            printf("expected %d bytes, got %d\n", (int)sizeof(struct input_event), rd);
            perror("\nerror reading");
            return 1;
        }

        for (i=0; i<rd/sizeof(struct input_event); i++) {
            unsigned int type, code;

            type = ev[i].type;
            code = ev[i].code;

            printf("Event: time %ld.%06ld, ", ev[i].time.tv_sec, ev[i].time.tv_usec);

            if (type == EV_SYN) {
                if (code == SYN_MT_REPORT)
                    printf("++++++++++++++++ %s +++++++++++++++\n", codename(type, code));
                else if (code == SYN_DROPPED)
                    printf(">>>>>>>>>>>>>>>> %s >>>>>>>>>>>>>>>\n", codename(type, code));
                else 
                    printf("---------------- %s ---------------\n", codename(type, code));
            } else {
                printf("type %d (%s), code %d (%s), ", 
                    type, typename(type),
                    code, codename(type, code) );
                if (type == EV_MSC && (code == MSC_RAW || code == MSC_SCAN))
                    printf("value %02x\n", ev[i].value);
                else 
                    printf("value %d\n", ev[i].value);
            }
        }
    }
    ioctl(fd, EVIOCGRAB, (void*)0);
    return EXIT_SUCCESS;
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
    printf("*************************************");
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

        printf("+++++++++++++++++++++++++++++++++++++++++++++");
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