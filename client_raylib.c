#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <raylib.h>
#include <math.h>
#include <signal.h>

#include "funciones.h"

// ==================== I18N (LANGUAGE SUPPORT) ====================
typedef enum {
    TXT_ENTER_USER, TXT_PRESS_ENTER_CONN, TXT_CREATE_GAME, TXT_JOIN_GAME, TXT_EXIT,
    TXT_JOIN_TITLE, TXT_ENTER_ROOM, TXT_JOIN_HINT, TXT_WAITING_TITLE, TXT_ROOM_ID,
    TXT_PLAYERS_COUNT, TXT_PRESS_S_START, TXT_YOUR_TURN, TXT_PLAYER_TURN,
    TXT_DISCARD, TXT_EMPTY, TXT_DECK, TXT_PLAY_CARD, TXT_DRAW_CARD, TXT_END_TURN,
    TXT_CONFIRM, TXT_PLAY_CARD_NUM, TXT_YOUR_HAND, TXT_NO_CARDS, TXT_FUTURE_SIGHT,
    TXT_TOP_3, TXT_CLICK_CLOSE, TXT_GAME_OVER, TXT_WINNER, TXT_PRESS_SPACE,
    TXT_SELECT_TARGET, TXT_GIVE_FAVOR, TXT_NOPE_WINDOW, TXT_NOPE_BTN,
    TXT_COUNT
} TextId;

const char* lang_dict[2][TXT_COUNT] = {
    // 0: ENGLISH
    {
        "Enter Username:", "Press ENTER to connect", "Create Game", "Join Game", "Exit",
        "JOIN GAME", "Enter Room Code:", "ENTER to join  |  ESC to go back",
        "WAITING FOR PLAYERS", "Room ID: %d", "Players: %d / 5", "Press S to start (host only)",
        "✦ YOUR TURN ✦", "Player %d's turn", "Discard", "EMPTY", "Deck", "Play Card(s)",
        "Draw Card", "(end turn)", "[ CONFIRM ]", "Selected", "Your Hand: (%d cards)",
        "(no cards in hand)", "Future Sight", "Top 3 cards:", "Click to close",
        "GAME OVER!", "Winner: %s", "Press SPACE to return to menu",
        "SELECT A TARGET TO STEAL FROM", "CHOOSE A CARD TO GIVE TO %s", "OPPONENT ACTION PENDING", "Yell Nao Nao!"
    },
    // 1: SPANISH
    {
        "Ingrese Usuario:", "Presione ENTER para conectar", "Crear Partida", "Unirse a Partida", "Salir",
        "UNIRSE A PARTIDA", "Codigo de Sala:", "ENTER para unirse | ESC para volver",
        "ESPERANDO JUGADORES", "ID de Sala: %d", "Jugadores: %d / 5", "Presione S para iniciar (solo host)",
        "✦ TU TURNO ✦", "Turno del Jugador %d", "Descarte", "VACIO", "Mazo", "Jugar Carta(s)",
        "Robar Carta", "(terminar turno)", "[ CONFIRMAR ]", "Seleccionada", "Tu Mano: (%d cartas)",
        "(sin cartas en mano)", "Vision Futura", "Top 3 cartas:", "Clic para cerrar",
        "¡FIN DEL JUEGO!", "Ganador: %s", "ESPACIO para volver al menu",
        "SELECCIONA UN OBJETIVO PARA ROBAR", "ELIGE UNA CARTA PARA DAR A %s", "ACCION DE OPONENTE PENDIENTE", "Dile Nao Nao"
    }
};

// Internal Translation overrides for Card Text rendering regardless of Server's language logic
const char* local_card_names[2][13] = {
    { "Kuroha (Exploding Kitten)", "Liora", "Valery Vane", "Nautica", "Elowen", "Elara Vane", "Marina", "Nao", "Taco", "Seraphina Sky", "Amara", "Celestia", "Lyra" },
    { "Kuroha (Exploding Mona Shina)", "Liora", "Valery Vane", "Nautica", "Elowen", "Elara Vane", "Marina", "Nao", "Taco", "Seraphina Sky", "Amara", "Celestia", "Lyra" }
};
const char* local_card_descs[2][13] = {
    { "Draw this and you lose!", "Disarm an exploding kitten", "Next player takes 2 turns", "Skip your turn", "Another player gives you a card", "Shuffle the deck", "See top 3 cards", "Cancel any card effect", "Steal a random card (need 2)", "Steal a random card (need 2)", "Steal a random card (need 2)", "Steal a random card (need 2)", "Steal a random card (need 2)" },
    { "Saca esto y pierdes!", "Desactiva un exploding kitten", "El siguiente jugador toma 2 turnos", "Salta tu turno", "Otro jugador te da una carta", "Baraja el mazo", "Mira las 3 cartas superiores", "Cancela el efecto de cualquier carta", "Roba una carta al azar (necesitas 2)", "Roba una carta al azar (necesitas 2)", "Roba una carta al azar (necesitas 2)", "Roba una carta al azar (necesitas 2)", "Roba una carta al azar (necesitas 2)" }
};

// ==================== GAME STATES ====================
typedef enum { LOGIN, MAIN_MENU, CREATE_GAME, JOIN_GAME, WAITING_PLAYERS, GAME_PLAY, GAME_OVER } ClientGameState;

typedef enum {
    MODE_NONE = 0, MODE_PLAY_CARD = 1, MODE_SELECT_TARGET = 2, MODE_GIVE_FAVOR = 3
} ActionMode;

// ==================== CLIENT STATE ====================
typedef struct {
    int socket;
    Player* player;
    GameState* game;
    int is_connected;
    ClientGameState current_state;
    char room_id_str[50];
    int room_id;
    pthread_t message_thread;
    int game_ended;
    char winner_name[50];
    int client_player_count;

    ActionMode action_mode;
    int selected_card_id_1;
    int selected_card_id_2;

    int discard_top_type;
    char discard_top_name[64];
    char discard_top_desc[128];

    int current_turn_player_id;
    int my_player_id;

    char feedback_msg[128];
    double feedback_timer;

    int  show_future_sight;
    int  future_sight_types[3]; // Uses types instead of text to allow local translations
    int  future_sight_count;

    int nope_window_active;
    double nope_timer_end;
    char nope_info[128];
    
    int show_kitten_anim;
    double kitten_anim_end;
    char kitten_victim[64];
    
    char favor_requester[64];

    int language; // 0 = EN, 1 = ES
    Texture2D flag_usa;
    Texture2D flag_chile;
    int flags_loaded;
} ClientState;

