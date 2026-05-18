#include "AVL_Tree.h"


void AVLIndex::insertar(std::any clave, const std::string& direccion) {
    raiz = insertar(raiz, clave, direccion);
}

NodoAVL* AVLIndex::insertar(NodoAVL* nodo, std::any clave, const std::string& direccion) {
    if (!nodo) return new NodoAVL(clave, direccion);

    int comp = compararClaves(clave, nodo->clave);
    if (comp < 0) {
        nodo->izquierda = insertar(nodo->izquierda, clave, direccion);
    }
    else if (comp > 0) {
        nodo->derecha = insertar(nodo->derecha, clave, direccion);
    }
    else {
        nodo->direcciones.push_back(direccion); // Clave ya existe, agregamos dirección
        return nodo;
    }

    // Actualiza altura
    nodo->altura = 1 + std::max(altura(nodo->izquierda), altura(nodo->derecha));

    int balance = balanceFactor(nodo);

    // Rebalanceo AVL
    if (balance > 1 && compararClaves(clave, nodo->izquierda->clave) < 0)
        return rotarDerecha(nodo);

    if (balance < -1 && compararClaves(clave, nodo->derecha->clave) > 0)
        return rotarIzquierda(nodo);

    if (balance > 1 && compararClaves(clave, nodo->izquierda->clave) > 0) {
        nodo->izquierda = rotarIzquierda(nodo->izquierda);
        return rotarDerecha(nodo);
    }

    if (balance < -1 && compararClaves(clave, nodo->derecha->clave) < 0) {
        nodo->derecha = rotarDerecha(nodo->derecha);
        return rotarIzquierda(nodo);
    }

    return nodo;
}

int AVLIndex::compararClaves(const std::any& a, const std::any& b) {
    if (tipo_clave == "INT") {
        return std::any_cast<int>(a) - std::any_cast<int>(b);
    }
    else if (tipo_clave == "CHAR") {
        return std::any_cast<char>(a) - std::any_cast<char>(b);
    }
    else if (tipo_clave == "VARCHAR") {
        return std::any_cast<std::string>(a).compare(std::any_cast<std::string>(b));
    }
    else {
        throw std::runtime_error("Tipo de clave no soportado en el índice AVL");
    }
}

int AVLIndex::altura(NodoAVL* nodo) {
    return nodo ? nodo->altura : 0;
}

int AVLIndex::balanceFactor(NodoAVL* nodo) {
    return nodo ? altura(nodo->izquierda) - altura(nodo->derecha) : 0;
}

NodoAVL* AVLIndex::rotarDerecha(NodoAVL* y) {
    NodoAVL* x = y->izquierda;
    NodoAVL* T2 = x->derecha;

    x->derecha = y;
    y->izquierda = T2;

    y->altura = std::max(altura(y->izquierda), altura(y->derecha)) + 1;
    x->altura = std::max(altura(x->izquierda), altura(x->derecha)) + 1;

    return x;
}

NodoAVL* AVLIndex::rotarIzquierda(NodoAVL* x) {
    NodoAVL* y = x->derecha;
    NodoAVL* T2 = y->izquierda;

    y->izquierda = x;
    x->derecha = T2;

    x->altura = std::max(altura(x->izquierda), altura(x->derecha)) + 1;
    y->altura = std::max(altura(y->izquierda), altura(y->derecha)) + 1;

    return y;
}

void AVLIndex::imprimirEnOrden(NodoAVL* nodo) {
    if (!nodo) return;
    imprimirEnOrden(nodo->izquierda);
    std::cout << "Clave: ";
    if (tipo_clave == "INT") std::cout << std::any_cast<int>(nodo->clave);
    else if (tipo_clave == "CHAR") std::cout << std::any_cast<char>(nodo->clave);
    else if (tipo_clave == "VARCHAR") std::cout << std::any_cast<std::string>(nodo->clave);
    std::cout << " -> Direcciones: ";
    for (const auto& d : nodo->direcciones) std::cout << d << " | ";
    std::cout << "\n";
    imprimirEnOrden(nodo->derecha);
}