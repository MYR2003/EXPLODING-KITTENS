#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include "funciones.h"

// ==================== GLOBAL STRUCTURES ====================
#define MAX_GAMES 100
#define MAX_CLIENTS_PER_GAME 5

typedef struct {
    int client_socket;
    int player_id;
    int game_id;
    Player * player;
} ClientInfo;

GameState * games[MAX_GAMES];
int game_count = 0;
int lamport_clock = 0;  
pthread_mutex_t games_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamport_lock = PTHREAD_MUTEX_INITIALIZER;

// ==================== LAMPORT CLOCK ====================
int GetLamportClock() {
    pthread_mutex_lock(&lamport_lock);
    int current = lamport_clock++;
    pthread_mutex_unlock(&lamport_lock);
    return current;
}

// ==================== UTILITY FUNCTIONS ====================
int CreateGameRoom() {
    if (game_count >= MAX_GAMES) return -1;
    pthread_mutex_lock(&games_lock);
    games[game_count] = CreateGame(game_count, 1000 + game_count);
    games[game_count]->lamport = GetLamportClock();
    int game_id = game_count;
    game_count++;
    pthread_mutex_unlock(&games_lock);
    return game_id;
}

GameState * GetGameById(int game_id) {
    if (game_id < 0 || game_id >= game_count) return NULL;
    return games[game_id];
}

int AddPlayerToGame(int game_id, Player * player) {
    GameState * game = GetGameById(game_id);
    if (game == NULL || game->player_count >= 5) return -1;
    pthread_mutex_lock(&games_lock);
    game->players[game->player_count] = player;
    player->player_id = game->player_count;
    player->lamport = GetLamportClock();
    game->player_count++;
    pthread_mutex_unlock(&games_lock);
    return 0;
}

void SendMessage(int client_socket, const char* msg_type, const char* payload) {
    char buffer[1024];
    SerializeMessage(msg_type, payload, buffer);
    GetLamportClock();
    send(client_socket, buffer, strlen(buffer), 0);
}

void BroadcastMessage(GameState * game, const char* msg_type, const char* payload) {
    char buffer[1024];
    SerializeMessage(msg_type, payload, buffer);
    GetLamportClock();
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i] != NULL && game->players[i]->socket > 0) {
            send(game->players[i]->socket, buffer, strlen(buffer), 0);
        }
    }
}

// ==================== GAME LOGIC FUNCTIONS ====================

// Thread-safe Nope Waiter. Pauses the current card execution for 4 seconds to give clients time to Nao.
int WaitForNope(GameState* game, Player* initiator, CardType card_type) {
    char payload[128];
    snprintf(payload, sizeof(payload), "%s|%s", initiator->username, GetCardName(card_type));
    BroadcastMessage(game, "NOPE_WINDOW", payload);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 4; // 4 seconds delay window

    pthread_mutex_lock(&game->nope_mutex);
    game->nope_active = 1;
    game->nope_count = 0;

    while (game->nope_active) {
        int res = pthread_cond_timedwait(&game->nope_cond, &game->nope_mutex, &ts);
        if (res == ETIMEDOUT) break; 
        
        if (game->nope_active) {
            // A Nao was played! Reset the timer for 4 more seconds.
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 4;
        }
    }
    game->nope_active = 0;
    int is_noped = (game->nope_count % 2 != 0);
    pthread_mutex_unlock(&game->nope_mutex);
    
    if (is_noped) BroadcastMessage(game, "ACTION_CANCELED", "");
    return is_noped;
}

void SyncAllHands(GameState* game) {
    for (int pi = 0; pi < game->player_count; pi++) {
        Player* p = game->players[pi];
        if (p != NULL && p->socket > 0) {
            char h[1024];
            SerializePlayerHand(p, h);
            SendMessage(p->socket, "HAND_UPDATE", h);
        }
    }
}

