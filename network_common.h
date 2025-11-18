#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H
#include <SDL2/SDL_net.h>

#define MAX_PLAYERS 4
#define MAX_BULLETS_PER_PLAYER 100
#define MAX_ENEMIES 10
#define MAX_ENEMY_BULLETS 50
#define MAX_PACKET_SIZE 8192
#define SERVER_PORT 9999
#define TICK_RATE 30  // Updates per second

// Player input structure
typedef struct {
    int player_id;
    int move_up;
    int move_down;
    int move_left;
    int move_right;
    int shooting;
    Uint32 timestamp;
} PlayerInput;

// Bullet structure for network
typedef struct {
    float x, y;
    float vx, vy;
    int active;
} NetworkBullet;

// Player structure for network
typedef struct {
    int id;
    float x, y;
    int health;
    int score;
    int active;
    int alive;
    int bullet_count;
    NetworkBullet bullets[MAX_BULLETS_PER_PLAYER];
    int bullets_fired;
    int reloading;
    Uint32 reload_start_time;
    Uint32 last_shoot_time;
    Uint32 respawn_time;
} NetworkPlayer;

// Enemy structure for network
typedef struct {
    float x, y;
    int texture_id;  // 0-5 for different enemy textures
    int active;
    int health;
} NetworkEnemy;

// Enemy bullet structure
typedef struct {
    float x, y;
    float vx, vy;
    int active;
} NetworkEnemyBullet;

// Explosion effect
typedef struct {
    float x, y;
    int active;
    Uint32 start_time;
} NetworkExplosion;

// Complete game state
typedef struct {
    NetworkPlayer players[MAX_PLAYERS];
    NetworkEnemy enemies[MAX_ENEMIES];
    NetworkEnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];
    NetworkExplosion explosions[20];
    int player_count;
    int enemy_count;
    int enemy_bullet_count;
    Uint32 tick;
} GameState;

// Packet types
typedef enum {
    PACKET_CONNECT,
    PACKET_DISCONNECT,
    PACKET_INPUT,
    PACKET_GAME_STATE,
    PACKET_PING
} PacketType;

// Network packet header
typedef struct {
    PacketType type;
    int player_id;
    Uint32 sequence;
} PacketHeader;

// Connect request
typedef struct {
    PacketHeader header;
    char player_name[32];
} ConnectPacket;

// Connect response
typedef struct {
    PacketHeader header;
    int assigned_id;
    int success;
} ConnectResponse;

// Input packet
typedef struct {
    PacketHeader header;
    PlayerInput input;
} InputPacket;

// Game state packet
typedef struct {
    PacketHeader header;
    GameState state;
} GameStatePacket;

#endif // NETWORK_COMMON_H