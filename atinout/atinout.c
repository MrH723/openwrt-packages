/*
 * atinout
 *
 * This program takes AT commands as input, sends them to the modem and outputs
 * the responses.
 *
 * Copyright (C) 2013 Håkon Løvdal <hlovdal@users.sourceforge.net>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>

#define MAX_LINE_LENGTH (4 * 1024)
static char buf[MAX_LINE_LENGTH];

static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"usage", no_argument, NULL, 0},
    {NULL, 0, NULL, 0}
};
static const char *short_options = "hV";

static void usage(const char * const argv0)
{
    printf("Usage: %s <input_file> <modem_device> <output_file>\n", argv0);
    printf("\n");
    printf("\t<input_file> is a file with AT commands to be executed. Value '-' means standard input.\n");
    printf("\t<modem_device> is the device file for the modem (e.g. /dev/ttyACM0, /dev/ttyS0, etc).\n");
    printf("\t<output_file> is a file the responses to the AT commands are saved. Value '-' means standard output.\n");
    printf("\n");
    printf("In addition the program supports the following options as the only argument:\n");
    printf("\n");
    printf("\t-h|--help\n");
    printf("\t-V|--version\n");
    printf("\t--usage\n");
    printf("\n");
}

static void help(const char * const argv0)
{
    printf("This program takes AT commands as input, sends them to the modem and outputs\n");
    printf("the responses.\n");
    printf("\n");
    printf("Example:\n");
    printf("\n");
    printf("$ echo 'AT+CPBS=\"ME\",\"ME\"; +CPBR=?' | %s - /dev/ttyACM0 -\n", argv0);
    printf("AT+CPBS=\"ME\",\"ME\"; +CPBR=?\n");
    printf("\n");
    printf("+CPBR: (1-7000),80,62\n");
    printf("\n");
    printf("OK\n");
    printf("$\n");
    printf("\n");
    printf("See also the --usage command line option\n");

}

/**
 * Replace '\n' with '\r', aka `tr '\012' '\015'` for a single line.
 * Supports both "\n" and "\r\n" line endings.
 */
static bool tr_lf_cr(const char *s)
{
    char *p;
    p = strchr(s, '\n');
    if (p == NULL || p[1] != '\0') {
        return false;
    }
    if (p > s && p[-1] == '\r') {
        *p = '\0';
    } else {
        *p = '\r';
    }
    return true;
}

static void strip_cr(char *s)
{
    char *from, *to;
    from = to = s;
    while (*from != '\0') {
        if (*from == '\r') {
            from++;
            continue;
        }
        *to++ = *from++;
    }
    *to = '\0';
}

static bool is_final_result(const char * const response)
{
#define STARTS_WITH(a, b) ( strncmp((a), (b), strlen(b)) == 0)
    switch (response[0]) {
        case '+':
            if (STARTS_WITH(&response[1], "CME ERROR:")) {
                return true;
            }
            if (STARTS_WITH(&response[1], "CMS ERROR:")) {
                return true;
            }
            return false;
        case 'B':
            if (strcmp(&response[1], "USY\n") == 0) {
                return true;
            }

            return false;

        case 'E':
            if (strcmp(&response[1], "RROR\n") == 0) {
                return true;
            }
            return false;
        case 'N':
            if (strcmp(&response[1], "O ANSWER\n") == 0) {
                return true;
            }
            if (strcmp(&response[1], "O CARRIER\n") == 0) {
                return true;
            }
            if (strcmp(&response[1], "O DIALTONE\n") == 0) {
                return true;
            }
            return false;
        case 'O':
            if (strcmp(&response[1], "K\n") == 0) {
                return true;
            }
            // fallthrough
        default:
            return false;
    }
}

int main(int argc, char *argv[])
{
    FILE *atcmds;
    FILE *modem;
    FILE *output;
    char *line;
    bool success;
    int res;
    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                help(argv[0]);
                return EXIT_SUCCESS;
            case 'V':
                printf("atinout version " VERSION "\n");
                if (argc == 2) {
                    printf("Copyright (C) 2013 Håkon Løvdal <hlovdal@users.sourceforge.net>\n"
                            "This program comes with ABSOLUTELY NO WARRANTY.\n"
                            "This is free software, and you are welcome to redistribute it under\n"
                            "certain conditions; see http://www.gnu.org/licenses/gpl.html for details.\n");
                    return EXIT_SUCCESS;
                }
                break;
            case 0:
                if (strcmp("usage", long_options[option_index].name) == 0) {
                    usage(argv[0]);
                    return EXIT_SUCCESS;
                }
                break;
            case '?':
                break;
            default:
                fprintf(stderr, "getopt returned character code 0x%02x\n", (unsigned int)c);
        }
    }

    if ((argc - optind) != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

#define INPUT_FILE   argv[optind + 0]
#define MODEM_DEVICE argv[optind + 1]
#define OUTPUT_FILE  argv[optind + 2]

    if (strcmp(INPUT_FILE, "-") == 0) {
        atcmds = stdin;
    } else {
        atcmds = fopen(INPUT_FILE, "rb");
        if (atcmds == NULL) {
            fprintf(stderr, "fopen(%s) failed: %s\n", INPUT_FILE, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    modem = fopen(MODEM_DEVICE, "r+b");
    if (modem == NULL) {
        fprintf(stderr, "fopen(%s) failed: %s\n", MODEM_DEVICE, strerror(errno));
        return EXIT_FAILURE;
    }

    if (strcmp(OUTPUT_FILE, "-") == 0) {
        output = stdout;
    } else {
        output = fopen(OUTPUT_FILE, "wb");
        if (output == NULL) {
            fprintf(stderr, "fopen(%s) failed: %s\n", OUTPUT_FILE, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    while ((line = fgets(buf, (int)sizeof(buf), atcmds)) != NULL) {
        success = tr_lf_cr(line);
        if (! success) {
            fprintf(stderr, "invalid string: '%s'\n", line);
            return EXIT_FAILURE;
        }
        res = fputs(line, modem);
        if (res < 0) {
            fprintf(stderr, "failed to send '%s' to modem (res = %d)\n", line, res);
            return EXIT_FAILURE;
        }
        do {
            line = fgets(buf, (int)sizeof(buf), modem);
            if (line == NULL) {
                fprintf(stderr, "EOF from modem\n");
                return EXIT_FAILURE;
            }
            strip_cr(line);
            res = fputs(line, output);
            if (res < 0) {
                fprintf(stderr, "failed to write '%s' to output file (res = %d)\n", line, res);
                return EXIT_FAILURE;
            }
        } while (! is_final_result(line));
    }

    if (strcmp(OUTPUT_FILE, "-") != 0) {
        res = fclose(output);
        if (res != 0) {
            fprintf(stderr, "closing output failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
    }
    res = fclose(modem);
    if (res != 0) {
        fprintf(stderr, "closing modem failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (strcmp(INPUT_FILE, "-") != 0) {
        res = fclose(atcmds);
        if (res != 0) {
            fprintf(stderr, "closing input failed: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

