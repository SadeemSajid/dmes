#ifndef H_COMMON
#define H_COMMON

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

/* APP CONFIGS */
#define ROOM_TYPE 1
#define CLIENT_TYPE 2
#define PORT 1337
#define ROOM_SIZE 24

/**
 * c: active client mode
 * a: address to connect to
 * r: passive client mode
 * p: port to connect to
 * s: active participant size
 * l: port to listen on
 */
#define ARGUMENTS "ca:rp:s:l:"

/* TECHNICAL PARAMS */
#define BUF_SIZE 64

typedef enum {
        DMES_ERROR,
        DMES_CLIENT,
        DMES_CLIENT_A,
        DMES_CLIENT_P,
        DMES_GLUE,
        DMES_NETWORK_REG,
        DMES_GLUE_REG
} app_type;


typedef enum {
        DMES_MSG_STD,
        DMES_MSG_LPM
} dmes_msg_type;

// TODO: Change to typedef
struct config_params {
        uint32_t address;   // 32 bit room IP address
        uint16_t port;      // 16 bit port to connect to
        uint16_t room_size; // 16 bit number of clients
        uint16_t l_port;    // port to listen on
        app_type type;      // application type
};

/* Shared data between threads */
typedef struct {
        uint8_t data[BUF_SIZE];
        int ready;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
} shared_data;


/* Message structure */
typedef struct {
        uint8_t type;     // Type of message
        char msg[BUF_SIZE]; // Message payload
} b_mes;

/* LPM message format */
typedef struct {
        uint16_t port;          // Listener port
        uint8_t repeat;         // Attempt connection to client if set
} lpm_t;

/*
 * Parses & validates user passed arguments.
 *
 * RETURN:
 *      app_type
 * */
uint8_t parse_args(int argc, char **argv, struct config_params *);

void exit_handler(int);

#endif