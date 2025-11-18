#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "network_client.h"

#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)
#define SPEED (300)
#define BULLET_SPEED (500)
#define MAX_HEALTH 100
#define HEALTH_BAR_WIDTH 200
#define HEALTH_BAR_HEIGHT 20

enum GameState {
    MENU,
    SERVER_SELECT,
    PLAYING_SINGLE,
    PLAYING_MULTI,
    GAME_OVER,
    HIGH_SCORE_SHOW,
    HELP,
    OPTIONS,
    QUIT
};

enum GameState currentState = MENU;

int score = 0;
int hscore = 0;
bool soundOn = true;
bool musicOn = true;
NetworkClient netClient;
char serverIP[256] = "127.0.0.1";

Mix_Chunk* sColl1 = NULL;
Mix_Chunk* sColl2 = NULL;
Mix_Chunk* sColl3 = NULL;
Mix_Chunk* sShoot = NULL;

SDL_Texture* load_texture(SDL_Renderer *rend, const char *file) {
    SDL_Surface *surface = IMG_Load(file);
    if (!surface) {
        printf("IMG_Load error: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void render_health_bar(SDL_Renderer *rend, int health, int x, int y) {
    SDL_Rect health_bar = {x, y, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT};
    SDL_Rect health_fill = {x, y, (health * HEALTH_BAR_WIDTH) / MAX_HEALTH, HEALTH_BAR_HEIGHT};

    SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
    SDL_RenderFillRect(rend, &health_fill);
    SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
    SDL_RenderDrawRect(rend, &health_bar);

    TTF_Font *font = TTF_OpenFont("resources/arial.ttf", 20);
    if (font == NULL) return;

    SDL_Color white = {255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, "HP", white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_Rect text_rect = {x, y - 25, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(rend, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

void render_score(SDL_Renderer *renderer, int score) {
    char score_text[50];
    sprintf(score_text, "Score: %d", score);
    
    SDL_Color White = {255, 255, 255};
    TTF_Font* font = TTF_OpenFont("resources/arial.ttf", 24);
    if (!font) return;
    
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, score_text, White);
    SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    
    SDL_Rect Message_rect = {10, 10, 100, 50};
    SDL_RenderCopy(renderer, Message, NULL, &Message_rect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(Message);
    TTF_CloseFont(font);
}

void render_text(SDL_Renderer *rend, const char *text, int x, int y, int size, SDL_Color color) {
    TTF_Font *font = TTF_OpenFont("resources/arial.ttf", size);
    if (!font) return;

    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_Rect text_rect = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(rend, texture, NULL, &text_rect);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

void renderServerSelect(SDL_Renderer* renderer, TTF_Font* font, char* serverIP) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    
    SDL_Surface* titleSurf = TTF_RenderText_Solid(font, "Enter Server IP:", white);
    SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
    SDL_Rect titleRect = {WINDOW_WIDTH/2 - titleSurf->w/2, 200, titleSurf->w, titleSurf->h};
    SDL_RenderCopy(renderer, titleTex, NULL, &titleRect);
    SDL_FreeSurface(titleSurf);
    SDL_DestroyTexture(titleTex);

    char display_ip[300];
    sprintf(display_ip, "> %s_", serverIP);
    SDL_Surface* ipSurf = TTF_RenderText_Solid(font, display_ip, yellow);
    SDL_Texture* ipTex = SDL_CreateTextureFromSurface(renderer, ipSurf);
    SDL_Rect ipRect = {WINDOW_WIDTH/2 - ipSurf->w/2, 300, ipSurf->w, ipSurf->h};
    SDL_RenderCopy(renderer, ipTex, NULL, &ipRect);
    SDL_FreeSurface(ipSurf);
    SDL_DestroyTexture(ipTex);

    SDL_Surface* instSurf = TTF_RenderText_Solid(font, "Press ENTER to connect | ESC to go back", white);
    SDL_Texture* instTex = SDL_CreateTextureFromSurface(renderer, instSurf);
    SDL_Rect instRect = {WINDOW_WIDTH/2 - instSurf->w/2, 400, instSurf->w, instSurf->h};
    SDL_RenderCopy(renderer, instTex, NULL, &instRect);
    SDL_FreeSurface(instSurf);
    SDL_DestroyTexture(instTex);
}

int game_multiplayer(SDL_Window* win, SDL_Renderer* rend) {
    // Load textures
    SDL_Texture *tex = load_texture(rend, "resources/playerplane.png");
    SDL_Texture *bg_tex = load_texture(rend, "resources/background.png");
    SDL_Texture *ej_tex = load_texture(rend, "resources/enemyplane_japan.png");
    SDL_Texture *eg_tex = load_texture(rend, "resources/enemyplane_german.png");
    SDL_Texture *eC_tex = load_texture(rend, "resources/enemyC.png");
    SDL_Texture *eD_tex = load_texture(rend, "resources/enemyD.png");
    SDL_Texture *eE_tex = load_texture(rend, "resources/enemyE.png");
    SDL_Texture *eF_tex = load_texture(rend, "resources/enemyF.png");
    SDL_Texture *bullet_tex = load_texture(rend, "resources/bullet.png");
    SDL_Texture *enemy_bullet_tex = load_texture(rend, "resources/enemy_bullet.png");
    SDL_Texture *explosion_tex = load_texture(rend, "resources/explosion.png");

    SDL_Texture* enemy_textures[] = {ej_tex, eg_tex, eC_tex, eD_tex, eE_tex, eF_tex};
    SDL_Color player_colors[] = {
        {100, 255, 100, 255},  // Green
        {100, 100, 255, 255},  // Blue
        {255, 255, 100, 255},  // Yellow
        {255, 100, 255, 255}   // Magenta
    };

    if (!tex || !bg_tex) {
        printf("Failed to load textures\n");
        return 1;
    }

    int close_requested = 0;
    Uint32 last_frame_time = SDL_GetTicks();

    printf("[CLIENT] Connected to server as Player %d\n", netClient.player_id);

    while (!close_requested && netClient.connected) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_frame_time) / 1000.0f;
        last_frame_time = current_time;

        // Prepare player input
        PlayerInput input = {0};
        input.player_id = netClient.player_id;
        input.timestamp = current_time;

        // Handle input
        SDL_Event event;
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_requested = 1;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                close_requested = 1;
            }
        }

        input.move_up = keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP];
        input.move_down = keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN];
        input.move_left = keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT];
        input.move_right = keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT];
        input.shooting = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT);

        // Send input to server
        client_send_input(&netClient, &input);

        // Receive game state from server
        client_receive_state(&netClient);

        // Render
        SDL_RenderClear(rend);
        SDL_RenderCopy(rend, bg_tex, NULL, NULL);

        // Render all players
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!netClient.game_state.players[i].active) continue;
            if (!netClient.game_state.players[i].alive) continue;

            NetworkPlayer* player = &netClient.game_state.players[i];
            SDL_Rect player_rect = {(int)player->x, (int)player->y, 192, 65};
            
            // Color code players
            SDL_Color pc = player_colors[i];
            SDL_SetTextureColorMod(tex, pc.r, pc.g, pc.b);
            SDL_RenderCopy(rend, tex, NULL, &player_rect);

            // Render player name/id
            char player_label[32];
            sprintf(player_label, "P%d", i);
            SDL_Color label_color = {255, 255, 255, 255};
            if (i == netClient.player_id) {
                strcpy(player_label, "YOU");
                label_color = (SDL_Color){255, 255, 0, 255};
            }
            render_text(rend, player_label, (int)player->x + 70, (int)player->y - 25, 18, label_color);

            // Render mini health bar above each player
            render_health_bar(rend, player->health, (int)player->x, (int)player->y + 70);

            // Render player bullets
            for (int j = 0; j < MAX_BULLETS_PER_PLAYER; j++) {
                if (!player->bullets[j].active) continue;

                SDL_Rect bullet_rect = {
                    (int)player->bullets[j].x, 
                    (int)player->bullets[j].y, 
                    40, 15
                };
                SDL_SetTextureColorMod(bullet_tex, 255, 255, 255);
                SDL_RenderCopy(rend, bullet_tex, NULL, &bullet_rect);
            }
        }

        // Render enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!netClient.game_state.enemies[i].active) continue;

            NetworkEnemy* enemy = &netClient.game_state.enemies[i];
            SDL_Rect enemy_rect = {(int)enemy->x, (int)enemy->y, 192, 65};
            
            int tex_id = enemy->texture_id % 6;
            SDL_SetTextureColorMod(enemy_textures[tex_id], 255, 255, 255);
            SDL_RenderCopy(rend, enemy_textures[tex_id], NULL, &enemy_rect);
        }

        // Render enemy bullets
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!netClient.game_state.enemy_bullets[i].active) continue;

            SDL_Rect eb_rect = {
                (int)netClient.game_state.enemy_bullets[i].x,
                (int)netClient.game_state.enemy_bullets[i].y,
                40, 15
            };
            SDL_SetTextureColorMod(enemy_bullet_tex, 255, 255, 255);
            SDL_RenderCopy(rend, enemy_bullet_tex, NULL, &eb_rect);
        }

        // Render explosions
        for (int i = 0; i < 20; i++) {
            if (!netClient.game_state.explosions[i].active) continue;

            SDL_Rect exp_rect = {
                (int)netClient.game_state.explosions[i].x,
                (int)netClient.game_state.explosions[i].y,
                170, 170
            };
            SDL_SetTextureColorMod(explosion_tex, 255, 255, 255);
            SDL_RenderCopy(rend, explosion_tex, NULL, &exp_rect);
        }

        // Render local player info
        if (netClient.player_id >= 0 && netClient.player_id < MAX_PLAYERS) {
            NetworkPlayer* local = &netClient.game_state.players[netClient.player_id];
            
            if (local->active) {
                render_score(rend, local->score);
                
                // Bullet count
                char bullet_text[32];
                sprintf(bullet_text, "Bullets: %d", 20 - local->bullets_fired);
                render_text(rend, bullet_text, WINDOW_WIDTH - 150, WINDOW_HEIGHT - 50, 20, (SDL_Color){255, 255, 255, 255});
                
                // Reload indicator
                if (local->reloading) {
                    render_text(rend, "RELOADING...", WINDOW_WIDTH - 200, WINDOW_HEIGHT - 90, 24, (SDL_Color){255, 0, 0, 255});
                }

                // Death message
                if (!local->alive) {
                    render_text(rend, "YOU DIED - Respawning...", WINDOW_WIDTH/2 - 200, WINDOW_HEIGHT/2, 36, (SDL_Color){255, 0, 0, 255});
                }
            }
        }

        // Scoreboard
        SDL_Color white = {255, 255, 255, 255};
        render_text(rend, "SCOREBOARD", WINDOW_WIDTH - 200, 10, 20, white);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!netClient.game_state.players[i].active) continue;
            char sb_text[64];
            sprintf(sb_text, "P%d: %d pts", i, netClient.game_state.players[i].score);
            render_text(rend, sb_text, WINDOW_WIDTH - 200, 40 + i * 25, 18, player_colors[i]);
        }

        SDL_RenderPresent(rend);
        SDL_Delay(1000 / 60);

        // Check connection
        if (SDL_GetTicks() - netClient.last_update > 10000) {
            printf("[CLIENT] Lost connection to server\n");
            break;
        }
    }

    // Cleanup
    SDL_DestroyTexture(tex);
    SDL_DestroyTexture(bg_tex);
    SDL_DestroyTexture(ej_tex);
    SDL_DestroyTexture(eg_tex);
    SDL_DestroyTexture(eC_tex);
    SDL_DestroyTexture(eD_tex);
    SDL_DestroyTexture(eE_tex);
    SDL_DestroyTexture(eF_tex);
    SDL_DestroyTexture(bullet_tex);
    SDL_DestroyTexture(enemy_bullet_tex);
    SDL_DestroyTexture(explosion_tex);

    client_disconnect(&netClient);
    currentState = MENU;
    return 0;
}

