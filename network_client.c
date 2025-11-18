#include <stdio.h>
#include <string.h>
#include <SDL2/SDL_net.h>
#include "network_client.h"

int client_init(NetworkClient *client, const char *host, int port) {
    if (SDLNet_Init() < 0) {
        printf("[CLIENT ERROR] SDLNet_Init failed: %s\n", SDLNet_GetError());
        return 0;
    }

    // Open UDP socket on any available port
    client->socket = SDLNet_UDP_Open(0);
    if (!client->socket) {
        printf("[CLIENT ERROR] SDLNet_UDP_Open failed: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        return 0;
    }

    // Allocate packet buffer
    client->packet = SDLNet_AllocPacket(MAX_PACKET_SIZE);
    if (!client->packet) {
        printf("[CLIENT ERROR] SDLNet_AllocPacket failed: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(client->socket);
        SDLNet_Quit();
        return 0;
    }

    // Resolve server address
    if (SDLNet_ResolveHost(&client->server_address, host, port) < 0) {
        printf("[CLIENT ERROR] SDLNet_ResolveHost failed for %s:%d - %s\n", 
               host, port, SDLNet_GetError());
        SDLNet_FreePacket(client->packet);
        SDLNet_UDP_Close(client->socket);
        SDLNet_Quit();
        return 0;
    }

    // Initialize client state
    client->connected = 0;
    client->player_id = -1;
    memset(&client->game_state, 0, sizeof(GameState));
    client->last_update = SDL_GetTicks();

    printf("[CLIENT] Network initialized successfully\n");
    printf("[CLIENT] Target server: %s:%d\n", host, port);

    return 1;
}

int client_connect(NetworkClient *client) {
    if (!client->socket || !client->packet) {
        printf("[CLIENT ERROR] Client not initialized\n");
        return 0;
    }

    printf("[CLIENT] Attempting to connect to server...\n");

    // Prepare connection packet
    ConnectPacket connect_pkt;
    connect_pkt.header.type = PACKET_CONNECT;
    connect_pkt.header.player_id = -1;
    connect_pkt.header.sequence = 0;
    strncpy(connect_pkt.player_name, "Player", sizeof(connect_pkt.player_name) - 1);
    connect_pkt.player_name[sizeof(connect_pkt.player_name) - 1] = '\0';

    // Copy packet data
    memcpy(client->packet->data, &connect_pkt, sizeof(ConnectPacket));
    client->packet->len = sizeof(ConnectPacket);
    client->packet->address = client->server_address;

    // Send connection request
    if (!SDLNet_UDP_Send(client->socket, -1, client->packet)) {
        printf("[CLIENT ERROR] Failed to send connect packet: %s\n", SDLNet_GetError());
        return 0;
    }

    printf("[CLIENT] Connection request sent, waiting for response...\n");

    // Wait for response with timeout
    Uint32 start_time = SDL_GetTicks();
    int retry_count = 0;
    const int MAX_RETRIES = 3;
    const Uint32 RETRY_INTERVAL = 1000; // 1 second between retries
    const Uint32 TOTAL_TIMEOUT = 5000;  // 5 second total timeout

    while (SDL_GetTicks() - start_time < TOTAL_TIMEOUT) {
        // Retry sending if no response after interval
        if (retry_count < MAX_RETRIES && 
            SDL_GetTicks() - start_time > (retry_count + 1) * RETRY_INTERVAL) {
            printf("[CLIENT] Retry attempt %d/%d...\n", retry_count + 1, MAX_RETRIES);
            SDLNet_UDP_Send(client->socket, -1, client->packet);
            retry_count++;
        }

        // Check for response
        if (SDLNet_UDP_Recv(client->socket, client->packet)) {
            PacketHeader *header = (PacketHeader *)client->packet->data;
            
            if (header->type == PACKET_CONNECT) {
                ConnectResponse *response = (ConnectResponse *)client->packet->data;
                
                if (response->success) {
                    client->player_id = response->assigned_id;
                    client->connected = 1;
                    client->last_update = SDL_GetTicks();
                    
                    printf("[CLIENT SUCCESS] Connected to server!\n");
                    printf("[CLIENT] Assigned Player ID: %d\n", client->player_id);
                    return 1;
                } else {
                    printf("[CLIENT ERROR] Server rejected connection (server full?)\n");
                    return 0;
                }
            }
        }
        
        SDL_Delay(10); // Small delay to prevent CPU spinning
    }

    printf("[CLIENT ERROR] Connection timeout (no response from server)\n");
    printf("[CLIENT] Possible issues:\n");
    printf("  - Server not running\n");
    printf("  - Incorrect IP address\n");
    printf("  - Firewall blocking UDP port %d\n", SERVER_PORT);
    printf("  - Network connectivity issues\n");
    
    return 0;
}

void client_send_input(NetworkClient *client, PlayerInput *input) {
    if (!client->connected || !client->socket || !client->packet) {
        return;
    }

    // Prepare input packet
    InputPacket input_pkt;
    input_pkt.header.type = PACKET_INPUT;
    input_pkt.header.player_id = client->player_id;
    input_pkt.header.sequence = SDL_GetTicks();
    input_pkt.input = *input;
    input_pkt.input.player_id = client->player_id; // Ensure consistency

    // Copy packet data
    memcpy(client->packet->data, &input_pkt, sizeof(InputPacket));
    client->packet->len = sizeof(InputPacket);
    client->packet->address = client->server_address;

    // Send input (fire and forget - UDP)
    if (!SDLNet_UDP_Send(client->socket, -1, client->packet)) {
        printf("[CLIENT WARNING] Failed to send input packet: %s\n", SDLNet_GetError());
    }
}

int client_receive_state(NetworkClient *client) {
    if (!client->connected || !client->socket || !client->packet) {
        return 0;
    }

    int received = 0;
    int packets_this_frame = 0;
    
    // Process all available packets (drain the receive buffer)
    while (SDLNet_UDP_Recv(client->socket, client->packet)) {
        PacketHeader *header = (PacketHeader *)client->packet->data;
        
        switch (header->type) {
            case PACKET_GAME_STATE: {
                GameStatePacket *state_pkt = (GameStatePacket *)client->packet->data;
                
                // Update game state
                client->game_state = state_pkt->state;
                client->last_update = SDL_GetTicks();
                received = 1;
                packets_this_frame++;
                break;
            }
            
            case PACKET_DISCONNECT: {
                printf("[CLIENT] Server requested disconnect\n");
                client->connected = 0;
                return 0;
            }
            
            default:
                // Unknown packet type, ignore
                break;
        }
    }

    // Debug: Log if we received multiple state packets in one frame
    if (packets_this_frame > 1) {
        // This is normal for UDP - we might receive multiple buffered packets
        // Just use the latest one
    }

    // Check for connection timeout
    Uint32 time_since_update = SDL_GetTicks() - client->last_update;
    if (received == 0 && time_since_update > 10000) {
        printf("[CLIENT WARNING] No packets received for %u ms\n", time_since_update);
    }

    return received;
}

void client_disconnect(NetworkClient *client) {
    if (!client->connected) {
        return;
    }

    printf("[CLIENT] Disconnecting from server...\n");

    // Send disconnect packet
    PacketHeader disconnect_pkt;
    disconnect_pkt.type = PACKET_DISCONNECT;
    disconnect_pkt.player_id = client->player_id;
    disconnect_pkt.sequence = SDL_GetTicks();

    memcpy(client->packet->data, &disconnect_pkt, sizeof(PacketHeader));
    client->packet->len = sizeof(PacketHeader);
    client->packet->address = client->server_address;

    // Send disconnect notification (try a few times)
    for (int i = 0; i < 3; i++) {
        SDLNet_UDP_Send(client->socket, -1, client->packet);
        SDL_Delay(10);
    }

    client->connected = 0;
    client->player_id = -1;

    printf("[CLIENT] Disconnected\n");
}

void client_cleanup(NetworkClient *client) {
    printf("[CLIENT] Cleaning up network resources...\n");

    // Disconnect if still connected
    if (client->connected) {
        client_disconnect(client);
    }

    // Free packet
    if (client->packet) {
        SDLNet_FreePacket(client->packet);
        client->packet = NULL;
    }

    // Close socket
    if (client->socket) {
        SDLNet_UDP_Close(client->socket);
        client->socket = NULL;
    }

    // Shutdown SDL_net
    SDLNet_Quit();

    printf("[CLIENT] Cleanup complete\n");
}

// Utility function to check connection status
int client_is_connected(NetworkClient *client) {
    if (!client->connected) {
        return 0;
    }

    // Check if we've received updates recently
    Uint32 time_since_update = SDL_GetTicks() - client->last_update;
    if (time_since_update > 10000) {
        printf("[CLIENT] Connection lost (timeout)\n");
        client->connected = 0;
        return 0;
    }

    return 1;
}

// Utility function to get latency estimate
Uint32 client_get_latency(NetworkClient *client) {
    if (!client->connected) {
        return 9999;
    }
    
    // Simple estimate: time since last update
    // In a more sophisticated implementation, you'd do proper RTT measurement
    Uint32 time_since_update = SDL_GetTicks() - client->last_update;
    
    // Assume server sends at 30Hz, so max normal delay is ~33ms
    // Anything beyond that is network latency
    if (time_since_update > 33) {
        return time_since_update - 33;
    }
    
    return 0;
}

// Utility function for debugging
void client_print_stats(NetworkClient *client) {
    printf("\n[CLIENT STATS]\n");
    printf("  Connected: %s\n", client->connected ? "Yes" : "No");
    printf("  Player ID: %d\n", client->player_id);
    printf("  Last Update: %u ms ago\n", SDL_GetTicks() - client->last_update);
    printf("  Active Players: %d\n", client->game_state.player_count);
    printf("  Enemies: %d\n", client->game_state.enemy_count);
    printf("  Enemy Bullets: %d\n", client->game_state.enemy_bullet_count);
    printf("  Server Tick: %u\n", client->game_state.tick);
    
    if (client->player_id >= 0 && client->player_id < MAX_PLAYERS) {
        NetworkPlayer *p = &client->game_state.players[client->player_id];
        if (p->active) {
            printf("  Your Score: %d\n", p->score);
            printf("  Your Health: %d\n", p->health);
            printf("  Your Status: %s\n", p->alive ? "Alive" : "Dead");
        }
    }
    printf("\n");
}