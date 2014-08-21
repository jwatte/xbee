
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <string.h>



char const *baud = "9600";
char const *port = "/dev/ttyUSB0";
char const *command = NULL;
char const *filename = "-";

struct baud_lookup {
    int baud;
    int symbol;
}
baudtable[] = {
    { 300, B300 },
    { 1200, B1200 },
    { 2400, B2400 },
    { 4800, B4800 },
    { 9600, B9600 },
    { 19200, B19200 },
    { 38400, B38400 },
    { 57600, B57600 },
    { 115200, B115200 }
};

int lookup_baud(int baud) {
    for (size_t i = 0; i != sizeof(baudtable)/sizeof(baudtable[0]); i++) {
        if (baudtable[i].baud == baud) {
            return baudtable[i].symbol;
        }
    }
    return 0;
}

void usage() {
    fprintf(stderr, "usage: xbee [-b baud] [-p port] cmd [filename]\n");
    fprintf(stderr, "cmd is 'dump' or 'load.'\n");
    exit(EXIT_FAILURE);
}

FILE *openfile(char const *mode, FILE *dflt) {
    if (!filename || !strcmp(filename, "-") || !strcmp(filename, "--")) {
        return dflt;
    }
    FILE *ret = fopen(filename, mode);
    if (!ret) {
        fprintf(stderr, "Could not open %s.\n", filename);
        exit(EXIT_FAILURE);
    }
    return ret;
}


int portfd;

void openport(int bval) {
    portfd = open(port, O_RDWR);
    if (portfd < 0) {
        perror(port);
        exit(EXIT_FAILURE);
    }
    struct termios tio;
    if (tcgetattr(portfd, &tio) < 0) {
        fprintf(stderr, "%s: tcgetattr() failed\n", port);
        exit(EXIT_FAILURE);
    }
    cfmakeraw(&tio);
    cfsetspeed(&tio, bval);
    if (tcsetattr(portfd, TCSAFLUSH, &tio) < 0) {
        fprintf(stderr, "%s: tcsetattr() failed\n", port);
        exit(EXIT_FAILURE);
    }
}


char pline[1024];

void pwrite(char const *txt) {
    size_t l = strlen(txt);
    int w = write(portfd, txt, l);
    if (w < 0) {
        perror(port);
        exit(EXIT_FAILURE);
    }
    if ((size_t)w != l) {
        fprintf(stderr, "%s: short write.\n", port);
        exit(EXIT_FAILURE);
    }
}

void pread() {
    char *iptr = pline;
    while (iptr < &pline[sizeof(pline)-1]) {
        int r = read(portfd, iptr, 1);
        if (r < 0) {
            perror(port);
            exit(EXIT_FAILURE);
        }
        else if (r != 1) {
            fprintf(stderr, "%s: short read.\n", port);
            exit(EXIT_FAILURE);
        }
        ++iptr;
        *iptr = 0;
        if (iptr[-1] == '\r') {
            break;
        }
    }
}

void translate(char *line, int from, int to) {
    while (*line) {
        if (*line == from) {
            *line = to;
        }
        ++line;
    }
}


void cmdmode() {
    sleep(1);
    pwrite("+++");
    sleep(1);
    pread();
    if (strcmp(pline, "OK\r")) {
        fprintf(stderr, "Failure entering command mode.\n");
        exit(EXIT_FAILURE);
    }
}

void exitcmd() {
    pwrite("ATCN\r");
}


char const *cmds[] = {
    "ATMY",
    "ATID",
    "ATDL",
    "ATDH",
    "ATCH",
    "ATRO",
    "ATRR",
    "ATA1",
    "ATCA",
    "ATBD"
};

void do_dump() {
    FILE *f = openfile("wb", stdout);
    cmdmode();
    for (size_t i = 0; i != sizeof(cmds)/sizeof(cmds[0]); ++i) {
        pwrite(cmds[i]);
        pwrite("\r");
        pread();
        translate(pline, '\r', '\n');
        fprintf(f, "%s%s", cmds[i], pline);
    }
    exitcmd();
    fflush(f);
    if (f != stdout) {
        fclose(f);
    }
}

void do_load() {
    FILE *f = openfile("rb", stdin);
    cmdmode();
    char line[1024];
    while (true) {
        line[0] = 0;
        fgets(line, 1024, f);
        if (line[0] == 0) {
            break;
        }
        if (line[0] != '#' && line[0] != '\n') {
            translate(line, '\n', '\r');
            pwrite(line);
            pread();
            if (strcmp(pline, "OK\r")) {
                fprintf(stderr, "Command failed: %s: %s\n", line, pline);
                exit(EXIT_FAILURE);
            }
        }
    }
    pwrite("ATWR\r");
    pread();
    exitcmd();
    if (strcmp(pline, "OK\r")) {
        fprintf(stderr, "Could not write changes: %s\n", pline);
        exit(EXIT_FAILURE);
    }
    if (f != stdin) {
        fclose(f);
    }
}

int main(int argc, char * const *argv) {

    int opt = 0;

    //  getopt treats argv interstingly
    while ((opt = getopt(argc, argv, "b:p:")) != -1) {
        switch (opt) {
        case 'b':
            baud = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            usage();
            break;
        }
    }

    int baudi = atoi(baud);
    int bval = lookup_baud(baudi);
    if (bval <= 0) {
        fprintf(stderr, "Not a recognized baud rate: %s\n", baud);
        exit(EXIT_FAILURE);
    }

    if (optind >= argc) {
        usage();
    }
    command = argv[optind++];
    if (optind < argc) {
        filename = argv[optind++];
    }
    if (optind < argc) {
        usage();
    }

    openport(bval);

    if (!strcmp(command, "dump")) {
        do_dump();
    }
    else if (!strcmp(command, "load")) {
        do_load();
    }
    else {
        fprintf(stderr, "unknown command '%s'\n", command);
        usage();
    }

    return EXIT_SUCCESS;
}

