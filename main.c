// Librerias Front
#include <raylib.h>

// Librerias Back
#include <qrencode.h>

// Librerias Genericas
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Mis librerias
#include "cartas.h"
#include "funciones.h"

/*
- GetRandomValue(min, max) # From Raylib
*/

typedef enum { MENU_PRINCIPAL, CREAR_PARTIDA, UNIRME_PARTIDA, CONFIGURACIONES, PARTIDA, SALIR } GameState;

int main(void) {
    // Inicializacion
    // --------------
    int screenWidth = 800;
    int screenHeight = 450;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Exploding Kittens - myr version");

    SetTargetFPS(60);

    // Variables
    // ---------
    // Pantalla
    GameState currentState = MENU_PRINCIPAL;

    // Configuraciones de partida
    int cantidadJugadores = 2;
    bool desactivacionesIniciales = true;

    // Configuraciones de aplicacion

    // Bucle del juego
    // ---------------
    while (!WindowShouldClose() && currentState != SALIR)
    {
        // Actualizacion de las variables
        // ------------------------------
        // Tamaño de pantalla
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        // Posicion del mouse
        Vector2 mousePoint = GetMousePosition();

        // Fonts
        int fontGrande = screenWidth/40 + screenHeight/20;
        int fontMediano = screenWidth/80 + screenHeight/40;
        int fontPeque = screenWidth/120 + screenHeight/60;

        // Botones 
        // Boton mediano
        int btnMedianoWidth = screenWidth/3;
        int btnMedianoHeight = screenHeight/8;

        // Boton chico
        int btnChicoWidth = screenHeight/16;
        int btnChicoHeight = btnChicoWidth;

        // Posiciones
        // Titulos
        int yPosTitulo = screenHeight/8;

        switch (currentState)
        {
        case MENU_PRINCIPAL:
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnMedianoWidth/2, 6*screenHeight/20, btnMedianoWidth, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                currentState = CREAR_PARTIDA;
            }
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnMedianoWidth/2, 9*screenHeight/20, btnMedianoWidth, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                currentState = UNIRME_PARTIDA;
            }
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnMedianoWidth/2, 12*screenHeight/20, btnMedianoWidth, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                currentState = CONFIGURACIONES;
            }
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnMedianoWidth/2, 15*screenHeight/20, btnMedianoWidth, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                currentState = SALIR;
            }
            break;
        
        case CREAR_PARTIDA:
            // Cantidad de jugadores
            if (CheckCollisionPointRec(mousePoint, (Rectangle){3*screenWidth/4 - btnChicoWidth/2, screenHeight/4, btnChicoWidth, btnChicoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && cantidadJugadores < 5) {
                cantidadJugadores++;
            }
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/4 - btnChicoWidth/2, screenHeight/4, btnChicoWidth, btnChicoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && cantidadJugadores > 2) {
                cantidadJugadores--;
            }

            // Desactivaciones iniciales
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnChicoWidth/2, 3*screenHeight/8, MeasureText("Desactivaciones iniciales desactivadas", fontMediano), btnChicoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && desactivacionesIniciales){
                desactivacionesIniciales = false;
            }
            else if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/2 - btnChicoWidth/2, 3*screenHeight/8, MeasureText("Desactivaciones iniciales desactivadas", fontMediano), btnChicoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !desactivacionesIniciales){
                desactivacionesIniciales = true;
            }

            // Cancelar
            if (CheckCollisionPointRec(mousePoint, (Rectangle){screenWidth/7 - 3*btnMedianoWidth/8, 7*screenHeight/8 - btnMedianoHeight/2, 3*btnMedianoWidth/4, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = MENU_PRINCIPAL;
            }
            if (CheckCollisionPointRec(mousePoint, (Rectangle){6*screenWidth/7 - 3*btnMedianoWidth/8, 7*screenHeight/8 - btnMedianoHeight/2, 3*btnMedianoWidth/4, btnMedianoHeight}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                currentState = PARTIDA;
            }
            break;
        
        case UNIRME_PARTIDA:
            break;
        case CONFIGURACIONES:
            break;
        default:
            break;
        }

        // Dibujo
        // ------
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText(TextFormat("FPS: %i", GetFPS()), screenWidth - MeasureText("FPS: 60", 20) - 5, 5, 20, BLACK);

            switch (currentState) {
                case MENU_PRINCIPAL:
                    // Titulo
                    DrawText("Exploding Kittens - myr version", screenWidth/2 - MeasureText("Exploding Kittens - myr version", fontGrande)/2, yPosTitulo, fontGrande, BLACK);
                    
                    // Botones
                    DrawRectangle(screenWidth/2 - btnMedianoWidth/2, 6*screenHeight/20, btnMedianoWidth, btnMedianoHeight, BLACK);
                    DrawText("Crear partida", screenWidth/2 - MeasureText("Crear partida", fontMediano)/2, 6*screenHeight/20 + btnMedianoHeight/2 - fontMediano/2, fontMediano, WHITE);

                    DrawRectangle(screenWidth/2 - btnMedianoWidth/2, 9*screenHeight/20, btnMedianoWidth, btnMedianoHeight, BLACK);
                    DrawText("Unirme a una partida", screenWidth/2 - MeasureText("Unirme a una partida", fontMediano)/2, 9*screenHeight/20 + btnMedianoHeight/2 - fontMediano/2, fontMediano, WHITE);

                    DrawRectangle(screenWidth/2 - btnMedianoWidth/2, 12*screenHeight/20, btnMedianoWidth, btnMedianoHeight, BLACK);
                    DrawText("Configuraciones", screenWidth/2 - MeasureText("Configuraciones", fontMediano)/2, 12*screenHeight/20 + btnMedianoHeight/2 - fontMediano/2, fontMediano, WHITE);

                    DrawRectangle(screenWidth/2 - btnMedianoWidth/2, 15*screenHeight/20, btnMedianoWidth, btnMedianoHeight, BLACK);
                    DrawText("Salir", screenWidth/2 - MeasureText("Salir", fontMediano)/2, 15*screenHeight/20 + btnMedianoHeight/2 - fontMediano/2, fontMediano, WHITE);

                    break;
                case CREAR_PARTIDA:
                    DrawText("Configuraciones de la partida", screenWidth/2 - MeasureText("Configuraciones de la partida", fontGrande)/2, yPosTitulo, fontGrande, BLACK);

                    // Botones
                    DrawRectangle(3*screenWidth/4 - btnChicoWidth/2, screenHeight/4, btnChicoWidth, btnChicoHeight, GRAY);
                    DrawText("+", 3*screenWidth/4 - MeasureText("+", fontPeque)/2, screenHeight/4 + btnChicoHeight/2 - fontPeque/2, fontPeque, WHITE);

                    DrawText(TextFormat("Cantidad de jugadores -%d-", cantidadJugadores), screenWidth/2 - MeasureText("Cantidad de jugadores 2", fontMediano)/2, screenHeight/4, fontMediano, BLUE);

                    DrawRectangle(screenWidth/4 - btnChicoWidth/2, screenHeight/4, btnChicoWidth, btnChicoHeight, GRAY);
                    DrawText("-", screenWidth/4 - MeasureText("-", fontPeque)/2, screenHeight/4 + btnChicoHeight/2 - fontPeque/2, fontPeque, WHITE);

                    if (desactivacionesIniciales) {
                        DrawText("Desactivaciones iniciales activadas", screenWidth/2 - MeasureText("Desactivaciones iniciales activadas", fontMediano)/2, 3*screenHeight/8, fontMediano, BLUE);
                    } else {
                        DrawText("Desactivaciones iniciales desactivadas", screenWidth/2 - MeasureText("Desactivaciones iniciales desactivadas", fontMediano)/2, 3*screenHeight/8, fontMediano, BLUE);
                    }

                    DrawRectangle(screenWidth/7 - 3*btnMedianoWidth/8, 7*screenHeight/8 - btnMedianoHeight/2, 3*btnMedianoWidth/4, btnMedianoHeight, RED);
                    DrawText("Cancelar", screenWidth/7 - MeasureText("Cancelar", fontMediano)/2, 7*screenHeight/8 - fontMediano/2, fontMediano, BLACK);

                    DrawRectangle(6*screenWidth/7 - 3*btnMedianoWidth/8, 7*screenHeight/8 - btnMedianoHeight/2, 3*btnMedianoWidth/4, btnMedianoHeight, GREEN);
                    DrawText("Iniciar", 6*screenWidth/7 - MeasureText("Iniciar", fontMediano)/2, 7*screenHeight/8 - fontMediano/2, fontMediano, BLACK);
                    break;
                case UNIRME_PARTIDA:
                    DrawText("Vuelva pronto", screenWidth/2 - MeasureText("Vuelva pronto", fontGrande)/2, yPosTitulo, fontGrande, BLACK);
                    break;
                case CONFIGURACIONES:
                    DrawText("Configuraciones", screenWidth/2 - MeasureText("Configuraciones", fontGrande)/2, yPosTitulo, fontGrande, BLACK);
                    break;
                case PARTIDA:
                    DrawText("Aca va a estar la partida", screenWidth/2 - MeasureText("Aca va a estar la partida", fontGrande)/2, screenHeight/2 - fontGrande/2, fontGrande, BLACK);
                    break;
                default:
                    break;
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
    
}