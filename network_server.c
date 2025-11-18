#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include "network_common.h"

#define SPEED 300
#define BULLET_SPEED 500
#define ENEMY_SPEED 300
#define ENEMY_BULLET_SPEED 400
#define ENEMY_SPAWN_INTERVAL 2000
#define ENEMY_SHOOT_INTERVAL 1500
#define PLAYER_SHOOT_INTERVAL 200
#define MAX_BULLETS_BEFORE_RELOAD 20
#define RELOAD_TIME 2000
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define RESPAWN_TIME 3000
#define PLAYER_WIDTH 192
#define PLAYER_HEIGHT 65
#define ENEMY_WIDTH 192
#define ENEMY_HEIGHT 65
#define BULLET_WIDTH 40
#define BULLET_HEIGHT 15

typedef struct {
    IPaddress address;
    int active;
    Uint32 last_heard;
} ClientInfo;

typedef struct {
    UDPsocket socket;
    UDPpacket *packet;
    ClientInfo clients[MAX_PLAYERS];
    GameState game_state;
    Uint32 last_enemy_spawn;
    Uint32 last_enemy_shoot;
    int running;
    Uint32 sequence;
} Server;

Server server;

int check_collision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return !(x1 + w1 <= x2 || x1 >= x2 + w2 || y1 + h1 <= y2 || y1 >= y2 + h2);
}

void add_explosion(float x, float y) {
    for (int i = 0; i < 20; i++) {
        if (!server.game_state.explosions[i].active) {
            server.game_state.explosions[i].active = 1;
            server.game_state.explosions[i].x = x;
            server.game_state.explosions[i].y = y;
            server.game_state.explosions[i].start_time = SDL_GetTicks();
            break;
        }
    }
}

void init_server() {
    if (SDLNet_Init() < 0) {
        printf("SDLNet_Init failed: %s\n", SDLNet_GetError());
        exit(1);
    }

    server.socket = SDLNet_UDP_Open(SERVER_PORT);
    if (!server.socket) {
        printf("SDLNet_UDP_Open failed: %s\n", SDLNet_GetError());
        exit(1);
    }

    server.packet = SDLNet_AllocPacket(MAX_PACKET_SIZE);
    if (!server.packet) {
        printf("SDLNet_AllocPacket failed: %s\n", SDLNet_GetError());
        exit(1);
    }

    // Initialize game state
    memset(&server.game_state, 0, sizeof(GameState));
    memset(server.clients, 0, sizeof(server.clients));
    
    server.running = 1;
    server.sequence = 0;
    server.last_enemy_spawn = 0;
    server.last_enemy_shoot = 0;

    printf("========================================\n");
    printf("  Flying Aces: 1942 - Server Started\n");
    printf("========================================\n");
    printf("Port: %d\n", SERVER_PORT);
    printf("Max Players: %d\n", MAX_PLAYERS);
    printf("Tick Rate: %d Hz\n", TICK_RATE);
    printf("\nWaiting for players...\n");
}

int find_free_player_slot() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!server.clients[i].active) {
            return i;
        }
    }
    return -1;
}

void handle_connect(IPaddress *addr) {
    int slot = find_free_player_slot();
    if (slot < 0) {
        printf("Server full, rejecting connection\n");
        
        ConnectResponse response;
        response.header.type = PACKET_CONNECT;
        response.header.player_id = -1;
        response.header.sequence = server.sequence++;
        response.assigned_id = -1;
        response.success = 0;

        memcpy(server.packet->data, &response, sizeof(ConnectResponse));
        server.packet->len = sizeof(ConnectResponse);
        server.packet->address = *addr;
        SDLNet_UDP_Send(server.socket, -1, server.packet);
        return;
    }

    server.clients[slot].address = *addr;
    server.clients[slot].active = 1;
    server.clients[slot].last_heard = SDL_GetTicks();

    // Initialize player
    NetworkPlayer *player = &server.game_state.players[slot];
    player->id = slot;
    player->x = 100 + slot * 150;
    player->y = (WINDOW_HEIGHT / 2) + (slot * 50) - 100;
    player->health = 100;
    player->score = 0;
    player->active = 1;
    player->alive = 1;
    player->bullet_count = 0;
    player->bullets_fired = 0;
    player->reloading = 0;
    player->last_shoot_time = 0;
    player->respawn_time = 0;
    memset(player->bullets, 0, sizeof(player->bullets));

    server.game_state.player_count++;

    // Send connection response
    ConnectResponse response;
    response.header.type = PACKET_CONNECT;
    response.header.player_id = slot;
    response.header.sequence = server.sequence++;
    response.assigned_id = slot;
    response.success = 1;

    memcpy(server.packet->data, &response, sizeof(ConnectResponse));
    server.packet->len = sizeof(ConnectResponse);
    server.packet->address = *addr;
    SDLNet_UDP_Send(server.socket, -1, server.packet);

    printf("[+] Player %d connected (Total: %d/%d)\n", slot, server.game_state.player_count, MAX_PLAYERS);
}