ClientState client;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
#define T(id) lang_dict[client.language][id]

// ==================== CARD ART TEXTURES ====================
typedef struct { Texture2D textures[13]; int loaded[13]; } CardArtTextures;
CardArtTextures cardArt;
const char* cardImagePaths[13] = {
    "assets/Kuroha_(ExplodingKitten).png", "assets/Liora_(Deactivation).png", "assets/(Attack).png",
    "assets/(Skip).jpg", "assets/(Favor).png", "assets/(Shuffle).jpg", "assets/(FutureSight).jpg",
    "assets/Cuy_(Nao).jpg", "assets/(Taco).jpg", "assets/(Mellon).jpg", "assets/(Potato).jpg",
    "assets/(Bread).jpg", "assets/(Apple).jpg"
};

// ==================== UTILITY ====================
void SendMessageNet(const char* msg_type, const char* payload) {
    char buffer[1024];
    SerializeMessage(msg_type, payload, buffer);
    if (send(client.socket, buffer, strlen(buffer), 0) < 0) perror("send");
}

void RecvMessageNet(char* msg_type, char* payload) {
    static char net_buf[8192]; static int  net_len = 0;
    msg_type[0] = '\0'; payload[0]  = '\0';
    for (;;) {
        net_buf[net_len] = '\0';
        char* nl = strchr(net_buf, '\n');
        if (nl) {
            int  linelen = (int)(nl - net_buf);
            char line[2048];
            int  cplen = linelen < (int)sizeof(line)-1 ? linelen : (int)sizeof(line)-1;
            memcpy(line, net_buf, cplen); line[cplen] = '\0';
            int remaining = net_len - linelen - 1;
            if (remaining > 0) memmove(net_buf, nl + 1, remaining);
            net_len = remaining > 0 ? remaining : 0;
            ParseMessage(line, msg_type, payload);
            return;
        }
        int space = (int)sizeof(net_buf) - net_len - 1;
        if (space <= 0) { net_len = 0; return; }
        int got = recv(client.socket, net_buf + net_len, space, 0);
        if (got <= 0) return;
        net_len += got;
    }
}

void UpdatePlayerHand(Player* player, const char* hand_payload) {
    if (!player || !hand_payload) return;
    FreeAllCartas(&player->hand);
    player->hand = NULL; player->hand_size = 0;
    if (strcmp(hand_payload,"EMPTY")==0 || strcmp(hand_payload,"NULL")==0) return;
    char temp[1024]; snprintf(temp, sizeof(temp), "%s", hand_payload);
    char* tok = strtok(temp, ";");
    while (tok) {
        int type, id;
        if (sscanf(tok, "%d|%d", &type, &id)==2) AddCardToHand(player, CrearCard((CardType)type, id));
        tok = strtok(NULL, ";");
    }
}

void SetFeedback(const char* msg) {
    snprintf(client.feedback_msg, sizeof(client.feedback_msg), "%s", msg);
    client.feedback_msg[sizeof(client.feedback_msg)-1]='\0';
    client.feedback_timer = GetTime() + 3.0;
}

void DrawLanguageButton(int sw) {
    int btnW = 46; int btnH = 30; int bx = sw - btnW - 15; int by = 7;
    Vector2 mouse = GetMousePosition();
    Rectangle btnRec = {(float)bx, (float)by, (float)btnW, (float)btnH};
    int hovered = CheckCollisionPointRec(mouse, btnRec);
    DrawRectangleRec(btnRec, hovered ? LIGHTGRAY : (Color){40, 40, 40, 255});
    DrawRectangleLinesEx(btnRec, 2, WHITE);
    Texture2D current_flag = (client.language == 0) ? client.flag_usa : client.flag_chile;
    if (current_flag.width > 0) {
        Rectangle src = {0, 0, (float)current_flag.width, (float)current_flag.height};
        Rectangle dst = {(float)bx + 2, (float)by + 2, (float)btnW - 4, (float)btnH - 4};
        DrawTexturePro(current_flag, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        const char* fallbackText = (client.language == 0) ? "USA" : "CHI";
        DrawText(fallbackText, bx + btnW/2 - MeasureText(fallbackText,14)/2, by + btnH/2 - 6, 14, WHITE);
    }
    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) client.language = (client.language == 0) ? 1 : 0;
}

