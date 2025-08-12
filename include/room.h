/*
 * Makes a server and returns its file descriptor.
 * PARAMS
 *      @config: server configuration parameters from the user.
 *               Accepts struct config_params address.
 * RETURN
 *      fd: Server file descriptor if successful
 *      -1: Error, errno will be set.
 * */
int make_server(struct config_params *);

/*
 * Handles a client connection to the room.
 * PARAMS
 *      @arg: Data needed for the client
 * */
void *handle_client(void *);

/*
 * Handles received messages from room in a client

*/
void *handle_recv(void *);

/*
 * Starts a room
 * */
void initiate_room(struct config_params *, int, int *, int *);