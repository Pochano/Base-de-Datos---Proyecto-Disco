#pragma once
#include <string>
#include <vector>
#include <iostream>

struct AVLNode {
    std::string clave;
    std::vector<std::string> direcciones; 
    int altura;
    AVLNode* izquierda;
    AVLNode* derecha;

    AVLNode(const std::string& key, const std::string& direccion)
        : clave(key), direcciones{ direccion }, altura(1), izquierda(nullptr), derecha(nullptr) {}
};

class AVLTree {
private:
    AVLNode* raiz;

    AVLNode* insertar(AVLNode* nodo, const std::string& clave, const std::string& direccion);
    AVLNode* rotarDerecha(AVLNode* y);
    AVLNode* rotarIzquierda(AVLNode* x);
    int obtenerAltura(AVLNode* nodo);
    int obtenerBalance(AVLNode* nodo);
    void liberar(AVLNode* nodo);
    void recorrerInOrder(AVLNode* nodo) const {
        if (!nodo) return;

        recorrerInOrder(nodo->izquierda);

        std::cout << "Clave: " << nodo->clave << " -> Direcciones: ";
        for (const auto& dir : nodo->direcciones) {
            std::cout << dir << " ";
        }
        std::cout << "\n";

        recorrerInOrder(nodo->derecha);
    }

public:
    AVLTree();
    ~AVLTree();

    void insertar(const std::string& clave, const std::string& direccion);
    AVLNode* buscar(const std::string& clave) const;
    void recorrerInOrder() const {
        recorrerInOrder(raiz);  // Asumiendo que "raiz" es tu nodo raíz
    }
    
};
