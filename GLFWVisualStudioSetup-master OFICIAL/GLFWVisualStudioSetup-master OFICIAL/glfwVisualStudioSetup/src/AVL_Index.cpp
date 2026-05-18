#include "AVL_Index.h"
#include <algorithm>

AVLTree::AVLTree() : raiz(nullptr) {}

AVLTree::~AVLTree() {
    liberar(raiz);
}

void AVLTree::liberar(AVLNode* nodo) {
    if (nodo) {
        liberar(nodo->izquierda);
        liberar(nodo->derecha);
        delete nodo;
    }
}

int AVLTree::obtenerAltura(AVLNode* nodo) {
    return nodo ? nodo->altura : 0;
}

int AVLTree::obtenerBalance(AVLNode* nodo) {
    return nodo ? obtenerAltura(nodo->izquierda) - obtenerAltura(nodo->derecha) : 0;
}

AVLNode* AVLTree::rotarDerecha(AVLNode* y) {
    AVLNode* x = y->izquierda;
    AVLNode* T2 = x->derecha;

    x->derecha = y;
    y->izquierda = T2;

    y->altura = std::max(obtenerAltura(y->izquierda), obtenerAltura(y->derecha)) + 1;
    x->altura = std::max(obtenerAltura(x->izquierda), obtenerAltura(x->derecha)) + 1;

    return x;
}

AVLNode* AVLTree::rotarIzquierda(AVLNode* x) {
    AVLNode* y = x->derecha;
    AVLNode* T2 = y->izquierda;

    y->izquierda = x;
    x->derecha = T2;

    x->altura = std::max(obtenerAltura(x->izquierda), obtenerAltura(x->derecha)) + 1;
    y->altura = std::max(obtenerAltura(y->izquierda), obtenerAltura(y->derecha)) + 1;

    return y;
}

AVLNode* AVLTree::insertar(AVLNode* nodo, const std::string& clave, const std::string& direccion) {
    if (!nodo)
        return new AVLNode(clave, direccion);

    if (clave < nodo->clave)
        nodo->izquierda = insertar(nodo->izquierda, clave, direccion);
    else if (clave > nodo->clave)
        nodo->derecha = insertar(nodo->derecha, clave, direccion);
    else {
        nodo->direcciones.push_back(direccion);
        return nodo;
    }

    nodo->altura = 1 + std::max(obtenerAltura(nodo->izquierda), obtenerAltura(nodo->derecha));
    int balance = obtenerBalance(nodo);

    // Rotaciones
    if (balance > 1 && clave < nodo->izquierda->clave)
        return rotarDerecha(nodo);

    if (balance < -1 && clave > nodo->derecha->clave)
        return rotarIzquierda(nodo);

    if (balance > 1 && clave > nodo->izquierda->clave) {
        nodo->izquierda = rotarIzquierda(nodo->izquierda);
        return rotarDerecha(nodo);
    }

    if (balance < -1 && clave < nodo->derecha->clave) {
        nodo->derecha = rotarDerecha(nodo->derecha);
        return rotarIzquierda(nodo);
    }

    return nodo;
}

void AVLTree::insertar(const std::string& clave, const std::string& direccion) {
    raiz = insertar(raiz, clave, direccion);
}

AVLNode* AVLTree::buscar(const std::string& clave) const {
    AVLNode* actual = raiz;
    while (actual) {
        if (clave == actual->clave)
            return actual;
        else if (clave < actual->clave)
            actual = actual->izquierda;
        else
            actual = actual->derecha;
    }
    return nullptr;
}

