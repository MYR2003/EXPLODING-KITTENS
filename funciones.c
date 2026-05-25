#include <sys/socket.h> 
#include <unistd.h>    
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "funciones.h"

// ==================== CARD NAME & DESCRIPTION MAPPINGS ====================
char* GetCardName(CardType type) {
    switch(type) {
        case EXPLODING_KITTEN: return "Exploding Kitten";
        case DEACTIVATION: return "Deactivation";
        case ATTACK: return "Attack";
        case SKIP: return "Skip";
        case FAVOR: return "Favor";
        case SHUFFLE: return "Shuffle";
        case FUTURE_SIGHT: return "Future Sight";
        case NAO: return "Nao";
        case TACO: return "Taco";
        case MELLON: return "Mellon";
        case POTATO: return "Potato";
        case BREAD: return "Bread";
        case APPLE: return "Apple";
        default: return "Unknown";
    }
}

char* GetCardDescription(CardType type) {
    switch(type) {
        case EXPLODING_KITTEN: return "Draw this and you lose!";
        case DEACTIVATION: return "Disarm an exploding kitten";
        case ATTACK: return "Next player takes 2 turns";
        case SKIP: return "Skip your turn";
        case FAVOR: return "Another player gives you a card";
        case SHUFFLE: return "Shuffle the deck";
        case FUTURE_SIGHT: return "See top 3 cards";
        case NAO: return "Cancel any card effect";
        case TACO: return "Steal a random card (need 2)";
        case MELLON: return "Steal a random card (need 2)";
        case POTATO: return "Steal a random card (need 2)";
        case BREAD: return "Steal a random card (need 2)";
        case APPLE: return "Steal a random card (need 2)";
        default: return "Unknown card";
    }
}

CardType StringToCardType(const char* str) {
    if (strcmp(str, "EXPLODING_KITTEN") == 0) return EXPLODING_KITTEN;
    if (strcmp(str, "DEACTIVATION") == 0) return DEACTIVATION;
    if (strcmp(str, "ATTACK") == 0) return ATTACK;
    if (strcmp(str, "SKIP") == 0) return SKIP;
    if (strcmp(str, "FAVOR") == 0) return FAVOR;
    if (strcmp(str, "SHUFFLE") == 0) return SHUFFLE;
    if (strcmp(str, "FUTURE_SIGHT") == 0) return FUTURE_SIGHT;
    if (strcmp(str, "NAO") == 0) return NAO;
    if (strcmp(str, "TACO") == 0) return TACO;
    if (strcmp(str, "MELLON") == 0) return MELLON;
    if (strcmp(str, "POTATO") == 0) return POTATO;
    if (strcmp(str, "BREAD") == 0) return BREAD;
    if (strcmp(str, "APPLE") == 0) return APPLE;
    return -1;
}

// ==================== CARD CREATION ====================
Cartas * CrearCard(CardType type, int card_id) {
    Cartas * temp = (Cartas *)malloc(sizeof(Cartas));
    temp->type = type;
    temp->card_id = card_id;
    temp->nombre = GetCardName(type);
    temp->descripcion = GetCardDescription(type);
    temp->sig = NULL;
    temp->ant = NULL;
    temp->lamport = 0;
    return temp;
}

Cartas * Crear(char* nombre, char* descripcion) {
    Cartas * temp = (Cartas *)malloc(sizeof(Cartas));
    temp->nombre = nombre;
    temp->descripcion = descripcion;
    temp->sig = NULL;
    temp->ant = NULL;
    temp->lamport = 0;
    temp->type = -1;
    temp->card_id = -1;
    return temp;
}

// ==================== LINKED LIST OPERATIONS ====================
Cartas * Inicio(Cartas * nodo) {
    if (nodo == NULL) return NULL;
    if (nodo->ant == NULL) return nodo;
    return Inicio(nodo->ant);
}

Cartas * Final(Cartas * nodo) {
    if (nodo == NULL) return NULL;
    if (nodo->sig == NULL) return nodo;
    return Final(nodo->sig);
}

Cartas * Insert(Cartas * carta, Cartas * nodo) {
    if (nodo == NULL) return carta;
    if (nodo->sig == NULL) {
        nodo->sig = carta;
        carta->ant = nodo;
        return Inicio(nodo);
    }
    return Insert(carta, nodo->sig);
}