void handle_disconnect(int player_id) {
    if (player_id < 0 || player_id >= MAX_PLAYERS || !server.clients[player_id].active) {
        return;
    }

    server.clients[player_id].active = 0;
    server.game_state.players[player_id].active = 0;
    server.game_state.players[player_id].alive = 0;
    server.game_state.player_count--;
    printf("[-] Player %d disconnected (Total: %d/%d)\n", player_id, server.game_state.player_count, MAX_PLAYERS);
}

void process_player_input(int player_id, PlayerInput *input, float delta_time) {
    if (player_id < 0 || player_id >= MAX_PLAYERS) return;
    if (!server.game_state.players[player_id].active) return;
    if (!server.game_state.players[player_id].alive) return;

    NetworkPlayer *player = &server.game_state.players[player_id];
    Uint32 current_time = SDL_GetTicks();

    // Update position
    float x_vel = 0, y_vel = 0;
    if (input->move_up && !input->move_down) y_vel = -SPEED;
    if (input->move_down && !input->move_up) y_vel = SPEED;
    if (input->move_left && !input->move_right) x_vel = -SPEED;
    if (input->move_right && !input->move_left) x_vel = SPEED;

    player->x += x_vel * delta_time;
    player->y += y_vel * delta_time;

    // Clamp position
    if (player->x < 0) player->x = 0;
    if (player->y < 0) player->y = 0;
    if (player->x > WINDOW_WIDTH - PLAYER_WIDTH) player->x = WINDOW_WIDTH - PLAYER_WIDTH;
    if (player->y > WINDOW_HEIGHT - PLAYER_HEIGHT) player->y = WINDOW_HEIGHT - PLAYER_HEIGHT;

    // Handle shooting with rate limiting
    if (input->shooting && !player->reloading && 
        player->bullets_fired < MAX_BULLETS_BEFORE_RELOAD &&
        current_time >= player->last_shoot_time + PLAYER_SHOOT_INTERVAL) {
        
        // Find free bullet slot
        for (int i = 0; i < MAX_BULLETS_PER_PLAYER; i++) {
            if (!player->bullets[i].active) {
                player->bullets[i].active = 1;
                player->bullets[i].x = player->x + PLAYER_WIDTH;
                player->bullets[i].y = player->y + (PLAYER_HEIGHT / 2);
                player->bullets[i].vx = BULLET_SPEED;
                player->bullets[i].vy = 0;
                player->bullets_fired++;
                player->last_shoot_time = current_time;
                
                if (player->bullets_fired >= MAX_BULLETS_BEFORE_RELOAD) {
                    player->reloading = 1;
                    player->reload_start_time = current_time;
                }
                break;
            }
        }
    }
}

