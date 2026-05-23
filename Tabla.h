#pragma once
#include <string>
#include <vector>
#include <tuple>
#include "AVL_Index.h"

class Tabla {
private:
    std::string nombre;
    int cantidad_columnas;
    
    int tamano_total_fila;
    std::vector<std::tuple<std::string, std::string, int, bool>> columnas;
    std::string nombre_pk;
    AVLTree indice_pk; 

public:
    Tabla() = default;
    Tabla(const std::string& ruta_metadata);
    int cantidad_filas;

    const std::string& getNombre() const { return nombre; }
    int getCantidadColumnas() const { return cantidad_columnas; }
    int getCantidadFilas() const { return cantidad_filas; }
    int getTamanoTotalFila() const { return tamano_total_fila; }
    const std::vector<std::tuple<std::string, std::string, int, bool>>& getColumnas() const { return columnas; }
    const std::string& getNombrePK() const { return nombre_pk; }
    AVLTree& getIndicePK() { return indice_pk; }
    void setCantidadFilas(int nuevas_filas) { cantidad_filas = nuevas_filas; }
    void MostrarInformacion() const;
};