Cartas * InsertActual(Cartas * carta, Cartas * nodo) {
    if (nodo == NULL) return carta;
    if (nodo->ant == NULL) {
        nodo->ant = carta;
        carta->sig = nodo;
        return Inicio(nodo);
    } else {
        Cartas * temp = nodo->ant;
        temp->sig = carta;
        nodo->ant = carta;
        carta->ant = temp;
        carta->sig = nodo;
        return Inicio(nodo);
    }
}

Cartas * Pop(Cartas ** nodo) {
    if (nodo == NULL || *nodo == NULL) return NULL;
    Cartas * final = Final(*nodo);
    if (final->ant != NULL) final->ant->sig = NULL;
    else *nodo = NULL;
    final->ant = NULL;
    final->sig = NULL;
    return final;
}

Cartas * PopActual(Cartas ** nodo) {
    if (*nodo == NULL) return NULL;
    Cartas * actual = *nodo;
    if (actual->ant == NULL && actual->sig == NULL) {
        *nodo = NULL;
        return actual;
    }
    if (actual->ant != NULL) actual->ant->sig = actual->sig;
    if (actual->sig != NULL) actual->sig->ant = actual->ant;
    actual->ant = NULL;
    actual->sig = NULL;
    return actual;
}

Cartas * PopById(Cartas ** nodo, int card_id) {
    if (*nodo == NULL) return NULL;
    Cartas * current = Inicio(*nodo);
    while (current != NULL) {
        if (current->card_id == card_id) {
            if (current->ant != NULL) current->ant->sig = current->sig;
            if (current->sig != NULL) current->sig->ant = current->ant;
            if (current == *nodo) *nodo = current->sig;
            if (*nodo != NULL) *nodo = Inicio(*nodo);
            current->ant = NULL;
            current->sig = NULL;
            return current;
        }
        current = current->sig;
    }
    return NULL;
}

int CountCartas(Cartas * nodo, int primeraVez) {
    if (nodo == NULL) return 0;
    if (nodo->ant != NULL && primeraVez == 1) nodo = Inicio(nodo);
    return CountCartas(nodo->sig, 0) + 1;
}

void PrintAllCartas(Cartas * nodo, int primeraVez) {
    if (nodo == NULL) {
        if (primeraVez == 1) printf("No hay cartas\n");
        return;
    }
    if (nodo->ant != NULL && primeraVez == 1) nodo = Inicio(nodo);
    printf("Carta: %s (ID: %d)\n", nodo->nombre, nodo->card_id);
    printf("Descripcion: %s\n", nodo->descripcion);
    PrintAllCartas(nodo->sig, 0);
}

void PrintCard(Cartas * card) {
    if (card == NULL) return;
    printf("[Card ID:%d] %s - %s\n", card->card_id, card->nombre, card->descripcion);
}

Cartas * FindCardById(Cartas * nodo, int card_id) {
    if (nodo == NULL) return NULL;
    nodo = Inicio(nodo);
    while (nodo != NULL) {
        if (nodo->card_id == card_id) return nodo;
        nodo = nodo->sig;
    }
    return NULL;
}

Cartas * FindCardByType(Cartas * nodo, CardType type) {
    if (nodo == NULL) return NULL;
    nodo = Inicio(nodo);
    while (nodo != NULL) {
        if (nodo->type == type) return nodo;
        nodo = nodo->sig;
    }
    return NULL;
}

int HasCardType(Cartas * nodo, CardType type) {
    if (nodo == NULL) return 0;
    nodo = Inicio(nodo);
    while (nodo != NULL) {
        if (nodo->type == type) return 1;
        nodo = nodo->sig;
    }
    return 0;
}

int CountCardType(Cartas * nodo, CardType type) {
    if (nodo == NULL) return 0;
    int count = 0;
    nodo = Inicio(nodo);
    while (nodo != NULL) {
        if (nodo->type == type) count++;
        nodo = nodo->sig;
    }
    return count;
}

void FreeAllCartas(Cartas ** nodo) {
    if (*nodo == NULL) return;
    *nodo = Inicio(*nodo);
    while (*nodo != NULL) {
        Cartas * temp = *nodo;
        *nodo = (*nodo)->sig;
        free(temp);
    }
}

// ==================== PLAYER OPERATIONS ====================
Player * CreatePlayer(int player_id, const char* username) {
    Player * player = (Player *)malloc(sizeof(Player));
    player->player_id = player_id;
    player->username = (char *)malloc(strlen(username) + 1);
    strcpy(player->username, username);
    player->hand = NULL;
    player->hand_size = 0;
    player->is_alive = 1;
    player->pending_turns = 0;
    player->lamport = 0;
    return player;
}