// ==================== MESSAGE THREAD ====================
void* MessageHandlerThread(void* args) {
    (void)args;
    char msg_type[256], payload[1024];
    while (client.is_connected && !client.game_ended) {
        memset(msg_type,0,sizeof(msg_type)); memset(payload,0,sizeof(payload));
        RecvMessageNet(msg_type, payload);
        if (!msg_type[0]) continue;

        pthread_mutex_lock(&client_lock);
        if (strcmp(msg_type,"HAND_UPDATE")==0) {
            UpdatePlayerHand(client.player, payload);
        }
        else if (strcmp(msg_type,"GAME_CREATED")==0 || strcmp(msg_type,"GAME_JOINED")==0) {
            int pc,rm,pid; sscanf(payload,"%d|%d|%d",&pc,&rm,&pid);
            client.room_id=rm; client.client_player_count=pc; client.my_player_id=pid;
            if (strcmp(msg_type,"GAME_JOINED")==0) client.current_state=WAITING_PLAYERS;
        }
        else if (strcmp(msg_type,"PLAYER_COUNT_UPDATE")==0) client.client_player_count = atoi(payload);
        else if (strcmp(msg_type,"GAME_STARTED")==0) {
            client.current_state=GAME_PLAY; client.action_mode=MODE_NONE;
            client.selected_card_id_1=-1; client.selected_card_id_2=-1;
            client.discard_top_type=-1; strcpy(client.discard_top_name,"Empty");
        }
        else if (strcmp(msg_type,"GAME_UPDATE")==0) {
            int new_turn = atoi(payload);
            if (new_turn != client.current_turn_player_id) {
                client.action_mode = MODE_NONE;
                client.selected_card_id_1 = -1; client.selected_card_id_2 = -1;
            }
            client.current_turn_player_id = new_turn;
        }
        else if (strcmp(msg_type,"DISCARD_UPDATE")==0) {
            int dtype; char dname[64]="";
            if (sscanf(payload,"%d|%63[^\n]",&dtype,dname)>=1) {
                client.discard_top_type=dtype;
                snprintf(client.discard_top_name, sizeof(client.discard_top_name), "%s", dname);
            }
        }
        else if (strcmp(msg_type,"NOPE_WINDOW")==0) {
            client.nope_window_active = 1;
            client.nope_timer_end = GetTime() + 4.0;
            snprintf(client.nope_info, sizeof(client.nope_info), "%s", payload);
        }
        else if (strcmp(msg_type,"NOPE_PLAYED")==0) {
            SetFeedback("NOPE PLAYED!");
            client.nope_window_active = 0;
        }
        else if (strcmp(msg_type,"ACTION_CANCELED")==0) {
            client.nope_window_active = 0;
        }
        else if (strcmp(msg_type,"REQUEST_TARGET")==0) {
            client.action_mode = MODE_SELECT_TARGET;
        }
        else if (strcmp(msg_type,"FAVOR_REQUEST")==0) {
            client.action_mode = MODE_GIVE_FAVOR;
            client.selected_card_id_1 = -1; client.selected_card_id_2 = -1;
            snprintf(client.favor_requester, sizeof(client.favor_requester), "%s", payload);
        }
        else if (strcmp(msg_type,"KITTEN_DRAWN")==0) {
            client.show_kitten_anim = 1;
            client.kitten_anim_end = GetTime() + 4.0;
            snprintf(client.kitten_victim, sizeof(client.kitten_victim), "%s", payload);
        }
        else if (strcmp(msg_type,"FUTURE_SIGHT_RESULT")==0) {
            client.future_sight_count = 0;
            char tmp2[256]; snprintf(tmp2, sizeof(tmp2), "%s", payload);
            char* tok2 = strtok(tmp2, ";");
            while (tok2 && client.future_sight_count < 3) {
                int t2, id2;
                if (sscanf(tok2, "%d|%d", &t2, &id2) == 2) {
                    client.future_sight_types[client.future_sight_count] = t2;
                    client.future_sight_count++;
                }
                tok2 = strtok(NULL, ";");
            }
            client.show_future_sight = 1;
        }
        else if (strcmp(msg_type,"GAME_ENDED")==0) {
            client.game_ended=1; char wname[50]=""; int wid=-1;
            sscanf(payload,"%d|%49[^\n]",&wid,wname);
            snprintf(client.winner_name, sizeof(client.winner_name), "%s", wname[0]?wname:payload);
            client.current_state=GAME_OVER;
        }
        else if (strcmp(msg_type,"ERROR")==0) SetFeedback(payload);
        pthread_mutex_unlock(&client_lock);
    }
    return NULL;
}

Color GetCardBgColor(CardType type) {
    switch(type) {
        case EXPLODING_KITTEN: return (Color){180,40,40,255};
        case DEACTIVATION:     return (Color){40,160,40,255};
        case ATTACK:           return (Color){200,90,20,255};
        case SKIP:             return (Color){30,130,200,255};
        case FAVOR:            return (Color){160,40,160,255};
        case SHUFFLE:          return (Color){80,80,190,255};
        case FUTURE_SIGHT:     return (Color){20,170,170,255};
        case NAO:              return (Color){190,190,20,255};
        case TACO:             return (Color){160,100,40,255};
        case MELLON:           return (Color){50,160,60,255};
        case POTATO:           return (Color){170,150,40,255};
        case BREAD:            return (Color){200,160,80,255};
        case APPLE:            return (Color){190,50,50,255};
        default:               return (Color){80,80,80,255};
    }
}