// ==================== GAME LOOP ====================
void StartGameLoop(GameState * game) {
    if (game == NULL || game->player_count < 2) return;
    printf("\n========== STARTING GAME ==========\n");
    game->status = GAME_IN_PROGRESS;
    InitializeDeck(game);
    
    for (int i = 0; i < game->player_count; i++) {
        Cartas * deact = CrearCard(DEACTIVATION, 900 + i);
        AddCardToHand(game->players[i], deact);
        for (int j = 0; j < 4; j++) {
            Cartas * card = DrawCardFromDeck(game);
            if (card != NULL) AddCardToHand(game->players[i], card);
        }
    }
    
    // Exactly 4 exploding kittens
    int kitten_id = 1000;
    for (int i = 0; i < 4; i++) {
        Cartas * kitten = CrearCard(EXPLODING_KITTEN, kitten_id++);
        InsertCardIntoDeck(game, kitten, 0);
    }
    ShuffleDeck(game);
    
    int first_player_idx = rand() % game->player_count;
    game->turn = (Turn *)malloc(sizeof(Turn));
    game->turn->current_player = game->players[first_player_idx];
    game->turn->turn_number = 1;
    game->current_player = game->players[first_player_idx];

    for (int i = 0; i < game->player_count; i++) {
        game->players[i]->is_alive = 1;
        game->players[i]->pending_turns = 0;
    }
    // Set 1st turn properties correctly
    game->turn->current_player->pending_turns = 1;

    PrintGameStatus(game);
}

void ProcessPlayerAction(GameState * game, Player* player, const char* action_msg) {
    if (game == NULL || player == NULL) return;
    
    char action_type[256], payload[256];
    ParseMessage(action_msg, action_type, payload);
    
    // Asynchronous Nao Intercept 
    if (strcmp(action_type, "PLAY_NAO") == 0) {
        int card_id = atoi(payload);
        Cartas* peek = FindCardById(player->hand, card_id);
        if (peek != NULL && peek->type == NAO) {
            pthread_mutex_lock(&game->nope_mutex);
            if (game->nope_active) {
                Cartas* nope_card = RemoveCardFromHand(player, card_id);
                game->nope_count++;
                
                char nope_msg[128];
                snprintf(nope_msg, sizeof(nope_msg), "%s", player->username);
                BroadcastMessage(game, "NOPE_PLAYED", nope_msg);
                
                char discard_upd[128];
                snprintf(discard_upd, sizeof(discard_upd), "%d|%s", (int)nope_card->type, nope_card->nombre);
                BroadcastMessage(game, "DISCARD_UPDATE", discard_upd);
                
                free(nope_card);
                pthread_cond_signal(&game->nope_cond);
            }
            pthread_mutex_unlock(&game->nope_mutex);
        }
        return; 
    }

    // Verify it is this player's turn or they are fulfilling a required response
    int is_my_turn = (game->turn->current_player->player_id == player->player_id);
    
    // Handshake checking for targeted callbacks overriding generic turn flow
    if (!is_my_turn) {
        if (game->waiting_for_action == FAVOR_SELECT_CARD && game->action_target == player->player_id && strcmp(action_type, "GIVE_FAVOR_CARD") == 0) {
            // Allow processing to proceed
        } else {
            return; 
        }
    }

    if (strcmp(action_type, "PLAY_CARD") == 0 && game->waiting_for_action == ACTION_NONE) {
        int card_id = atoi(payload);
        Cartas * peek = FindCardById(player->hand, card_id);
        if (peek == NULL || !ValidateCardPlay(player, peek)) {
            SendMessage(player->socket, "ERROR", "Cannot play that card right now.");
            return;
        }

        Cartas * card = RemoveCardFromHand(player, card_id);
        if (card == NULL) return;

        char discard_upd[128];
        snprintf(discard_upd, sizeof(discard_upd), "%d|%s", (int)card->type, card->nombre);
        BroadcastMessage(game, "DISCARD_UPDATE", discard_upd);
        SyncAllHands(game);

        if (WaitForNope(game, player, card->type)) {
            free(card);
            return; // Noped! Do not resolve
        }

        switch (card->type) {
            case ATTACK:
                HandleAttackCard(game, player);
                break;
            case SKIP:
                HandleSkipCard(game, player);
                break;
            case SHUFFLE:
                ShuffleDeck(game);
                break;
            case FUTURE_SIGHT: {
                char fs_buf[256] = "";
                Cartas* top = game->deck ? Inicio(game->deck) : NULL;
                for (int fi = 0; fi < 3 && top != NULL; fi++) {
                    char entry[32];
                    snprintf(entry, sizeof(entry), "%s%d|%d", fi > 0 ? ";" : "", (int)top->type, top->card_id);
                    strncat(fs_buf, entry, sizeof(fs_buf) - strlen(fs_buf) - 1);
                    top = top->sig;
                }
                SendMessage(player->socket, "FUTURE_SIGHT_RESULT", fs_buf);
                break;
            }
            case FAVOR:
                game->waiting_for_action = FAVOR_SELECT_TARGET;
                game->action_initiator = player->player_id;
                SendMessage(player->socket, "REQUEST_TARGET", "Choose a player for Favor");
                break;
            default:
                // Cannot play cat cards isolated.
                AddCardToHand(player, card);
                SendMessage(player->socket, "ERROR", "Cat cards must be played in pairs.");
                return;
        }
        free(card);
    } 
    else if (strcmp(action_type, "PLAY_PAIR") == 0 && game->waiting_for_action == ACTION_NONE) {
        int id1, id2;
        sscanf(payload, "%d|%d", &id1, &id2);

        Cartas* p1 = FindCardById(player->hand, id1);
        Cartas* p2 = FindCardById(player->hand, id2);

        if (p1 == NULL || p2 == NULL || p1->type != p2->type) {
            SendMessage(player->socket, "ERROR", "Invalid pair.");
            return;
        }

        Cartas* c1 = RemoveCardFromHand(player, id1);
        Cartas* c2 = RemoveCardFromHand(player, id2);

        char discard_upd[128];
        snprintf(discard_upd, sizeof(discard_upd), "%d|%s", (int)c1->type, c1->nombre);
        BroadcastMessage(game, "DISCARD_UPDATE", discard_upd);
        SyncAllHands(game);

        if (WaitForNope(game, player, c1->type)) {
            free(c1); free(c2); return;
        }

        game->waiting_for_action = PAIR_SELECT_TARGET;
        game->action_initiator = player->player_id;
        SendMessage(player->socket, "REQUEST_TARGET", "Choose target to steal from");
        
        free(c1); free(c2);
    }
    else if (strcmp(action_type, "CHOOSE_TARGET") == 0) {
        int target_id = atoi(payload);
        if (target_id >= 0 && target_id < game->player_count && game->players[target_id] != NULL && game->players[target_id]->is_alive) {
            
            if (game->waiting_for_action == FAVOR_SELECT_TARGET) {
                game->waiting_for_action = FAVOR_SELECT_CARD;
                game->action_target = target_id;
                char req_msg[128];
                snprintf(req_msg, sizeof(req_msg), "%s", player->username);
                SendMessage(game->players[target_id]->socket, "FAVOR_REQUEST", req_msg);
            } 
            else if (game->waiting_for_action == PAIR_SELECT_TARGET) {
                HandlePairCard(game, player, game->players[target_id]);
                game->waiting_for_action = ACTION_NONE;
            }
        }
    }
    else if (strcmp(action_type, "GIVE_FAVOR_CARD") == 0 && game->waiting_for_action == FAVOR_SELECT_CARD) {
        int card_id = atoi(payload);
        Cartas* given = RemoveCardFromHand(player, card_id);
        if (given != NULL) {
            AddCardToHand(game->players[game->action_initiator], given);
        }
        game->waiting_for_action = ACTION_NONE;
    }
    else if (strcmp(action_type, "DRAW_CARD") == 0 && game->waiting_for_action == ACTION_NONE) {
        Cartas* discarded_deact = NULL;
        int drew_kitten = ProcessDrawCard(game, player, &discarded_deact);
        
        if (drew_kitten) {
            char kitten_msg[128];
            snprintf(kitten_msg, sizeof(kitten_msg), "%s", player->username);
            BroadcastMessage(game, "KITTEN_DRAWN", kitten_msg);
            
            if (discarded_deact != NULL) {
                char discard_upd[128];
                snprintf(discard_upd, sizeof(discard_upd), "%d|%s", (int)discarded_deact->type, discarded_deact->nombre);
                BroadcastMessage(game, "DISCARD_UPDATE", discard_upd);
                free(discarded_deact);
            }
        }
    }
}