void AddCardToHand(Player* player, Cartas * card) {
    if (player == NULL || card == NULL) return;
    if (player->hand == NULL) player->hand = card;
    else player->hand = Insert(card, player->hand);
    player->hand_size++;
}

Cartas * RemoveCardFromHand(Player* player, int card_id) {
    if (player == NULL || player->hand == NULL) return NULL;
    Cartas * removed = PopById(&(player->hand), card_id);
    if (removed != NULL) player->hand_size--;
    return removed;
}

void PrintPlayerHand(Player* player) {
    if (player == NULL) return;
    printf("\n=== %s's Hand ===\n", player->username);
    if (player->hand == NULL) printf("No cards in hand\n");
    else PrintAllCartas(player->hand, 1);
}

void PrintPlayerInfo(Player* player) {
    if (player == NULL) return;
    printf("\n--- Player %d: %s ---\n", player->player_id, player->username);
    printf("Status: %s\n", player->is_alive ? "ALIVE" : "DEAD");
    printf("Hand Size: %d\n", player->hand_size);
    printf("Pending Turns: %d\n", player->pending_turns);
}

void FreePlayer(Player* player) {
    if (player == NULL) return;
    FreeAllCartas(&(player->hand));
    free(player->username);
    free(player);
}

// ==================== GAME OPERATIONS ====================
GameState * CreateGame(int game_id, int room_id) {
    GameState * game = (GameState *)malloc(sizeof(GameState));
    game->game_id = game_id;
    game->room_id = room_id;
    game->player_count = 0;
    game->current_player = NULL;
    game->deck = NULL;
    game->deck_size = 0;
    game->status = LOBBY;
    game->winner_id = -1;
    game->lamport = 0;
    game->last_action = NULL;
    game->turn = NULL;
    
    game->nope_active = 0;
    game->nope_count = 0;
    pthread_cond_init(&game->nope_cond, NULL);
    pthread_mutex_init(&game->nope_mutex, NULL);

    game->waiting_for_action = ACTION_NONE;
    game->action_initiator = -1;
    game->action_target = -1;

    for (int i = 0; i < 5; i++) {
        game->players[i] = NULL;
    }
    return game;
}