int DrawFormattedCard(int x, int y, int w, int h, const char* name, const char* desc, CardType type, int selected, int clickable) {
    int border = 5;
    DrawRectangle(x, y, w, h, BLACK);
    DrawRectangle(x+border, y+border, w-border*2, h-border*2, WHITE);
    int innerX = x + border + 4; int innerY = y + border + 4;
    int innerW = w - (border+4)*2; int imgH = (int)((h - border*2) * 0.52f);
    DrawRectangle(innerX - 3, innerY - 3, innerW + 6, imgH + 6, (Color){50,50,50,255});
    if (cardArt.loaded[type] && cardArt.textures[type].width > 0) {
        Rectangle src = {0, 0, (float)cardArt.textures[type].width, (float)cardArt.textures[type].height};
        Rectangle dst = {(float)innerX, (float)innerY, (float)innerW, (float)imgH};
        DrawTexturePro(cardArt.textures[type], src, dst, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangle(innerX, innerY, innerW, imgH, GetCardBgColor(type));
        char big[4]; snprintf(big, sizeof(big), "%c", name[0]);
        int bfs = 48; DrawText(big, innerX + innerW/2 - MeasureText(big,bfs)/2, innerY + imgH/2 - bfs/2, bfs, WHITE);
    }
    int nameBarY = innerY + imgH + 6; int nameBarH = (int)((h - border*2) * 0.14f);
    DrawRectangle(x+border, nameBarY, w-border*2, nameBarH, (Color){50,220,50,255});
    int nameFs = (nameBarH > 22) ? 20 : 14; int nameTW = MeasureText(name, nameFs);
    while (nameTW > w - border*2 - 8 && nameFs > 9) { nameFs--; nameTW = MeasureText(name, nameFs); }
    DrawText(name, x + border + (w-border*2)/2 - nameTW/2, nameBarY + nameBarH/2 - nameFs/2, nameFs, BLACK);
    int descY = nameBarY + nameBarH + 2; int descH = y + h - border - descY;
    DrawRectangle(x+border, descY, w-border*2, descH, (Color){0,210,210,255});
    if (desc && desc[0]) {
        int dfs = 11; int padX = 6, lineH = dfs + 3; int maxLineW = w - border*2 - padX*2;
        char buf[128]; snprintf(buf, sizeof(buf), "%s", desc);
        char lines[5][64]; int nlines = 0; char* word = strtok(buf, " "); char curLine[64] = "";
        while (word && nlines < 5) {
            char test[64];
            if (curLine[0]) snprintf(test, sizeof(test), "%s %s", curLine, word); else snprintf(test, sizeof(test), "%s", word);
            if (MeasureText(test, dfs) <= maxLineW) snprintf(curLine, sizeof(curLine), "%s", test);
            else { if (curLine[0]) { snprintf(lines[nlines++], 64, "%s", curLine); } snprintf(curLine, sizeof(curLine), "%s", word); }
            word = strtok(NULL, " ");
        }
        if (curLine[0] && nlines < 5) snprintf(lines[nlines++], 64, "%s", curLine);
        int totalTextH = nlines * lineH; int startTextY = descY + descH/2 - totalTextH/2;
        for (int i = 0; i < nlines; i++) {
            DrawText(lines[i], x + border + (w-border*2)/2 - MeasureText(lines[i], dfs)/2, startTextY + i*lineH, dfs, BLACK);
        }
    }
    if (selected) {
        DrawRectangleLinesEx((Rectangle){(float)x,(float)y,(float)w,(float)h}, 4, YELLOW);
        DrawRectangleLinesEx((Rectangle){(float)x-2,(float)y-2,(float)w+4,(float)h+4}, 2, (Color){255,220,0,160});
    }
    Vector2 mouse = GetMousePosition();
    int hovered = CheckCollisionPointRec(mouse, (Rectangle){(float)x,(float)y,(float)w,(float)h});
    if (hovered && clickable && !selected) {
        DrawRectangle(x, y, w, h, (Color){255,255,255,30});
        DrawRectangleLinesEx((Rectangle){(float)x,(float)y,(float)w,(float)h}, 2, WHITE);
    }
    return (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && clickable);
}

void DrawFormattedCardSmall(int x, int y, int w, int h, const char* name, const char* desc, CardType type) {
    DrawFormattedCard(x, y, w, h, name, desc, type, 0, 0);
}

void DrawGameplay(int sw, int sh) {
    BeginDrawing();
    ClearBackground((Color){25,55,25,255});

    // Top bar
    DrawRectangle(0,0,sw,44,(Color){15,35,15,255});
    DrawText("EXPLODING KITTENS",12,12,20,WHITE);
    int isMyTurn=(client.current_turn_player_id==client.my_player_id);
    if (isMyTurn) {
        DrawText(T(TXT_YOUR_TURN),sw/2-MeasureText(T(TXT_YOUR_TURN),22)/2,11,22,YELLOW);
    } else {
        char tt[64]; snprintf(tt,sizeof(tt),T(TXT_PLAYER_TURN),client.current_turn_player_id);
        DrawText(tt,sw/2-MeasureText(tt,18)/2,13,18,LIGHTGRAY);
    }

    int cardW = 110, cardH = 160; int handStartX = 10;
    int discW = 120, discH = 175; int discX = sw - discW - 20; int discY = 55;

    DrawText(T(TXT_DISCARD),discX + discW/2 - MeasureText(T(TXT_DISCARD),14)/2, discY-18, 14, LIGHTGRAY);
    if (client.discard_top_type >= 0) {
        DrawFormattedCardSmall(discX, discY, discW, discH, 
            local_card_names[client.language][client.discard_top_type], 
            local_card_descs[client.language][client.discard_top_type], 
            (CardType)client.discard_top_type);
    } else {
        DrawRectangle(discX, discY, discW, discH, (Color){40,40,40,255});
        DrawRectangleLinesEx((Rectangle){discX,discY,discW,discH},2,GRAY);
        DrawText(T(TXT_EMPTY),discX+discW/2-MeasureText(T(TXT_EMPTY),14)/2,discY+discH/2-7,14,GRAY);
    }

    int deckX = discX - 140; int deckY = discY;
    DrawText(T(TXT_DECK),deckX+60-MeasureText(T(TXT_DECK),14)/2,deckY-18,14,LIGHTGRAY);
    DrawRectangle(deckX,deckY,120,175,(Color){50,50,80,255});
    DrawRectangleLinesEx((Rectangle){deckX,deckY,120,175},3,LIGHTGRAY);
    DrawText("?",deckX+47,deckY+70,40,WHITE);

    int btnW=155, btnH=52, btnBaseY = sh - 58;
    int playX = sw/2 - btnW - 8;
    Color playCol = (client.action_mode==MODE_PLAY_CARD) ? (Color){220,130,10,255} : (Color){50,140,50,255};
    DrawRectangleRounded((Rectangle){playX,btnBaseY,btnW,btnH},0.25f,6,playCol);
    if (client.action_mode==MODE_PLAY_CARD) DrawRectangleLinesEx((Rectangle){playX,btnBaseY,btnW,btnH},3,YELLOW);
    DrawText(T(TXT_PLAY_CARD),playX+btnW/2-MeasureText(T(TXT_PLAY_CARD),19)/2,btnBaseY+16,19,WHITE);

    int drawX = sw/2 + 8;
    DrawRectangleRounded((Rectangle){drawX,btnBaseY,btnW,btnH},0.25f,6,(Color){40,40,180,255});
    DrawText(T(TXT_DRAW_CARD),drawX+btnW/2-MeasureText(T(TXT_DRAW_CARD),19)/2,btnBaseY+16,19,WHITE);
    DrawText(T(TXT_END_TURN),drawX+btnW/2-MeasureText(T(TXT_END_TURN),11)/2,btnBaseY+38,11,LIGHTGRAY);

    int availW = sw - handStartX - 170;

    pthread_mutex_lock(&client_lock);
    int handCount = client.player ? client.player->hand_size : 0;
    int dynCardW = cardW;
    if (handCount > 0) { int maxW = availW / handCount - 4; if (maxW < dynCardW) dynCardW = maxW; if (dynCardW < 60) dynCardW = 60; }
    int dynCardH = (int)(dynCardW * 160.0f / 110.0f);
    int dynSpacing = dynCardW + 4;
    int dynHandY = sh - dynCardH - 70;

    int isSelecting = (client.action_mode == MODE_PLAY_CARD || client.action_mode == MODE_GIVE_FAVOR);
    int confirmVisible = (client.selected_card_id_1 >= 0);

    if (confirmVisible && isSelecting) {
        int confW=160, confH=54, confX=sw/2 - confW/2, confY=dynHandY - confH - 12;
        DrawRectangleRounded((Rectangle){confX,confY,confW,confH},0.3f,8,(Color){30,200,30,255});
        DrawRectangleLinesEx((Rectangle){confX,confY,confW,confH},3,YELLOW);
        DrawText(T(TXT_CONFIRM),confX+confW/2-MeasureText(T(TXT_CONFIRM),21)/2,confY+confH/2-14,21,WHITE);
        char sub[32]; snprintf(sub,sizeof(sub),T(TXT_PLAY_CARD_NUM), client.selected_card_id_1);
        DrawText(sub,confX+confW/2-MeasureText(sub,11)/2,confY+confH-17,11,(Color){200,255,200,255});
        
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, (Rectangle){confX,confY,confW,confH})) {
            if (client.action_mode == MODE_PLAY_CARD) {
                if (client.selected_card_id_2 >= 0) {
                    char pay[64]; snprintf(pay, sizeof(pay), "%d|%d", client.selected_card_id_1, client.selected_card_id_2);
                    SendMessageNet("PLAY_PAIR", pay);
                } else {
                    char pay[32]; snprintf(pay, sizeof(pay), "%d", client.selected_card_id_1);
                    SendMessageNet("PLAY_CARD", pay);
                }
            } else if (client.action_mode == MODE_GIVE_FAVOR) {
                char pay[32]; snprintf(pay, sizeof(pay), "%d", client.selected_card_id_1);
                SendMessageNet("GIVE_FAVOR_CARD", pay);
            }
            client.action_mode = MODE_NONE;
            client.selected_card_id_1 = -1;
            client.selected_card_id_2 = -1;
        }
    }

    Cartas* cur = Inicio(client.player ? client.player->hand : NULL);
    int ix=0;
    while (cur) {
        int cx = handStartX + ix * dynSpacing;
        if (cx + dynCardW > sw - 170 + 10) break;
        int isSel = (cur->card_id == client.selected_card_id_1 || cur->card_id == client.selected_card_id_2);
        int cy = isSel ? dynHandY - 14 : dynHandY;

        int clicked = DrawFormattedCard(cx, cy, dynCardW, dynCardH,
            local_card_names[client.language][cur->type], 
            local_card_descs[client.language][cur->type],
            cur->type, isSel, (isSelecting && isMyTurn) || client.action_mode == MODE_GIVE_FAVOR);

        if (clicked) {
            if (cur->card_id == client.selected_card_id_1) {
                client.selected_card_id_1 = client.selected_card_id_2; client.selected_card_id_2 = -1;
            } else if (cur->card_id == client.selected_card_id_2) {
                client.selected_card_id_2 = -1;
            } else if (client.selected_card_id_1 == -1) {
                client.selected_card_id_1 = cur->card_id;
            } else if (client.selected_card_id_2 == -1) {
                client.selected_card_id_2 = cur->card_id;
            }
        }
        cur = cur->sig; ix++;
    }
    pthread_mutex_unlock(&client_lock);

    char hlabel[48]; snprintf(hlabel, sizeof(hlabel), T(TXT_YOUR_HAND), handCount);
    DrawText(hlabel, handStartX, dynHandY - 22, 16, WHITE);
    if (handCount == 0) DrawText(T(TXT_NO_CARDS), handStartX, sh - 130, 16, LIGHTGRAY);

    // FUTURE SIGHT VIEW
    if (client.show_future_sight) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,180});
        int boxW=320, boxH=220, boxX=sw/2-boxW/2, boxY=sh/2-boxH/2;
        DrawRectangleRounded((Rectangle){boxX,boxY,boxW,boxH},0.1f,6,(Color){20,170,170,255});
        DrawRectangleLinesEx((Rectangle){boxX,boxY,boxW,boxH},3,WHITE);
        DrawText(T(TXT_FUTURE_SIGHT),boxX+boxW/2-MeasureText(T(TXT_FUTURE_SIGHT),20)/2,boxY+12,20,WHITE);
        DrawText(T(TXT_TOP_3),boxX+16,boxY+44,15,LIGHTGRAY);
        for (int fi=0; fi<client.future_sight_count; fi++) {
            char line[80]; 
            snprintf(line,sizeof(line),"%d. %s",fi+1,local_card_names[client.language][client.future_sight_types[fi]]);
            DrawText(line,boxX+24,boxY+68+fi*36,18,YELLOW);
        }
        DrawText(T(TXT_CLICK_CLOSE),boxX+boxW/2-MeasureText(T(TXT_CLICK_CLOSE),13)/2,boxY+boxH-28,13,LIGHTGRAY);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) client.show_future_sight = 0;
    }

    // TARGET SELECTION OVERLAY
    if (client.action_mode == MODE_SELECT_TARGET) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,200});
        DrawText(T(TXT_SELECT_TARGET), sw/2 - MeasureText(T(TXT_SELECT_TARGET),24)/2, sh/4, 24, YELLOW);
        
        for (int i=0; i<client.client_player_count; i++) {
            if (i == client.my_player_id) continue;
            int boxW=200, boxH=50, boxX=sw/2-boxW/2, boxY=sh/2 - 100 + i*60;
            Rectangle rec = {boxX, boxY, boxW, boxH};
            Vector2 m = GetMousePosition();
            int hov = CheckCollisionPointRec(m, rec);
            DrawRectangleRec(rec, hov ? LIGHTGRAY : GRAY);
            DrawRectangleLinesEx(rec, 2, WHITE);
            char tlbl[32]; snprintf(tlbl, sizeof(tlbl), "Player %d", i);
            DrawText(tlbl, boxX+boxW/2-MeasureText(tlbl,20)/2, boxY+15, 20, BLACK);
            
            if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                char pay[32]; snprintf(pay, sizeof(pay), "%d", i);
                SendMessageNet("CHOOSE_TARGET", pay);
                client.action_mode = MODE_NONE;
            }
        }
    }
    
    // GIVING FAVOR OVERLAY
    if (client.action_mode == MODE_GIVE_FAVOR && !confirmVisible) {
        DrawRectangle(0, 0, sw, sh, (Color){100,20,20,150});
        char req_text[128]; snprintf(req_text, sizeof(req_text), T(TXT_GIVE_FAVOR), client.favor_requester);
        DrawText(req_text, sw/2 - MeasureText(req_text, 28)/2, sh/3, 28, YELLOW);
    }

    // NOPE WINDOW OVERLAY
    if (client.nope_window_active) {
        if (GetTime() > client.nope_timer_end) {
            client.nope_window_active = 0;
        } else {
            DrawRectangle(0, 0, sw, sh, (Color){150,0,0,100});
            DrawText(T(TXT_NOPE_WINDOW), sw/2 - MeasureText(T(TXT_NOPE_WINDOW), 24)/2, sh/2 - 100, 24, WHITE);
            DrawText(client.nope_info, sw/2 - MeasureText(client.nope_info, 20)/2, sh/2 - 60, 20, YELLOW);
            
            // Check if player has a nope card
            pthread_mutex_lock(&client_lock);
            int nope_id = -1;
            Cartas* c = Inicio(client.player ? client.player->hand : NULL);
            while (c) { if (c->type == NAO) { nope_id = c->card_id; break; } c = c->sig; }
            pthread_mutex_unlock(&client_lock);
            
            if (nope_id != -1) {
                int btnW = 200, btnH = 60;
                Rectangle nopeBtn = {sw/2 - btnW/2, sh/2, btnW, btnH};
                Vector2 m = GetMousePosition();
                int hov = CheckCollisionPointRec(m, nopeBtn);
                DrawRectangleRounded(nopeBtn, 0.2f, 6, hov ? RED : MAROON);
                DrawRectangleLinesEx(nopeBtn, 3, WHITE);
                DrawText(T(TXT_NOPE_BTN), sw/2 - MeasureText(T(TXT_NOPE_BTN), 24)/2, sh/2 + 18, 24, WHITE);
                
                if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    char pay[32]; snprintf(pay, sizeof(pay), "%d", nope_id);
                    SendMessageNet("PLAY_NAO", pay);
                    client.nope_window_active = 0;
                }
            }
        }
    }

    // EXPLODING KITTEN ANIMATION
    if (client.show_kitten_anim) {
        if (GetTime() > client.kitten_anim_end) {
            client.show_kitten_anim = 0;
        } else {
            DrawRectangle(0, 0, sw, sh, (Color){0,0,0,200});
            float scale = 1.0f + sin(GetTime() * 10) * 0.1f;
            int imgW = 200 * scale; int imgH = 300 * scale;
            if (cardArt.loaded[EXPLODING_KITTEN]) {
                Rectangle src = {0, 0, cardArt.textures[EXPLODING_KITTEN].width, cardArt.textures[EXPLODING_KITTEN].height};
                Rectangle dst = {sw/2, sh/2, imgW, imgH};
                DrawTexturePro(cardArt.textures[EXPLODING_KITTEN], src, dst, (Vector2){imgW/2, imgH/2}, 0, WHITE);
            }
            char msg[128]; snprintf(msg, sizeof(msg), "%s DREW AN EXPLODING KITTEN!", client.kitten_victim);
            DrawText(msg, sw/2 - MeasureText(msg, 32)/2, sh/2 + imgH/2 + 20, 32, RED);
        }
    }

    if (client.feedback_timer > GetTime()) {
        int fw = MeasureText(client.feedback_msg, 18);
        DrawRectangle(sw/2-fw/2-12,sh/2-22,fw+24,40,(Color){0,0,0,190});
        DrawText(client.feedback_msg,sw/2-fw/2,sh/2-13,18,YELLOW);
    }
    DrawLanguageButton(sw);
    EndDrawing();
}

