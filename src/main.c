#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* Networking */
#include <sys/socket.h>

/* Concurrency */
#include <pthread.h>

/* Functionality includes */
#include "common.h"
#include "room.h"
#include "client.h"
#include "crypto.h"
#include "listener.h"

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
        system("clear"); // Clear the screen

        /* Listens for incoming communication */
        pthread_t listener;

        /* App config init */
        struct config_params config = {0, PORT, ROOM_SIZE, PORT};
        listener_args l_config;

        /* Parse command line options */
        uint8_t type = parse_args(argc, argv, &config);
        config.type = type;

        if (type == DMES_ERROR)
        {
                printf("[ERROR] Unable to parse arguments.\n");
                return EXIT_FAILURE;
        }

        /* Init params and spawn a listener thread */
        l_config.config = config;
        l_config.mailbox.ready = 0;
        pthread_mutex_init(&l_config.mailbox.mutex, NULL);
        pthread_cond_init(&l_config.mailbox.cond, NULL);
        pthread_create(&listener, NULL, handle_listener, (void *) &l_config);

        /* Active Client Flow */
        if (type == DMES_CLIENT_A) {
                printf("INFO: Active client mode\n");
                /* Start the session */
                dmes_connect(&config);

                /* Shutdown the listener */
                printf("[INFO] Shutting down listener...\n");
                pthread_mutex_lock(&l_config.mailbox.mutex);
                l_config.mailbox.ready = 1;
                pthread_cond_signal(&l_config.mailbox.cond);
                pthread_mutex_unlock(&l_config.mailbox.mutex);
        }
        else if (type == DMES_CLIENT_P) {
                printf("[INFO] Passive client mode\n");
                
                /* Wait for listener thread */
                pthread_join(listener, NULL);
        }

        return 0;
}
