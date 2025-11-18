#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <SDL2/SDL_net.h>
#include "network_common.h"

typedef struct {
    UDPsocket socket;
    UDPpacket *packet;
    IPaddress server_address;
    int player_id;
    int connected;
    GameState game_state;
    Uint32 last_update;
} NetworkClient;

/**
 * Initialize network client
 * 
 * @param client Pointer to NetworkClient structure
 * @param host Server IP address or hostname
 * @param port Server port number
 * @return 1 on success, 0 on failure
 */
int client_init(NetworkClient *client, const char *host, int port);

/**
 * Connect to server
 * Sends connection request and waits for response
 * 
 * @param client Pointer to initialized NetworkClient
 * @return 1 on success, 0 on failure
 */
int client_connect(NetworkClient *client);

/**
 * Send player input to server
 * Called every frame with current input state
 * 
 * @param client Pointer to connected NetworkClient
 * @param input Pointer to PlayerInput structure with current controls
 */
void client_send_input(NetworkClient *client, PlayerInput *input);

/**
 * Receive game state from server
 * Should be called every frame to get latest game state
 * 
 * @param client Pointer to connected NetworkClient
 * @return 1 if new state received, 0 otherwise
 */
int client_receive_state(NetworkClient *client);

/**
 * Disconnect from server
 * Sends disconnect notification and closes connection
 * 
 * @param client Pointer to connected NetworkClient
 */
void client_disconnect(NetworkClient *client);

/**
 * Cleanup and free network resources
 * Should be called before program exit
 * 
 * @param client Pointer to NetworkClient
 */
void client_cleanup(NetworkClient *client);

/**
 * Check if client is still connected
 * Includes timeout detection
 * 
 * @param client Pointer to NetworkClient
 * @return 1 if connected, 0 if disconnected or timed out
 */
int client_is_connected(NetworkClient *client);

/**
 * Get estimated network latency
 * Simple estimate based on update timing
 * 
 * @param client Pointer to NetworkClient
 * @return Estimated latency in milliseconds
 */
Uint32 client_get_latency(NetworkClient *client);

/**
 * Print client statistics for debugging
 * 
 * @param client Pointer to NetworkClient
 */
void client_print_stats(NetworkClient *client);

#endif // NETWORK_CLIENT_H