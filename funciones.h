#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// ==================== CARD ENUM ====================
typedef enum {
    EXPLODING_KITTEN = 0,
    DEACTIVATION = 1,
    ATTACK = 2,
    SKIP = 3,
    FAVOR = 4,
    SHUFFLE = 5,
    FUTURE_SIGHT = 6,
    NAO = 7,
    TACO = 8,
    MELLON = 9,
    POTATO = 10,
    BREAD = 11,
    APPLE = 12
} CardType;

// ==================== LINKED LIST CARD NODE ====================
typedef struct Cartas {
    char* nombre;
    char* descripcion;
    CardType type;
    int card_id;
    struct Cartas * sig;
    struct Cartas * ant;
    int lamport;
} Cartas;

// ==================== PLAYER STRUCT ====================
typedef struct Player {
    int player_id;
    int socket;              
    char* username;
    Cartas * hand;           
    int hand_size;
    int is_alive;
    int pending_turns;       
    int lamport;
} Player;

// ==================== TURN MANAGEMENT ====================
typedef struct Turn {
    Player * current_player;
    Player * next;
    Player * prev;
    int turn_number;
} Turn;

// ==================== GAME STATE ====================
typedef enum {
    LOBBY,
    WAITING_FOR_PLAYERS,
    GAME_IN_PROGRESS,
    GAME_ENDED
} GameStatus;

typedef enum {
    ACTION_NONE,
    FAVOR_SELECT_TARGET,
    FAVOR_SELECT_CARD,
    PAIR_SELECT_TARGET
} ActionState;

typedef struct GameState {
    int game_id;
    int room_id;
    Player * players[5];     
    Player* current_player;
    int player_count;
    Cartas * deck;           
    int deck_size;
    Turn * turn;
    GameStatus status;
    int winner_id;
    int lamport;
    char* last_action;
    
    // Nope Window State
    int nope_active;
    int nope_count;
    pthread_cond_t nope_cond;
    pthread_mutex_t nope_mutex;

    // Target Selection State
    ActionState waiting_for_action;
    int action_initiator;
    int action_target;

} GameState;

// ==================== CARD OPERATIONS ====================
Cartas * CrearCard(CardType type, int card_id);
Cartas * Crear(char* nombre, char* descripcion);
char* GetCardName(CardType type);
char* GetCardDescription(CardType type);
CardType StringToCardType(const char* str);

// ==================== LINKED LIST OPERATIONS ====================
Cartas * Inicio(Cartas * nodo);
Cartas * Final(Cartas * nodo);
Cartas * Insert(Cartas * carta, Cartas * nodo);
Cartas * InsertActual(Cartas * carta, Cartas * nodo);
Cartas * Pop(Cartas ** nodo);
Cartas * PopActual(Cartas ** nodo);
Cartas * PopById(Cartas ** nodo, int card_id);
int CountCartas(Cartas * nodo, int primeraVez);
void PrintAllCartas(Cartas * nodo, int primeraVez);
void PrintCard(Cartas * nodo);
Cartas * FindCardById(Cartas * nodo, int card_id);
Cartas * FindCardByType(Cartas * nodo, CardType type);
int HasCardType(Cartas * nodo, CardType type);
int CountCardType(Cartas * nodo, CardType type);
void FreeAllCartas(Cartas ** nodo);

// ==================== PLAYER OPERATIONS ====================
Player * CreatePlayer(int player_id, const char* username);
void AddCardToHand(Player* player, Cartas * card);
Cartas * RemoveCardFromHand(Player* player, int card_id);
void PrintPlayerHand(Player* player);
void PrintPlayerInfo(Player* player);
void FreePlayer(Player* player);

// ==================== DECK OPERATIONS ====================
GameState * CreateGame(int game_id, int room_id);
void InitializeDeck(GameState * game);
void ShuffleDeck(GameState * game);
Cartas * DrawCardFromDeck(GameState * game);
void InsertCardIntoDeck(GameState * game, Cartas * card, int position);
void PrintDeckTopCards(GameState * game, int num_cards);
void PrintGameState(GameState * game);
void FreeGame(GameState * game);

// ==================== GAME LOGIC ====================
Player* GetNextPlayer(GameState * game, Player * current);
void NotifyClientsOfTurnChange(GameState * game);
int ValidateCardPlay(Player* player, Cartas * card);
void PlayCard(GameState * game, Player* player, Cartas * card, Player* target_player);
void HandleAttackCard(GameState * game, Player* attacker);
void HandleSkipCard(GameState * game, Player* player);
void HandleDeactivation(GameState * game, Player* player);
void HandleFavor(GameState * game, Player* player, Player* target);
void HandlePairCard(GameState * game, Player* player, Player* target);
int ProcessDrawCard(GameState * game, Player* player, Cartas** discarded_deact);
int CheckWinCondition(GameState * game);
void NextPlayerTurn(GameState * game);
void EndTurn(GameState * game);
void PrintGameStatus(GameState * game);

// ==================== SERIALIZATION FOR NETWORK ====================
void SerializeCardToString(Cartas * card, char* buffer);
void SerializePlayerHand(Player* player, char* buffer);
void SerializeGameState(GameState * game, char* buffer);
Cartas * DeserializeCard(const char* str);
void SerializeMessage(const char* msg_type, const char* payload, char* buffer);
void ParseMessage(const char* message, char* msg_type, char* payload);

// ==================== UTILITY ====================
void PrintHolaMundo();
int GetRandomInt(int min, int max);
void UpdateLamportClock(int received_clock);

#endif // FUNCIONES_H