void DrawMainMenu(int sw, int sh) {
    BeginDrawing(); ClearBackground((Color){30,30,30,255});
    int tw = MeasureText("EXPLODING KITTENS", 44); DrawText("EXPLODING KITTENS", sw/2-tw/2, sh/6, 44, WHITE);
    int bw=320, bh=62, bx=sw/2-bw/2, by=sh/2-bh-40;
    DrawRectangleRounded((Rectangle){bx,by,bw,bh}, 0.3f, 6, (Color){0,120,215,255});
    DrawText(T(TXT_CREATE_GAME),bx+bw/2-MeasureText(T(TXT_CREATE_GAME),22)/2,by+bh/2-11,22,WHITE);
    DrawRectangleRounded((Rectangle){bx,by+bh+18,bw,bh}, 0.3f, 6, (Color){0,120,215,255});
    DrawText(T(TXT_JOIN_GAME),bx+bw/2-MeasureText(T(TXT_JOIN_GAME),22)/2,by+bh+18+bh/2-11,22,WHITE);
    DrawRectangleRounded((Rectangle){bx,by+2*(bh+18),bw,bh}, 0.3f, 6, (Color){200,50,50,255});
    DrawText(T(TXT_EXIT),bx+bw/2-MeasureText(T(TXT_EXIT),22)/2,by+2*(bh+18)+bh/2-11,22,WHITE);
    DrawLanguageButton(sw); EndDrawing();
}

