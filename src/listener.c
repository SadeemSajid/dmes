#include "listener.h"
#include "client.h"
#include "common.h"

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/* Networking Functionalities */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void *handle_listener(void *arg)
{
        
        listener_args *params = (listener_args *) arg;
        pthread_t threads[params->config.room_size];

        /* Open a socket for listening */

        struct sockaddr_in addr;
        addr.sin_port = params->config.l_port;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY; // Accept all connections

        /* Make socket */
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
                perror("[ERROR] handle_listener > socket ");
                goto exit_error;
        }

        // TODO: Not sure about this
        /* Set SO_REUSEADDR before binding */

        int option_value = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value)) < 0) {
                perror("[ERROR] handle_listener > setsockopt(SO_REUSEADDR) ");
                close(fd);
                goto exit_error;
        }

        /* Bind socket */
        if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
                perror("[ERROR] handle_listener > bind ");
                goto exit_error;
        }

        /* Listen for connections */
        if (listen(fd, params->config.room_size) == -1)
        {
                perror("[ERROR] handle_listener > listen ");
                goto exit_error;
        }

        printf("[DEBUG] handle_listener listening...\n");

        /* Spawn threads to handle participants */

        // Initiate client arguments
        client_args args = {&params->config.room_size, fd, params->config.type};

        // TODO: Error handling, close all threads before exiting

        // Create threads
        for (int i = 0; i < params->config.room_size; i++)
                if (pthread_create(&threads[i], NULL, handle_client, (void *)&args) != 0) {
                        perror("[ERROR] handle_listener > pthread_create ");
                        goto exit_error;
                }

        /* Wait for main thread signal */
        pthread_mutex_lock(&params->mailbox.mutex);
        while (params->mailbox.ready == 0)
                pthread_cond_wait(&params->mailbox.cond, &params->mailbox.mutex);
        pthread_mutex_unlock(&params->mailbox.mutex);
        printf("[INFO] Listener shut down\n");

        goto exit_normal;
exit_error:
        fprintf(stderr, "[ERROR] listener could not be started\n");
exit_normal:
        // TODO: use return values to signify different states like error
        pthread_exit(NULL);
}


void *handle_client(void *arg)
{
        client_args args = *(client_args *)arg;
        int client_fd = -1;

        // I/O Operations
        char client_buf[1024];
        ssize_t read_len;
        b_mes rem_msg;

        /* Client information */
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
        struct config_params client_conf_params;
        
        /* Reconnection information */
        uint8_t repeat;
        pthread_t reconnect_thread;

        /* Main loop */
        while(true) {

                // TODO: Do something about the address
                client_fd = accept(args.server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

                if (client_fd == -1)
                {
                        perror("[ERROR] handle_client > accept ");
                        pthread_exit(NULL);
                }

                /* Connect to client's listener */

                /* read listener port message */
                read_len = read(client_fd, (void *) &rem_msg, sizeof(rem_msg));

                if (read_len == 0 || rem_msg.type != DMES_MSG_LPM) {
                        printf("[ERROR] Could not get client LPM.\n");
                        send(client_fd, "!!", sizeof "!!", 0);
                        goto disconnect;
                } else {
                        // memcpy(&client_addr.sin_port, rem_msg.msg, sizeof(uint16_t));
                        client_addr.sin_port = ((lpm_t *) rem_msg.msg)->port;
                        repeat = ((lpm_t *) rem_msg.msg)->repeat;
                }

                /* call dmes_connect in a separate thread */

                if (repeat == 1) {
                        printf("[DEBUG] Repeat set, dmes_connect called\n");
                        client_conf_params.address = client_addr.sin_addr.s_addr;
                        client_conf_params.port = client_addr.sin_port;
                        client_conf_params.room_size = *args.size;
                        client_conf_params.type = args.l_type;

                        pthread_create(&reconnect_thread, NULL, dmes_reconnect, (void *) &client_conf_params);
                }

                /* Main Recv Loop */
                while (true)
                {
                        read_len = read(client_fd, client_buf, BUF_SIZE);

                        if (read_len == 0)
                                break;
                        // Remove trailing newline character
                        client_buf[read_len] = 0;

                        // Leave room
                        if (strcmp(client_buf, "!!") == 0) {
                                send(client_fd, "!!", sizeof "!!", 0);
                                close(client_fd);
                                break;
                        }

                        printf("CLIENT %d: %s\n", client_fd, client_buf);
                }
                *args.size -= 1;

disconnect:                        
                /* TODO: Wait for connection to close */
                printf("[INFO] Terminated connection with client %d\n", client_fd);
        }
        pthread_exit(NULL);
}

void *dmes_reconnect(void *arg)
{
        dmes_connect((struct config_params *) arg);
        pthread_exit(NULL);
}