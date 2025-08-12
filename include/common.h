#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct config_params
{
        uint32_t address;   // 32 bit room IP address
        uint16_t port;      // 16 bit port
        uint16_t room_size; // 16 bit number of clients
};

/* Broadcast message from room */
typedef struct
{
        char name[64];    /* Clients name (to be implemented)*/
        int id;           /* Assigned ID */
        char msg[BUFSIZ]; /* Message to broadcast */
} b_mes;

/*
 * Parses & validates user passed arguments.
 *
 * RETURN:
 *      0: Parse failed
 *      1: Room config
 *      2: Client config
 * */
uint8_t parse_args(int argc, char **argv, struct config_params *);

void exit_handler(int);