void DrawWaitingScreen(int sw, int sh) {
    BeginDrawing(); ClearBackground((Color){30,30,30,255});
    DrawText(T(TXT_WAITING_TITLE),sw/2-MeasureText(T(TXT_WAITING_TITLE),32)/2,sh/3,32,WHITE);
    char rt[64]; snprintf(rt,sizeof(rt),T(TXT_ROOM_ID),client.room_id);
    DrawText(rt,sw/2-MeasureText(rt,28)/2,sh/2,28,YELLOW);
    char pt[64]; snprintf(pt,sizeof(pt),T(TXT_PLAYERS_COUNT),client.client_player_count);
    DrawText(pt,sw/2-MeasureText(pt,22)/2,sh/2+50,22,WHITE);
    DrawText(T(TXT_PRESS_S_START),sw/2-MeasureText(T(TXT_PRESS_S_START),18)/2,sh/2+120,18,LIGHTGRAY);
    DrawLanguageButton(sw); EndDrawing();
}

void DrawGameOver(int sw, int sh) {
    BeginDrawing(); ClearBackground((Color){30,30,30,255});
    DrawText(T(TXT_GAME_OVER),sw/2-MeasureText(T(TXT_GAME_OVER),52)/2,sh/3,52,YELLOW);
    char wt[200]; snprintf(wt,sizeof(wt),T(TXT_WINNER),client.winner_name);
    DrawText(wt,sw/2-MeasureText(wt,30)/2,sh/2,30,WHITE);
    DrawText(T(TXT_PRESS_SPACE),sw/2-MeasureText(T(TXT_PRESS_SPACE),20)/2,sh-100,20,LIGHTGRAY);
    DrawLanguageButton(sw); EndDrawing();
}

