#include "client.h"
#include "common.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Networking Functionalities */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

void dmes_connect(struct config_params *params)
{
        // Connection info
        struct sockaddr_in server_addr;
        server_addr.sin_port = params->port;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = params->address;

        // Prepare socket
        int serverfd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverfd == -1)
        {
                perror("[ERROR] connect_room > socket ");
                goto exit_normal;
        }

        // TODO: Negotiate security

        // Connect to room
        if (connect(serverfd, (const struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == -1)
        {
                perror("[ERROR] connect_room > connect ");
                goto exit_error;
        }
        
        // I/O Operations
        b_mes net_msg; lpm_t lpm;
        char *msg = NULL;
        char buf[BUF_SIZE];
        size_t read_len, bytes_sent, size = 0;
        
        /* Configure the lpm */
        net_msg.type = DMES_MSG_LPM;
        lpm.port = params->l_port;
        
        /* Active clients need a connection back */
        if (params->type == DMES_CLIENT_A) {
                lpm.repeat = 1;
        }
        /* Passive clients do not need a connection back */
        else if (params->type == DMES_CLIENT_P) {
                lpm.repeat = 0;
        }

        memset(net_msg.msg, 0, sizeof(net_msg.msg));
        memcpy(net_msg.msg, &lpm, sizeof(lpm));

        bytes_sent = send(serverfd, (void *) &net_msg, sizeof(net_msg), 0);

        if (bytes_sent == -1) {
                perror("[ERROR] connect_room > send (1) ");
                goto exit_error;
        }

        printf("[INFO] Connected to client\n");

        /* Main I/O Loop */

        while (true)
        {
                /* Get a message from the console */
                printf("$ ");
                read_len = getline(&msg, &size, stdin);

                /* Evaluate message */

                if (read_len == 0)
                        break;

                // Remove trailing newline character
                msg[read_len - 1] = 0;

                /* Send message to remote */
                bytes_sent = send(serverfd, msg, read_len - 1, 0);

                if (bytes_sent == -1) {
                        perror("[ERROR] connect_room > send (2) ");
                        break;
                }

                /* Check for reply */
                read_len = read(serverfd, buf, BUF_SIZE);
                buf[read_len - 1] = 0;

                // Connection termination
                if (strcmp(buf, "!!") == 0) {
                        close(serverfd);
                        goto exit_normal;
                }
        }

exit_error:
        close(serverfd);
exit_normal:
}