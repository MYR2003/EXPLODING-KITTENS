#include <stdio.h>
#include <stdlib.h>
#include "cartas.h"

// Metodos
// Metodo para crear un nodo de la estructura
Cartas * Crear(char* nombre, char* descripcion){
    Cartas * temp = (Cartas *)malloc(sizeof(Cartas));
    temp->nombre = nombre;
    temp->descripcion = descripcion;
    temp->sig = NULL;
    temp->ant = NULL;
    return temp;
}

// Metodo para ir al primer nodo
Cartas * Inicio(Cartas * nodo) {
    if (nodo->ant == NULL) {
        return nodo;
    }
    return Inicio(nodo->ant);
}

// Metodo para ir al ultimo nodo
Cartas * Final(Cartas * nodo) {
    if (nodo->sig == NULL) {
        return nodo;
    }
    return Final(nodo->sig);
}

// Metodo para insertar una nueva carta al final 
Cartas * Insert(Cartas * carta, Cartas * nodo) {
    if (nodo == NULL) {
        return carta;
    } 
    if (nodo->sig == NULL) {
        nodo->sig = carta;
        carta->ant = nodo;
        nodo = Inicio(nodo); // Devuelvo el nodo entregado al inicio antes de retornarlo
        return nodo;
    }
    return Insert(carta, nodo->sig);
}

// Metodo para insertar una carta en la posicion actual
Cartas * InsertActual(Cartas * carta, Cartas * nodo) {
    if (nodo == NULL) {
        return carta;
    }
    if (nodo->ant == NULL) {
        nodo->ant = carta;
        carta->sig = nodo;
        nodo = Inicio(nodo);
        return nodo;
    }
    else {
        Cartas * temp = nodo->ant;
        temp->sig = carta;
        nodo->ant = carta;
        carta->ant = temp;
        carta->sig = nodo;
        nodo = Inicio(nodo);
        return nodo;
    }
}

// Metodo para quitar la carta final
Cartas * Pop(Cartas ** nodo) {
    if (nodo == NULL) {
        return NULL;
    }
    Cartas * final = *nodo;
    final = Final(final);

    if (final->ant != NULL) {
        final->ant->sig = NULL;
    } else {
        *nodo = NULL;
    }
    final->ant = NULL;
    final->sig = NULL;
    return final;
}

// Metodo para quitar la carta en la posicion actual
Cartas * PopActual(Cartas ** nodo) {
    if (*nodo == NULL) {
        return NULL;
    }

    Cartas * actual = *nodo;

    if (actual->ant == NULL && actual->sig == NULL) {
        *nodo = NULL;
        return actual;
    }
    actual->ant->sig = actual->sig;
    actual->sig->ant = actual->ant;
    actual->ant = NULL;
    actual->sig = NULL;
    return actual;
}

// Metodo que cuenta la cantidad de cartas total
int CountCartas(Cartas * nodo, int primeraVez){
    if (nodo == NULL) {
        return 0;
    }
    if (nodo->ant != NULL && primeraVez == 1) {
        nodo = Inicio(nodo);
    }
    return CountCartas(nodo->sig, 0) + 1;
}

// Metodo que imprime todas las cartas de la estructura
void PrintAllCartas(Cartas * nodo, int primeraVez) {
    if (nodo == NULL) {
        if (primeraVez == 1) {
            printf("No hay cartas\n");
        }
        return;
    }
    if (nodo->ant != NULL && primeraVez == 1) {
        nodo = Inicio(nodo);
    }
    printf("Carta: %s\n", nodo->nombre);
    printf("Descripcion: %s\n", nodo->descripcion);
    return PrintAllCartas(nodo->sig, 0);
}

// Metodo que imprime una carta de la estructura
void PrintCard(Cartas * nodo) {
    if (nodo == NULL) {
        printf("Ninguna carta seleccionada\n");
        return;
    }
    printf("Carta: %s\n", nodo->nombre);
    printf("Descripcion: %s\n", nodo->descripcion);
    return;
}