// Include your existing game() function for single player here
// (Copy from original Main.c)

const char* menuItems[] = {
    "Play Singleplayer",
    "Play Multiplayer",
    "Help",
    "High Scores",
    "Options",
    "Quit"
};
int menuItemCount = 6;
int selectedItem = 0;

void renderMenu(SDL_Renderer* renderer, TTF_Font* font, int selectedItem) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color red = {255, 0, 0, 255};
    
    for (int i = 0; i < menuItemCount; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, menuItems[i], (i == selectedItem) ? red : white);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect dstrect = { 50, 450 + i * 50, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() == -1) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer init failed: %s\n", Mix_GetError());
    }

    // Load sounds
    sColl1 = Mix_LoadWAV("resources/sounds/collision1.wav");
    sColl2 = Mix_LoadWAV("resources/sounds/collision2.wav");
    sColl3 = Mix_LoadWAV("resources/sounds/collision3.wav");
    sShoot = Mix_LoadWAV("resources/sounds/shoot.wav");

    SDL_Window* win = SDL_CreateWindow("Flying Aces: 1942 - Multiplayer",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_SHOWN);
    if (!win) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!rend) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    TTF_Font* font = TTF_OpenFont("resources/arial.ttf", 36);
    if (!font) {
        printf("Font load failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(win);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Texture *menu_tex = load_texture(rend, "resources/menu.jpeg");
    SDL_Texture *menu_title = load_texture(rend, "resources/title.png");

    printf("========================================\n");
    printf("  Flying Aces: 1942 - Multiplayer\n");
    printf("========================================\n");
    printf("Use menu to select game mode.\n");
    printf("For multiplayer: Ensure server is running first.\n\n");

    SDL_Event e;
    bool programquit = false;

    while (!programquit) {
        switch (currentState) {
            case MENU: {
                bool menuquit = false;
                while (!menuquit) {
                    while (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT) {
                            menuquit = true;
                            currentState = QUIT;
                        } else if (e.type == SDL_KEYDOWN) {
                            switch (e.key.keysym.sym) {
                                case SDLK_UP:
                                    selectedItem--;
                                    if (selectedItem < 0) selectedItem = menuItemCount - 1;
                                    break;
                                case SDLK_DOWN:
                                    selectedItem++;
                                    if (selectedItem >= menuItemCount) selectedItem = 0;
                                    break;
                                case SDLK_RETURN:
                                    switch (selectedItem) {
                                        case 0: // Single player
                                            printf("[INFO] Singleplayer not implemented in this version\n");
                                            printf("[INFO] Use original Main.c for singleplayer\n");
                                            break;
                                        case 1: // Multiplayer
                                            currentState = SERVER_SELECT;
                                            menuquit = true;
                                            break;
                                        case 2: // Help
                                            printf("[INFO] Help screen not implemented\n");
                                            break;
                                        case 3: // High Scores
                                            printf("[INFO] High scores not implemented\n");
                                            break;
                                        case 4: // Options
                                            printf("[INFO] Options not implemented\n");
                                            break;
                                        case 5: // Quit
                                            menuquit = true;
                                            currentState = QUIT;
                                            break;
                                    }
                                    break;
                            }
                        }
                    }

                    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
                    SDL_RenderClear(rend);
                    if (menu_tex) SDL_RenderCopy(rend, menu_tex, NULL, NULL);
                    renderMenu(rend, font, selectedItem);
                    SDL_RenderPresent(rend);
                }
                break;
            }

            case SERVER_SELECT: {
                bool selectquit = false;
                while (!selectquit) {
                    while (SDL_PollEvent(&e)) {
                        if (e.type == SDL_QUIT) {
                            selectquit = true;
                            currentState = QUIT;
                        } else if (e.type == SDL_KEYDOWN) {
                            if (e.key.keysym.sym == SDLK_RETURN) {
                                printf("[CLIENT] Connecting to %s:%d...\n", serverIP, SERVER_PORT);
                                
                                if (client_init(&netClient, serverIP, SERVER_PORT)) {
                                    if (client_connect(&netClient)) {
                                        printf("[CLIENT] Successfully connected!\n");
                                        currentState = PLAYING_MULTI;
                                        selectquit = true;
                                    } else {
                                        printf("[CLIENT] Connection failed!\n");
                                        client_cleanup(&netClient);
                                    }
                                } else {
                                    printf("[CLIENT] Network init failed!\n");
                                }
                            } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                                currentState = MENU;
                                selectquit = true;
                                selectedItem = 0;
                            } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                                int len = strlen(serverIP);
                                if (len > 0) serverIP[len - 1] = '\0';
                            } else if (e.type == SDL_TEXTINPUT) {
                                if (strlen(serverIP) < 255) {
                                    strcat(serverIP, e.text.text);
                                }
                            }
                        } else if (e.type == SDL_TEXTINPUT) {
                            if (strlen(serverIP) < 255) {
                                strcat(serverIP, e.text.text);
                            }
                        }
                    }

                    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
                    SDL_RenderClear(rend);
                    renderServerSelect(rend, font, serverIP);
                    SDL_RenderPresent(rend);
                }
                break;
            }

            case PLAYING_MULTI:
                game_multiplayer(win, rend);
                currentState = MENU;
                selectedItem = 0;
                break;

            case QUIT:
                programquit = true;
                break;

            default:
                currentState = MENU;
                break;
        }
    }

    // Cleanup
    if (sColl1) Mix_FreeChunk(sColl1);
    if (sColl2) Mix_FreeChunk(sColl2);
    if (sColl3) Mix_FreeChunk(sColl3);
    if (sShoot) Mix_FreeChunk(sShoot);
    Mix_CloseAudio();
    
    if (menu_tex) SDL_DestroyTexture(menu_tex);
    if (menu_title) SDL_DestroyTexture(menu_title);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    
    printf("\n[EXIT] Game closed.\n");
    return 0;
}