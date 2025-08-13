#include <stdint.h>
#include "common.h"

/* Arguments passed to the listener thread */
typedef struct {
        struct config_params config;    // app config params
        shared_data mailbox;            // shared data with main thread
} listener_args;

/* Argument sent to the thread handling a client connection in handle_client */
typedef struct {
        uint16_t *size;         // Current room size
        int server_fd;          // Server file descriptor
        app_type l_type;        // Listener application type
} client_args;

/*
 * Main logic for the listener thread.
 * 
 * PARAMS
 *      @arg: listener args
 * 
 * RETURNS
 *      NULL
 */
void *handle_listener(void *);

/*
 * Handles a client connection to the room.
 * PARAMS
 *      @arg: Data needed for the client
 * */
void *handle_client(void *);

/*
 * Connects to a client's listener.
 * PARAMS
 *      @arg: Data needed for the connection
 * */
void *connect_listener(void *);


/*
 * Connects to a connected client's listener
 * */
void *dmes_reconnect(void *);