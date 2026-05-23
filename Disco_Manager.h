#pragma once
#include <string>
#include "Tabla.h"
#include <map>
#include <any>


class Disco_Manager {

public:
    int Platos_Cantidad;
    int Pistas_por_Plato;
    int Sectores_por_Pista;
    int Capacidad_de_Sectores;
    bool Disco_Esta_Lleno = false;
    std::string Ruta_Predefinida;
    std::map<std::string, Tabla> tablas_cargadas; 

    Disco_Manager(int Platos, int Pistas, int Sectores, int Capacidad_De_Sector);

    bool Creacion_de_Disco_en_Memoria();
    void Borrar_Disco_Duro();
    void Asignar_Ruta(std::string Ruta);
    void CrearTablaDesdeArchivo(const std::string& ruta_archivo_sql);
    void InsertarFilasDesdeCSV(const std::string& ruta_csv, const std::string& nombre_tabla);
    void GuardarFilaEnSectores(const std::vector<uint8_t>& fila_en_bytes, const std::string& nombre_tabla);
    std::vector<std::any> LeerFilaPorNumero(const std::string& nombre_tabla, int numero_fila);
    void MostrarFilaCasteada(const Tabla& tabla, const std::vector<std::any>& fila);
    std::vector<std::vector<std::string>> ExecuteSQLQuery(const std::string& selectStr, const std::string& fromStr, const std::string& whereStr);
    void ActualizarCantidadFilas(const std::string& nombre_tabla, int cantidad_a_sumar);

};