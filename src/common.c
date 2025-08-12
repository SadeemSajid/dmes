#include "common.h"
#include "client.h"
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

/* APP CONFIGS */
#define ARGUMENTS "ca:rps:"
#define ROOM_TYPE 1
#define CLIENT_TYPE 2
#define PORT 1337
#define ROOM_SIZE 24

// Thread args
struct client_args
{
        int *blist;    // Broadcast list
        int *size;     // Current room size
        int server_fd; // Server file descriptor
};

uint8_t parse_args(int argc, char **argv, struct config_params *config)
{
        int option;
        int type = 0;
        bool room_check = false;

        char *usage = "Usage: dmes [-p port]\n";
        if (argc == 1)
        {
                printf("Invalid Configuration!\n");
                printf("%s\n", usage);
                return 0;
        }

        // Default configs
        config->port = htons(PORT);
        config->room_size = ROOM_SIZE;

        while ((option = getopt(argc, argv, ARGUMENTS)) != -1)
        {
                switch (option)
                {
                case 'r':
                        type = ROOM_TYPE;
                        room_check = true;
                        break;
                case 'c':
                        type = CLIENT_TYPE;
                        break;
                case 'p':
                        config->port = htons(atoi(optarg));
                        break;
                case 'a':
                        inet_pton(AF_INET, optarg, &config->address);
                        break;
                case 's':
                        config->room_size = atoi(optarg);
                default:
                }
        }

        /* Validation */
        if (type == CLIENT_TYPE && config->address == 0x0)
        {
                printf("ERROR: A client must specify a room address to connect to!\n");
                printf("Usage: dmes -c -a <address>\n");
                type = 0;
        }

        if (room_check == true && type == CLIENT_TYPE)
        {
                printf("ERROR: Cannot use both -r & -c!\n");
                type = 0;
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
                perror("ERROR: make_server > socket | ");
                return -1;
        }

        // Bind socket
        if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
                perror("ERROR: make_server > bind | ");
                return -1;
        }

        // Listen for connections
        if (listen(fd, config->room_size) == -1)
        {
                perror("ERROR: make_server > listen | ");
                return -1;
        }

        return fd;
}

int connect_room(struct config_params *params)
{
        // Connection info
        struct sockaddr_in server_addr;
        server_addr.sin_port = params->port;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = params->address;

        // To handle incoming messages
        pthread_t recv_thread;

        // Prepare socket
        int serverfd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverfd == -1)
        {
                perror("ERROR: connect_room > socket | ");
                return -1;
        }

        // TODO: Negotiate security

        // Connect to room
        if (connect(serverfd, (const struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == -1)
        {
                perror("ERROR: connect_room > connect | ");
                return -1;
        }
        printf("INFO: Connected to room.\n");

        // I/O Operations
        char *msg = NULL;
        size_t read_len, bytes_sent, size = 0;

        if (pthread_create(&recv_thread, NULL, handle_recv, (void *)&serverfd))
        {
                perror("ERROR: connect_room > pthread_create | ");
                return -1;
        }

        /* Main Loop */

        while (true)
        {
                read_len = getline(&msg, &size, stdin);

                if (read_len == 0)
                        break;

                // Remove trailing newline character
                msg[read_len - 1] = 0;

                // Send message
                bytes_sent = send(serverfd, msg, read_len - 1, 0);

                if (bytes_sent == -1)
                {
                        perror("ERROR: connect_room > send | ");
                        break;
                }

                // Connection termination
                if (strcmp(msg, "!!") == 0)
                        break;
        }

        /* Wait for thread to complete */
        if (pthread_join(recv_thread, NULL))
        {
                perror("ERROR: connect_room > pthread_join | ");
                return -1;
        }

        return serverfd;
}

void *handle_client(void *arg)
{
        struct client_args args = *(struct client_args *)arg;

        // I/O Operations
        char client_buf[1024];
        ssize_t read_len;

        // TODO: Do something about the address
        int client_fd = accept(args.server_fd, NULL, NULL);

        /* Broadcast message */
        b_mes broadcast_msg;

        if (client_fd == -1)
        {
                perror("ERROR: handle_client > accept | ");
                pthread_exit(NULL);
        }

        // Update blist
        args.blist[*args.size] = client_fd;
        *args.size += 1;

        /* Main Recv Loop */
        while (true)
        {
                read_len = read(client_fd, client_buf, 1024);

                if (read_len == 0)
                        break;
                // Remove trailing newline character
                client_buf[read_len] = 0;

                // Leave room
                if (strcmp(client_buf, "!!") == 0)
                {
                        send(client_fd, "!!", sizeof "!!", 0);
                        break;
                }

                printf("CLIENT %d: %s\n", client_fd, client_buf);

                broadcast_msg.id = client_fd;
                strcpy(broadcast_msg.msg, client_buf);

                // Broadcast message to the rest of the room
                for (int i = 0; i < *args.size; i++)
                {
                        if (args.blist[i] != client_fd)
                                send(args.blist[i], &broadcast_msg, sizeof(broadcast_msg), 0);
                }
        }

        // Clean blist
        bool swap = false;
        int temp = 0;
        for (int i = 0; i < *args.size; i++)
        {
                if (args.blist[i] == client_fd)
                {
                        args.blist[i] = -1;
                        swap = true;
                }

                if (swap == true && i != *args.size - 1)
                {
                        temp = args.blist[i];
                        args.blist[i] = args.blist[i + 1];
                        args.blist[i + 1] = temp;
                }
        }
        *args.size -= 1;

        // DEBUG - Check blist
        printf("DEBUG: blist\n");
        for (int i = 0; i < *args.size; i++)
        {
                printf("%d ", args.blist[i]);
        }
        printf("\n");

        close(client_fd);
        printf("INFO: Terminated connection with client %d\n", client_fd);
        pthread_exit(NULL);
}

void *handle_recv(void *arg)
{
        int serverfd = *(int *)arg;
        ssize_t read_len = 0;
        b_mes recv_msg;

        /* Main Recv Loop */
        while (true)
        {
                read_len = read(serverfd, &recv_msg, sizeof(recv_msg));

                if (read_len == 0)
                        break;

                // Leave room
                if (strcmp(recv_msg.msg, "!!") == 0)
                        break;

                printf("FROM %d: %s\n", recv_msg.id, recv_msg.msg);
        }

        pthread_exit(NULL);
}

void initiate_room(struct config_params *config, int server_fd,
                   int *broadcast_list, int *size)
{

        // Thread implementation
        pthread_t threads[config->room_size];

        // Initiate client descriptors for broadcasting
        struct client_args args = {broadcast_list, size, server_fd};
        args.blist = (int *)malloc(config->room_size * sizeof(int));
        memset(args.blist, -1, config->room_size * sizeof(int));

        // Create threads
        for (int i = 0; i < config->room_size; i++)
                if (pthread_create(&threads[i], NULL, handle_client, (void *)&args) != 0)
                {
                        perror("ERROR: initiate_room > pthread_create | ");
                        return;
                }

        // Block main execution
        for (int i = 0; i < config->room_size; i++)
                if (pthread_join(threads[i], NULL) != 0)
                {
                        perror("ERROR: initiate_room > pthread_join | ");
                        return;
                }
}

void exit_handler(int sig)
{
        printf("INFO: Closing room...\n");

        exit(sig);
}