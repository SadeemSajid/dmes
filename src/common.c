#include "common.h"
#include "room.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* I/O Handling */
#include <string.h>
#include <signal.h>

/* Error Handling */
#include <errno.h>

/* Networking Functionalities */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h> // FIXME: Threads don't scale well
#include <sys/socket.h>

uint8_t parse_args(int argc, char **argv, struct config_params *config)
{
        int option;
        int type = 0;
        bool room_check = false;

        // Default configs
        config->port = htons(PORT);
        config->room_size = ROOM_SIZE;

        /* Default case */
        if (argc == 1) {
                type = DMES_CLIENT_P;
                printf("[INFO] Starting as a passive client...\n");
        }

        while ((option = getopt(argc, argv, ARGUMENTS)) != -1)
        {
                switch (option)
                {
                case 'r':
                        type = DMES_CLIENT_P;
                        room_check = true;
                        break;
                case 'c':
                        type = DMES_CLIENT_A;
                        break;
                case 'p':
                        config->port = htons(atoi(optarg));
                        break;
                case 'l':
                        config->l_port = htons(atoi(optarg));
                        break;
                case 'a':
                        inet_pton(AF_INET, optarg, &config->address);
                        break;
                case 's':
                        config->room_size = atoi(optarg);
                        break;
                default:
                }
        }

        /* Validation */
        if (type == DMES_CLIENT_A && config->address == 0x0)
        {
                printf("[ERROR] An active client must specify an address to connect to!\n");
                printf("Usage: dmes -c -a <address>\n");
                type = DMES_ERROR;
        }

        if (room_check == true && type == CLIENT_TYPE)
        {
                printf("[ERROR] Cannot use both -r & -c!\n");
                type = DMES_ERROR;
        }
        return type;
}

int make_server(struct config_params *config)
{
        struct sockaddr_in addr;
        addr.sin_port = config->port;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY; // Accept all connections

        // Make socket
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
                perror("[ERROR] make_server > socket ");
                return -1;
        }

        // Bind socket
        if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
                perror("[ERROR] make_server > bind ");
                return -1;
        }

        // Listen for connections
        if (listen(fd, config->room_size) == -1)
        {
                perror("[ERROR] make_server > listen ");
                return -1;
        }

        return fd;
}

void exit_handler(int sig)
{
        printf("INFO: Closing room...\n");

        exit(sig);
}