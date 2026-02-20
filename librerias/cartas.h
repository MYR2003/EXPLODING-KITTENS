#ifndef CARTAS
#define CARTAS

// Estructura de datos
typedef struct Cartas {
    char* nombre;
    char* descripcion;
    struct Cartas * sig;
    struct Cartas * ant;
}Cartas;

// Metodos
// Metodo para crear un nodo de la estructura
Cartas * Crear(char* nombre, char* descripcion);

// Metodo para ir al primer nodo
Cartas * Inicio(Cartas * nodo);

// Metodo para ir al ultimo nodo
Cartas * Final(Cartas * nodo);

// Metodo para insertar una nueva carta al final 
Cartas * Insert(Cartas * carta, Cartas * nodo);

// Metodo para insertar una carta en la posicion actual
Cartas * InsertActual(Cartas * carta, Cartas * nodo);

// Metodo para quitar la carta final
Cartas * Pop(Cartas ** nodo);

// Metodo para quitar la carta en la posicion actual
Cartas * PopActual(Cartas ** nodo);

// Metodo que cuenta la cantidad de cartas total
int CountCartas(Cartas * nodo, int primeraVez);

// Metodo que imprime todas las cartas de la estructura
void PrintAllCartas(Cartas * nodo, int primeraVez);

// Metodo que imprime una carta de la estructura
void PrintCard(Cartas * nodo);

#endif