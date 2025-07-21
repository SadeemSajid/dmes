#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* Networking */
#include <sys/socket.h>

/* Client Handling */
#include <pthread.h>

#include "crypto.h" // PQC - RLWE
#include "setup.h"  // Room/Client setups

#define PORT 1337
#define ROOM_SIZE 24

/*
 * Arguments;
 * -c: Starts as a client. Needs address to connect [-a address]
 * -r: Hosts a room and generates an address.
 * -a: Specify an address to use for connection. Required for clients only.
 * -p: Specify a port to use for connection (default: 1337)
 */

// Room broadcast list & current participant size
int room_curr_size = 0;
int *room_broadcast;

int main(int argc, char **argv)
{
        /* App config init */
        struct config_params config = {0, PORT, ROOM_SIZE};

        /* Parse command line options */
        uint8_t type = parse_args(argc, argv, &config);

        if (type == 0)
        {
                printf("ERROR: Unable to parse arguments.\n");
                return EXIT_FAILURE;
        }

        /* Room Flow */
        if (type == 1)
        {
                /* Register signal handler */
                signal(SIGINT, exit_handler);

                int serverfd = make_server(&config);
                if (serverfd != -1)
                {
                        printf("INFO: Room running on port %d.\n", config.port);
                }

                // Start the room to accept clients
                initiate_room(&config, serverfd, room_broadcast, &room_curr_size);

                /* Clean App */
                if (serverfd != -1)
                        close(serverfd);
                if (room_curr_size != 0)
                        free(room_broadcast);
        }
        /* Client Flow */
        else
        {
                printf("INFO: Starting in client mode...\n");
                int roomfd = connect_room(&config);
        }
        return 0;
}