void InitializeDeck(GameState * game) {
    if (game == NULL) return;
    FreeAllCartas(&(game->deck));
    game->deck = NULL;
    game->deck_size = 0;
    
    int card_id = 0;
    
    // Exact requested distributions
    for (int i = 0; i < 6; i++) {
        game->deck = Insert(CrearCard(DEACTIVATION, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(ATTACK, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(SKIP, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(FAVOR, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(SHUFFLE, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 5; i++) {
        game->deck = Insert(CrearCard(FUTURE_SIGHT, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 5; i++) {
        game->deck = Insert(CrearCard(NAO, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(TACO, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(MELLON, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(POTATO, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(BREAD, card_id++), game->deck);
        game->deck_size++;
    }
    for (int i = 0; i < 4; i++) {
        game->deck = Insert(CrearCard(APPLE, card_id++), game->deck);
        game->deck_size++;
    }
    ShuffleDeck(game);
}

void ShuffleDeck(GameState * game) {
    if (game == NULL || game->deck_size < 2) return;
    Cartas ** cards = (Cartas **)malloc(game->deck_size * sizeof(Cartas *));
    Cartas * current = Inicio(game->deck);
    int index = 0;
    while (current != NULL) {
        cards[index++] = current;
        current = current->sig;
    }
    for (int i = game->deck_size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Cartas * temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
    for (int i = 0; i < game->deck_size; i++) {
        cards[i]->ant = (i > 0) ? cards[i-1] : NULL;
        cards[i]->sig = (i < game->deck_size - 1) ? cards[i+1] : NULL;
    }
    game->deck = cards[0];
    free(cards);
}

Cartas * DrawCardFromDeck(GameState * game) {
    if (game == NULL || game->deck == NULL) return NULL;
    Cartas * card = Inicio(game->deck);
    if (card != NULL) {
        if (card->sig != NULL) {
            card->sig->ant = NULL;
            game->deck = card->sig;
        } else {
            game->deck = NULL;
        }
        card->sig = NULL;
        card->ant = NULL;
        game->deck_size--;
    }
    return card;
}

void InsertCardIntoDeck(GameState * game, Cartas * card, int position) {
    if (game == NULL || card == NULL) return;
    if (position <= 0 || game->deck == NULL) {
        card->sig = game->deck;
        card->ant = NULL;
        if (game->deck != NULL) game->deck->ant = card;
        game->deck = card;
    } else {
        Cartas * current = Inicio(game->deck);
        int index = 0;
        while (current != NULL && index < position - 1) {
            current = current->sig;
            index++;
        }
        if (current != NULL) {
            card->sig = current->sig;
            card->ant = current;
            if (current->sig != NULL) current->sig->ant = card;
            current->sig = card;
        } else {
            game->deck = Insert(card, game->deck);
        }
    }
    game->deck_size++;
}

void PrintDeckTopCards(GameState * game, int num_cards) {
    if (game == NULL || game->deck == NULL) return;
    Cartas * current = Inicio(game->deck);
    int count = 0;
    while (current != NULL && count < num_cards) {
        current = current->sig;
        count++;
    }
}

// ==================== HELPER IMPLEMENTATIONS ====================

Player* GetNextPlayer(GameState * game, Player * current) {
    int next_idx = (current->player_id + 1) % game->player_count;
    int attempts = 0;
    while (game->players[next_idx]->is_alive == 0 && attempts < game->player_count) {
        next_idx = (next_idx + 1) % game->player_count;
        attempts++;
    }
    return game->players[next_idx];
}

void NotifyClientsOfTurnChange(GameState * game) {
    char buffer[256];
    sprintf(buffer, "TURN_CHANGE|%s", game->current_player->username);
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i]->socket != -1) {
            send(game->players[i]->socket, buffer, strlen(buffer), 0);
        }
    }
}

int ValidateCardPlay(Player* player, Cartas * card) {
    if (player == NULL || card == NULL) return 0;
    if (card->type == EXPLODING_KITTEN) return 0;
    if (card->type == DEACTIVATION) return 0;
    if (card->type == NAO) return 0; // Nao cannot be played normally as an action
    return 1;
}

int ProcessDrawCard(GameState * game, Player* player, Cartas** discarded_deact) {
    if (game == NULL || player == NULL) return 0;
    Cartas * drawn = DrawCardFromDeck(game);
    int drew_kitten = 0;
    
    if (drawn == NULL) {
        EndTurn(game);
        return 0;
    }
    
    if (drawn->type == EXPLODING_KITTEN) {
        drew_kitten = 1;
        if (HasCardType(player->hand, DEACTIVATION)) {
            Cartas * deact = FindCardByType(player->hand, DEACTIVATION);
            *discarded_deact = RemoveCardFromHand(player, deact->card_id);
            int pos = GetRandomInt(0, game->deck_size);
            InsertCardIntoDeck(game, drawn, pos);
        } else {
            player->is_alive = 0;
            free(drawn);
            CheckWinCondition(game);
        }
    } else {
        AddCardToHand(player, drawn);
    }
    
    // Consuming a turn
    if (player->pending_turns > 0) {
        player->pending_turns--;
    }
    // Only pass to next player if all turns are exhausted
    if (player->pending_turns == 0) {
        EndTurn(game);
    }
    return drew_kitten;
}

void HandleAttackCard(GameState * game, Player* attacker) {
    Player* target = GetNextPlayer(game, attacker);
    
    // Target takes current remaining turns plus 2 extra turns.
    // Standard EK: Next player must take 2 turns.
    if (target->pending_turns == 0) target->pending_turns = 2;
    else target->pending_turns += 2;
    
    attacker->pending_turns = 0; // Immediately ends attacker's phase
    EndTurn(game);
}

void HandleSkipCard(GameState * game, Player* player) {
    if (player == NULL) return;
    if (player->pending_turns > 0) {
        player->pending_turns--;
    }
    if (player->pending_turns == 0) {
        EndTurn(game);
    }
}

void HandleFavor(GameState * game, Player* player, Player* target) {
    (void)game; 
    (void)player;
    (void)target;
    // handled entirely through the server state machine now
}

void HandlePairCard(GameState * game, Player* player, Player* target) {
    (void)game; 
    if (target == NULL || target->hand == NULL) return;
    
    Cartas * current = Inicio(target->hand);
    int count = CountCartas(target->hand, 1);
    if (count <= 0) return;
    
    int random_pos = GetRandomInt(0, count - 1);
    int idx = 0;
    while (current != NULL && idx < random_pos) {
        current = current->sig;
        idx++;
    }
    
    if (current != NULL) {
        Cartas * stolen = RemoveCardFromHand(target, current->card_id);
        if (stolen != NULL) {
            AddCardToHand(player, stolen);
        }
    }
}

int CheckWinCondition(GameState * game) {
    if (game == NULL) return 0;
    int alive_count = 0;
    int last_alive_id = -1;
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i] != NULL && game->players[i]->is_alive) {
            alive_count++;
            last_alive_id = i;
        }
    }
    if (alive_count <= 1 && last_alive_id != -1) {
        game->status = GAME_ENDED;
        game->winner_id = last_alive_id;
        return 1;
    }
    return 0;
}

void EndTurn(GameState * game) {
    if (game == NULL || game->turn == NULL) return;
    Player* next = GetNextPlayer(game, game->turn->current_player);
    
    // Give the next player 1 standard turn if they don't already have stacked turns (from attacks)
    if (next->pending_turns == 0) {
        next->pending_turns = 1;
    }
    game->turn->current_player = next;
    game->turn->turn_number++;
}

void PrintGameStatus(GameState * game) {
    if (game == NULL) return;
    printf("\n========== GAME STATUS ==========\n");
    printf("Game ID: %d\n", game->game_id);
    printf("Room ID: %d\n", game->room_id);
    printf("Players: %d/5\n", game->player_count);
    printf("Deck Size: %d\n", game->deck_size);
    printf("Game Status: ");
    switch (game->status) {
        case LOBBY: printf("LOBBY\n"); break;
        case WAITING_FOR_PLAYERS: printf("WAITING\n"); break;
        case GAME_IN_PROGRESS: printf("IN PROGRESS\n"); break;
        case GAME_ENDED: printf("ENDED\n"); break;
        default: printf("UNKNOWN\n"); break;
    }
    if (game->turn != NULL) {
        printf("Current Turn: Player %s (Turn #%d)\n",
               game->turn->current_player->username, game->turn->turn_number);
    }
}

// ==================== SERIALIZATION ====================
void SerializeCardToString(Cartas * card, char* buffer) {
    if (card == NULL || buffer == NULL) return;
    sprintf(buffer, "%d|%d", (int)card->type, card->card_id);
}

void SerializePlayerHand(Player* player, char* buffer) {
    if (player == NULL || buffer == NULL) {
        strcpy(buffer, "NULL");
        return;
    }
    if (player->hand == NULL || player->hand_size == 0) {
        strcpy(buffer, "EMPTY");
        return;
    }
    buffer[0] = '\0';
    Cartas * current = Inicio(player->hand);
    int first = 1;
    while (current != NULL) {
        if (!first) strcat(buffer, ";");
        char card_str[50];
        sprintf(card_str, "%d|%d", (int)current->type, current->card_id);
        strcat(buffer, card_str);
        current = current->sig;
        first = 0;
    }
}

void SerializeGameState(GameState * game, char* buffer) {
    if (game == NULL || buffer == NULL) return;
    sprintf(buffer, "%d|%d|%d|%d|%d",
            game->game_id, game->room_id, game->player_count,
            game->deck_size, (int)game->status);
}

Cartas * DeserializeCard(const char* str) {
    if (str == NULL) return NULL;
    int type, id;
    if (sscanf(str, "%d|%d", &type, &id) != 2) return NULL;
    return CrearCard((CardType)type, id);
}

void SerializeMessage(const char* msg_type, const char* payload, char* buffer) {
    if (buffer == NULL) return;
    if (msg_type == NULL) { buffer[0] = '\0'; return; }
    if (payload == NULL || strlen(payload) == 0) {
        sprintf(buffer, "%s:\n", msg_type);
    } else {
        sprintf(buffer, "%s:%s\n", msg_type, payload);
    }
}

void ParseMessage(const char* message, char* msg_type, char* payload) {
    if (message == NULL) {
        strcpy(msg_type, "");
        strcpy(payload, "");
        return;
    }
    const char* colon = strchr(message, ':');
    if (colon == NULL) {
        strcpy(msg_type, message);
        strcpy(payload, "");
    } else {
        int len = colon - message;
        strncpy(msg_type, message, len);
        msg_type[len] = '\0';
        strcpy(payload, colon + 1);
    }
}

// ==================== UTILITY ====================
void PrintHolaMundo() {
    printf("---\nHola Mundo\n---\n");
}

int GetRandomInt(int min, int max) {
    if (min > max) return min;
    return min + (rand() % (max - min + 1));
}

void UpdateLamportClock(int received_clock) {
    (void)received_clock;
}