void update_game_state(float delta_time) {
    Uint32 current_time = SDL_GetTicks();

    // Update players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!server.game_state.players[i].active) continue;
        
        NetworkPlayer *player = &server.game_state.players[i];

        // Handle respawn
        if (!player->alive && player->respawn_time > 0 && current_time >= player->respawn_time) {
            player->alive = 1;
            player->health = 100;
            player->x = 100 + i * 150;
            player->y = (WINDOW_HEIGHT / 2) + (i * 50) - 100;
            player->bullets_fired = 0;
            player->reloading = 0;
            player->respawn_time = 0;
            printf("[RESPAWN] Player %d respawned\n", i);
        }

        if (!player->alive) continue;

        // Handle reloading
        if (player->reloading && current_time >= player->reload_start_time + RELOAD_TIME) {
            player->reloading = 0;
            player->bullets_fired = 0;
        }

        // Update player bullets
        for (int j = 0; j < MAX_BULLETS_PER_PLAYER; j++) {
            if (!player->bullets[j].active) continue;

            player->bullets[j].x += player->bullets[j].vx * delta_time;
            player->bullets[j].y += player->bullets[j].vy * delta_time;

            // Remove off-screen bullets
            if (player->bullets[j].x > WINDOW_WIDTH || player->bullets[j].x < 0 ||
                player->bullets[j].y > WINDOW_HEIGHT || player->bullets[j].y < 0) {
                player->bullets[j].active = 0;
            }
        }
    }

    // Spawn enemies
    if (current_time > server.last_enemy_spawn + ENEMY_SPAWN_INTERVAL &&
        server.game_state.enemy_count < MAX_ENEMIES) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!server.game_state.enemies[i].active) {
                server.game_state.enemies[i].active = 1;
                server.game_state.enemies[i].x = WINDOW_WIDTH;
                server.game_state.enemies[i].y = rand() % (WINDOW_HEIGHT - 250);
                server.game_state.enemies[i].texture_id = rand() % 6;
                server.game_state.enemies[i].health = 1;
                server.game_state.enemy_count++;
                server.last_enemy_spawn = current_time;
                break;
            }
        }
    }

    // Update enemies and enemy shooting
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!server.game_state.enemies[i].active) continue;

        server.game_state.enemies[i].x -= ENEMY_SPEED * delta_time;

        // Enemy shooting
        if (current_time > server.last_enemy_shoot + ENEMY_SHOOT_INTERVAL &&
            server.game_state.enemy_bullet_count < MAX_ENEMY_BULLETS) {
            for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                if (!server.game_state.enemy_bullets[j].active) {
                    server.game_state.enemy_bullets[j].active = 1;
                    server.game_state.enemy_bullets[j].x = server.game_state.enemies[i].x;
                    server.game_state.enemy_bullets[j].y = server.game_state.enemies[i].y + (ENEMY_HEIGHT / 2);
                    server.game_state.enemy_bullets[j].vx = -ENEMY_BULLET_SPEED;
                    server.game_state.enemy_bullets[j].vy = 0;
                    server.game_state.enemy_bullet_count++;
                    server.last_enemy_shoot = current_time;
                    break;
                }
            }
        }

        // Remove off-screen enemies
        if (server.game_state.enemies[i].x < -200) {
            server.game_state.enemies[i].active = 0;
            server.game_state.enemy_count--;
        }
    }

    // Update enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!server.game_state.enemy_bullets[i].active) continue;

        server.game_state.enemy_bullets[i].x += server.game_state.enemy_bullets[i].vx * delta_time;
        server.game_state.enemy_bullets[i].y += server.game_state.enemy_bullets[i].vy * delta_time;

        // Remove off-screen bullets
        if (server.game_state.enemy_bullets[i].x < -50 || server.game_state.enemy_bullets[i].x > WINDOW_WIDTH + 50 ||
            server.game_state.enemy_bullets[i].y < -50 || server.game_state.enemy_bullets[i].y > WINDOW_HEIGHT + 50) {
            server.game_state.enemy_bullets[i].active = 0;
            server.game_state.enemy_bullet_count--;
        }
    }

    // Update explosions
    for (int i = 0; i < 20; i++) {
        if (server.game_state.explosions[i].active && 
            current_time - server.game_state.explosions[i].start_time > 500) {
            server.game_state.explosions[i].active = 0;
        }
    }

    // Check collisions: Player bullets vs Enemies
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (!server.game_state.players[p].active || !server.game_state.players[p].alive) continue;
        NetworkPlayer *player = &server.game_state.players[p];

        for (int b = 0; b < MAX_BULLETS_PER_PLAYER; b++) {
            if (!player->bullets[b].active) continue;

            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!server.game_state.enemies[e].active) continue;

                if (check_collision(player->bullets[b].x, player->bullets[b].y, BULLET_WIDTH, BULLET_HEIGHT,
                                  server.game_state.enemies[e].x, server.game_state.enemies[e].y, 
                                  ENEMY_WIDTH, ENEMY_HEIGHT)) {
                    player->bullets[b].active = 0;
                    server.game_state.enemies[e].active = 0;
                    server.game_state.enemy_count--;
                    player->score += 10;
                    add_explosion(server.game_state.enemies[e].x, server.game_state.enemies[e].y);
                }
            }
        }
    }

    // Check collisions: Players vs Enemies (ram damage)
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (!server.game_state.players[p].active || !server.game_state.players[p].alive) continue;
        NetworkPlayer *player = &server.game_state.players[p];

        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!server.game_state.enemies[e].active) continue;

            if (check_collision(player->x, player->y, PLAYER_WIDTH, PLAYER_HEIGHT,
                              server.game_state.enemies[e].x, server.game_state.enemies[e].y, 
                              ENEMY_WIDTH, ENEMY_HEIGHT)) {
                player->health -= 20;
                player->score += 10;
                server.game_state.enemies[e].active = 0;
                server.game_state.enemy_count--;
                add_explosion(server.game_state.enemies[e].x, server.game_state.enemies[e].y);

                if (player->health <= 0) {
                    player->alive = 0;
                    player->health = 0;
                    player->respawn_time = current_time + RESPAWN_TIME;
                    add_explosion(player->x, player->y);
                    printf("[DEATH] Player %d killed by collision (Score: %d)\n", p, player->score);
                }
            }
        }
    }

    // Check collisions: Enemy bullets vs Players
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!server.game_state.enemy_bullets[i].active) continue;

        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (!server.game_state.players[p].active || !server.game_state.players[p].alive) continue;
            NetworkPlayer *player = &server.game_state.players[p];

            if (check_collision(server.game_state.enemy_bullets[i].x, server.game_state.enemy_bullets[i].y, 
                              BULLET_WIDTH, BULLET_HEIGHT,
                              player->x, player->y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                server.game_state.enemy_bullets[i].active = 0;
                server.game_state.enemy_bullet_count--;
                player->health -= 10;

                if (player->health <= 0) {
                    player->alive = 0;
                    player->health = 0;
                    player->respawn_time = current_time + RESPAWN_TIME;
                    add_explosion(player->x, player->y);
                    printf("[DEATH] Player %d killed by enemy fire (Score: %d)\n", p, player->score);
                }
            }
        }
    }

    server.game_state.tick++;
}