// ==================== CLIENT HANDLER ====================
void* HandleClient(void* args) {
    ClientInfo* client_info = (ClientInfo*)args;
    int client_socket = client_info->client_socket;
    char buffer[1024];
    
    recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[strlen(buffer)] = '\0';
    Player * player = CreatePlayer(0, buffer);
    player->socket = client_socket;
    SendMessage(client_socket, "WELCOME", player->username);
    
    recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[strlen(buffer)] = '\0';
    char msg_type[256], payload[256];
    ParseMessage(buffer, msg_type, payload);
    
    GameState * game = NULL;
    
    if (strcmp(msg_type, "CREATE_GAME") == 0) {
        int game_id = CreateGameRoom();
        AddPlayerToGame(game_id, player);
        game = GetGameById(game_id);
        char response[256];
        snprintf(response, 256, "%d|%d|%d", game->player_count, game->room_id, player->player_id);
        SendMessage(client_socket, "GAME_CREATED", response);
        
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 100000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        while (game->status != GAME_IN_PROGRESS) {
            int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                ParseMessage(buffer, msg_type, payload);
                if (strcmp(msg_type, "START_GAME") == 0 && game->player_count >= 2) {
                    StartGameLoop(game);
                    char game_state[1024];
                    SerializeGameState(game, game_state);
                    BroadcastMessage(game, "GAME_STARTED", game_state);
                    char first_turn[32];
                    snprintf(first_turn, sizeof(first_turn), "%d", game->turn->current_player->player_id);
                    BroadcastMessage(game, "GAME_UPDATE", first_turn);
                    break;
                }
            } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
            static time_t last_broadcast = 0;
            time_t now = time(NULL);
            if (now - last_broadcast >= 1) {
                char player_count_str[256];
                snprintf(player_count_str, 256, "%d", game->player_count);
                BroadcastMessage(game, "PLAYER_COUNT_UPDATE", player_count_str);
                last_broadcast = now;
            }
        }
    } 
    else if (strcmp(msg_type, "JOIN_GAME") == 0) {
        SendMessage(client_socket, "ENTER_ROOM_CODE", "");
        recv(client_socket, buffer, sizeof(buffer), 0);
        buffer[strlen(buffer)] = '\0';
        int room_code; sscanf(buffer, "%d", &room_code);
        
        int found_game = -1;
        for (int i = 0; i < game_count; i++) {
            if (games[i]->room_id == room_code && games[i]->player_count < 5) { found_game = i; break; }
        }
        
        if (found_game != -1) {
            AddPlayerToGame(found_game, player);
            game = games[found_game];
            char response[256];
            snprintf(response, 256, "%d|%d|%d", game->player_count, game->room_id, player->player_id);
            SendMessage(client_socket, "GAME_JOINED", response);
            
            char player_count_str[256];
            snprintf(player_count_str, 256, "%d", game->player_count);
            BroadcastMessage(game, "PLAYER_COUNT_UPDATE", player_count_str);
            
            struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 100000;
            setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            while (game->status != GAME_IN_PROGRESS) {
                int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes > 0 && bytes <= 0) break;
                usleep(50000);
            }
        } else {
            SendMessage(client_socket, "ERROR", "Game not found");
            close(client_socket);
            pthread_exit(NULL);
        }
    }

    if (game != NULL && game->status == GAME_IN_PROGRESS) {
        char hand_str[1024];
        SerializePlayerHand(player, hand_str);
        SendMessage(client_socket, "HAND_UPDATE", hand_str);

        if (player->player_id == 0) {
            sleep(1);
            char update_payload[256];
            snprintf(update_payload, 256, "%d", game->turn->current_player->player_id);
            BroadcastMessage(game, "GAME_UPDATE", update_payload);
        }

        struct timeval tv2; tv2.tv_sec = 0; tv2.tv_usec = 100000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(tv2));

        while (game->status == GAME_IN_PROGRESS) {
            int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                break;
            }
            if (bytes == 0) break;
            buffer[bytes] = '\0';

            pthread_mutex_lock(&games_lock);
            ProcessPlayerAction(game, player, buffer);
            SyncAllHands(game);

            if (game->status == GAME_IN_PROGRESS && game->waiting_for_action == ACTION_NONE) {
                char upd[64];
                snprintf(upd, sizeof(upd), "%d", game->turn->current_player->player_id);
                BroadcastMessage(game, "GAME_UPDATE", upd);
            }

            int game_over = (game->status == GAME_ENDED);
            int winner_id  = game->winner_id;
            pthread_mutex_unlock(&games_lock);

            if (game_over) {
                char winner_msg[256];
                snprintf(winner_msg, sizeof(winner_msg), "%d|%s", winner_id, game->players[winner_id]->username);
                BroadcastMessage(game, "GAME_ENDED", winner_msg);
                break;
            }
        }
    }
    
    close(client_socket); free(client_info); FreePlayer(player);
    pthread_exit(NULL);
}

int main(void) {
    srand(time(NULL));
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return 1;
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;
    if (listen(server_fd, 10) < 0) return 1;

    printf("========== EXPLODING KITTENS SERVER ==========\n");
    printf("Server running on port 8080\n");
    printf("Waiting for connections...\n\n");

    while(1) {
        struct sockaddr_in client_addr; socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client < 0) continue;

        ClientInfo* client_info = (ClientInfo*)malloc(sizeof(ClientInfo));
        client_info->client_socket = client;
        client_info->player_id = -1;
        client_info->game_id = -1;
        client_info->player = NULL;
        
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, HandleClient, (void*)client_info);
        pthread_detach(thread_id);
    }
    close(server_fd); return 0;
}