int ConnectToServer(const char* host, int port) {
    int sock=socket(AF_INET,SOCK_STREAM,0); if(sock<0) return -1;
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,host,&addr.sin_addr)<=0) return -1;
    if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0) return -1;
    return sock;
}

int main(void) {
    int sw=1150, sh=780;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(sw,sh,"Exploding Kittens - Online"); SetTargetFPS(60);
    
    // Ignore dead sockets crashing the client application
    signal(SIGPIPE, SIG_IGN); 

    memset(&cardArt, 0, sizeof(CardArtTextures));
    for (int i = 0; i < 13; i++) {
        cardArt.textures[i] = LoadTexture(cardImagePaths[i]);
        cardArt.loaded[i] = (cardArt.textures[i].width > 0) ? 1 : 0;
    }
    
    memset(&client,0,sizeof(client));
    client.flag_usa = LoadTexture("assets/flag_usa.jpg");
    client.flag_chile = LoadTexture("assets/flag_chile.jpg");
    client.flags_loaded = (client.flag_usa.width > 0 && client.flag_chile.width > 0) ? 1 : 0;

    client.is_connected=0; client.game_ended=0; client.current_state=LOGIN;
    client.action_mode=MODE_NONE; client.selected_card_id_1=-1; client.selected_card_id_2=-1;
    client.discard_top_type=-1; client.current_turn_player_id=-2; client.my_player_id=-1;
    client.nope_window_active=0; client.show_kitten_anim=0; client.language=0;
    strcpy(client.discard_top_name,"Empty");

    char username[50]=""; int username_idx=0;
    char room_code[20]=""; int room_code_idx=0;
    char server_ip[50]="44.214.143.12"; int ip_idx=9;
    int active_input = 0;

    while (!WindowShouldClose()) {
        sw=GetScreenWidth(); sh=GetScreenHeight();

        switch(client.current_state) {
        // ======== LOGIN ========
        case LOGIN: {
            // Toggle active input field with TAB
            if (IsKeyPressed(KEY_TAB)) {
                active_input = (active_input == 0) ? 1 : 0;
            }

            // Handle typing based on which input is active
            if (active_input == 0) { // Username logic
                if(IsKeyPressed(KEY_BACKSPACE) && username_idx > 0) username[--username_idx]='\0';
                else if(username_idx < 49){
                    int k = GetCharPressed();
                    if(k >= 32 && k <= 126){ username[username_idx++]=(char)k; username[username_idx]='\0'; }
                }
            } else { // IP logic
                if(IsKeyPressed(KEY_BACKSPACE) && ip_idx > 0) server_ip[--ip_idx]='\0';
                else if(ip_idx < 49){
                    int k = GetCharPressed();
                    // Allow numbers and dots for IP addresses
                    if((k >= '0' && k <= '9') || k == '.'){ server_ip[ip_idx++]=(char)k; server_ip[ip_idx]='\0'; }
                }
            }

            // Connect when ENTER is pressed and both fields have data
            if (IsKeyPressed(KEY_ENTER) && username_idx > 0 && ip_idx > 0) {
                int sock = ConnectToServer(server_ip, 8080); // Now uses dynamic IP
                if(sock >= 0){
                    client.socket=sock; client.is_connected=1;
                    client.player=CreatePlayer(0,username); send(sock,username,strlen(username),0);
                    char mt[256],pl[256]; RecvMessageNet(mt,pl);
                    pthread_create(&client.message_thread,NULL,MessageHandlerThread,NULL);
                    client.current_state=MAIN_MENU;
                } else {
                    SetFeedback("Connection Failed! Check IP.");
                }
            }

            BeginDrawing(); ClearBackground((Color){25,25,25,255});
            DrawText("EXPLODING KITTENS",sw/2-MeasureText("EXPLODING KITTENS",52)/2,sh/4 - 40,52,WHITE);

            // Draw Username Input
            Color userBoxColor = (active_input == 0) ? YELLOW : WHITE;
            DrawText(T(TXT_ENTER_USER),sw/2-140,sh/2-60,20,LIGHTGRAY);
            DrawRectangleRounded((Rectangle){sw/2-160.f,sh/2-30.f,320,44},0.3f,6,(Color){55,55,55,255});
            DrawRectangleLinesEx((Rectangle){sw/2-160.f,sh/2-30.f,320,44},2,userBoxColor);
            DrawText(username,sw/2-150,sh/2-18,22,userBoxColor);

            // Draw IP Input
            Color ipBoxColor = (active_input == 1) ? YELLOW : WHITE;
            const char* ipLabel = (client.language == 0) ? "Server IP Address:" : "Direccion IP del Servidor:";
            DrawText(ipLabel,sw/2-140,sh/2+30,20,LIGHTGRAY);
            DrawRectangleRounded((Rectangle){sw/2-160.f,sh/2+60.f,320,44},0.3f,6,(Color){55,55,55,255});
            DrawRectangleLinesEx((Rectangle){sw/2-160.f,sh/2+60.f,320,44},2,ipBoxColor);
            DrawText(server_ip,sw/2-150,sh/2+72,22,ipBoxColor);

            DrawText("Press TAB to switch fields",sw/2-MeasureText("Press TAB to switch fields",14)/2,sh/2+120,14,GRAY);
            DrawText(T(TXT_PRESS_ENTER_CONN),sw/2-MeasureText(T(TXT_PRESS_ENTER_CONN),15)/2,sh/2+150,15,GRAY);
            
            if (client.feedback_timer > GetTime()) {
                DrawText(client.feedback_msg, sw/2-MeasureText(client.feedback_msg,18)/2, sh/2+180, 18, RED);
            }

            DrawLanguageButton(sw); 
            EndDrawing(); 
            break;
        }

        case MAIN_MENU: {
            Vector2 mouse=GetMousePosition(); int bw=320,bh=62,bx=sw/2-bw/2,by=sh/2-bh-40;
            if(CheckCollisionPointRec(mouse,(Rectangle){bx,by,bw,bh})&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                SendMessageNet("CREATE_GAME",""); client.current_state=WAITING_PLAYERS; }
            if(CheckCollisionPointRec(mouse,(Rectangle){bx,by+bh+18,bw,bh})&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                room_code_idx=0;room_code[0]='\0'; client.current_state=JOIN_GAME; }
            if(CheckCollisionPointRec(mouse,(Rectangle){bx,by+2*(bh+18),bw,bh})&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                CloseWindow(); return 0;
            }
            DrawMainMenu(sw,sh); break;
        }

        case JOIN_GAME: {
            if(IsKeyPressed(KEY_BACKSPACE)&&room_code_idx>0) room_code[--room_code_idx]='\0';
            else if(room_code_idx<19){int k=GetCharPressed();if(k>='0'&&k<='9'){room_code[room_code_idx++]=(char)k;room_code[room_code_idx]='\0';}}
            if(IsKeyPressed(KEY_ENTER)&&room_code_idx>0){ SendMessageNet("JOIN_GAME",""); send(client.socket,room_code,strlen(room_code),0); room_code_idx=0;room_code[0]='\0'; }
            if(IsKeyPressed(KEY_ESCAPE)) client.current_state=MAIN_MENU;
            BeginDrawing(); ClearBackground((Color){30,30,30,255});
            DrawText(T(TXT_JOIN_TITLE),sw/2-MeasureText(T(TXT_JOIN_TITLE),40)/2,sh/4,40,WHITE);
            DrawText(T(TXT_ENTER_ROOM),sw/2-140,sh/2-30,20,LIGHTGRAY);
            DrawRectangleRounded((Rectangle){sw/2-120.f,sh/2+5.f,240,44},0.3f,6,(Color){55,55,55,255});
            DrawRectangleLinesEx((Rectangle){sw/2-120.f,sh/2+5.f,240,44},2,YELLOW);
            DrawText(room_code,sw/2-110,sh/2+17,24,YELLOW);
            DrawLanguageButton(sw); EndDrawing(); break;
        }

        case WAITING_PLAYERS: {
            if(IsKeyPressed(KEY_S)) SendMessageNet("START_GAME","");
            DrawWaitingScreen(sw,sh); break;
        }

        case GAME_PLAY: {
            int isMyTurn=(client.current_turn_player_id==client.my_player_id);
            Vector2 mouse=GetMousePosition(); int btnW=155,btnH=52,btnBaseY=sh-58; int playX=sw/2-btnW-8, drawX=sw/2+8;

            if(isMyTurn && client.action_mode != MODE_SELECT_TARGET && client.action_mode != MODE_GIVE_FAVOR && CheckCollisionPointRec(mouse,(Rectangle){playX,btnBaseY,btnW,btnH}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                client.action_mode = (client.action_mode==MODE_PLAY_CARD) ? MODE_NONE : MODE_PLAY_CARD;
                client.selected_card_id_1=-1; client.selected_card_id_2=-1;
            }

            if(isMyTurn && client.action_mode != MODE_SELECT_TARGET && client.action_mode != MODE_GIVE_FAVOR && CheckCollisionPointRec(mouse,(Rectangle){drawX,btnBaseY,btnW,btnH}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                SendMessageNet("DRAW_CARD","");
                client.action_mode=MODE_NONE; client.selected_card_id_1=-1; client.selected_card_id_2=-1;
            }

            DrawGameplay(sw,sh); break;
        }

        case GAME_OVER: {
            if(IsKeyPressed(KEY_SPACE)){
                // CRITICAL FIX: Safe exit process on game reset clears dead sockets to prevent crashes
                if(client.is_connected){ close(client.socket); client.is_connected=0; }
                client.current_state=LOGIN; 
                client.game_ended=0; 
                client.action_mode=MODE_NONE;
                client.selected_card_id_1=-1; 
                client.selected_card_id_2=-1; 
                client.discard_top_type=-1;
                client.show_kitten_anim=0; 
                client.nope_window_active=0;
                strcpy(client.discard_top_name,"Empty");
            }
            DrawGameOver(sw,sh); break;
        }

        default: break;
        }
    }

    for (int i = 0; i < 13; i++) { if (cardArt.loaded[i]) UnloadTexture(cardArt.textures[i]); }
    if (client.flags_loaded) { UnloadTexture(client.flag_usa); UnloadTexture(client.flag_chile); }
    if(client.is_connected){ close(client.socket); if(client.player) FreePlayer(client.player); }
    CloseWindow(); return 0;
}