void send_game_state() {
    GameStatePacket pkt;
    pkt.header.type = PACKET_GAME_STATE;
    pkt.header.sequence = server.sequence++;
    pkt.state = server.game_state;

    memcpy(server.packet->data, &pkt, sizeof(GameStatePacket));
    server.packet->len = sizeof(GameStatePacket);

    // Send to all active clients
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server.clients[i].active) {
            server.packet->address = server.clients[i].address;
            SDLNet_UDP_Send(server.socket, -1, server.packet);
        }
    }
}

void receive_packets() {
    while (SDLNet_UDP_Recv(server.socket, server.packet)) {
        PacketHeader *header = (PacketHeader *)server.packet->data;

        switch (header->type) {
            case PACKET_CONNECT:
                handle_connect(&server.packet->address);
                break;

            case PACKET_INPUT: {
                InputPacket *input_pkt = (InputPacket *)server.packet->data;
                int pid = input_pkt->header.player_id;
                if (pid >= 0 && pid < MAX_PLAYERS && server.clients[pid].active) {
                    server.clients[pid].last_heard = SDL_GetTicks();
                    process_player_input(pid, &input_pkt->input, 1.0f / TICK_RATE);
                }
                break;
            }

            case PACKET_DISCONNECT:
                handle_disconnect(header->player_id);
                break;

            default:
                break;
        }
    }
}

void check_timeouts() {
    Uint32 current_time = SDL_GetTicks();
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server.clients[i].active && 
            current_time - server.clients[i].last_heard > 10000) {
            printf("[TIMEOUT] Player %d timed out\n", i);
            handle_disconnect(i);
        }
    }
}

void print_stats() {
    static Uint32 last_print = 0;
    Uint32 current = SDL_GetTicks();
    
    if (current - last_print > 5000) {
        printf("\n[STATS] Tick: %u | Players: %d | Enemies: %d | Enemy Bullets: %d\n", 
               server.game_state.tick, 
               server.game_state.player_count,
               server.game_state.enemy_count,
               server.game_state.enemy_bullet_count);
        
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (server.game_state.players[i].active) {
                printf("  Player %d: Score=%d HP=%d %s\n", 
                       i, 
                       server.game_state.players[i].score,
                       server.game_state.players[i].health,
                       server.game_state.players[i].alive ? "ALIVE" : "DEAD");
            }
        }
        last_print = current;
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    if (SDL_Init(0) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    init_server();

    Uint32 last_time = SDL_GetTicks();
    Uint32 tick_interval = 1000 / TICK_RATE;

    printf("\n[SERVER READY] Waiting for connections...\n");
    printf("Press Ctrl+C to stop.\n\n");

    while (server.running) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        if (delta_time > 0.1f) delta_time = 0.1f; // Cap delta

        receive_packets();
        update_game_state(delta_time);
        send_game_state();
        check_timeouts();
        print_stats();

        last_time = current_time;
        
        Uint32 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < tick_interval) {
            SDL_Delay(tick_interval - frame_time);
        }
    }

    printf("\n[SHUTDOWN] Server closing...\n");
    SDLNet_FreePacket(server.packet);
    SDLNet_UDP_Close(server.socket);
    SDLNet_Quit();
    SDL_Quit();

    return 0;
}