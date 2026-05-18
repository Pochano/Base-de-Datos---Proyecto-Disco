#pragma once
#include <vector>
#include <any>
#include <iostream>

struct NodoAVL {
    std::any clave; // Puede ser int, char o std::string
    std::vector<std::string> direcciones; // Ej: ["0/1/3/0-16", "0/2/0/0-16"]
    int altura;
    NodoAVL* izquierda;
    NodoAVL* derecha;

    NodoAVL(std::any valor, const std::string& direccion)
        : clave(valor), direcciones({ direccion }), altura(1), izquierda(nullptr), derecha(nullptr) {}
};

class AVLIndex {
private:
    NodoAVL* raiz;
    std::string tipo_clave; // "INT", "CHAR", "VARCHAR"

    int altura(NodoAVL* nodo);
    int balanceFactor(NodoAVL* nodo);
    NodoAVL* rotarDerecha(NodoAVL* y);
    NodoAVL* rotarIzquierda(NodoAVL* x);
    NodoAVL* insertar(NodoAVL* nodo, std::any clave, const std::string& direccion);
    int compararClaves(const std::any& a, const std::any& b);

public:
    AVLIndex(const std::string& tipo) : raiz(nullptr), tipo_clave(tipo) {}
    void insertar(std::any clave, const std::string& direccion);
    void imprimirEnOrden(NodoAVL* nodo);
    void imprimir() { imprimirEnOrden(raiz); }
};