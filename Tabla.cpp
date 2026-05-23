#include "Tabla.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Tabla.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <bitset>

Tabla::Tabla(const std::string& ruta_metadata) {
    std::ifstream file(ruta_metadata);
    if (!file.is_open()) {
        std::cerr << "Error al abrir: " << ruta_metadata << "\n";
        return;
    }

    std::string linea;
    std::getline(file, nombre);  // Línea 1
    std::getline(file, linea);   // Línea 2: C:...
    cantidad_columnas = std::stoi(linea.substr(2));

    std::getline(file, linea);   // Línea 3: F:...
    cantidad_filas = std::stoi(linea.substr(2));

    while (std::getline(file, linea)) {
        if (linea.rfind("T:", 0) == 0) {
            tamano_total_fila = std::stoi(linea.substr(2));
            break;
        }

        std::istringstream ss(linea);
        std::string col, tipo, bytes, es_pk_str;
        std::getline(ss, col, ',');
        std::getline(ss, tipo, ',');
        std::getline(ss, bytes, ',');
        std::getline(ss, es_pk_str);

        // Limpiar espacios
        col.erase(col.begin(), std::find_if(col.begin(), col.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        col.erase(std::find_if(col.rbegin(), col.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), col.end());

        tipo.erase(tipo.begin(), std::find_if(tipo.begin(), tipo.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        tipo.erase(std::find_if(tipo.rbegin(), tipo.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), tipo.end());

        bool es_pk = false;
        if (!es_pk_str.empty()) {
            es_pk_str.erase(es_pk_str.begin(), std::find_if(es_pk_str.begin(), es_pk_str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            es_pk_str.erase(std::find_if(es_pk_str.rbegin(), es_pk_str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), es_pk_str.end());
            es_pk = (std::stoi(es_pk_str) == 1);
        }

        columnas.emplace_back(col, tipo, std::stoi(bytes), es_pk);
        if (es_pk) nombre_pk = col;
    }
}

void Tabla::MostrarInformacion() const {
    std::cout << "Tabla: " << nombre << "\n";
    std::cout << "Cantidad de columnas: " << cantidad_columnas << "\n";
    std::cout << "Cantidad de filas: " << cantidad_filas << "\n";
    std::cout << "Tamaño total de fila: " << tamano_total_fila << " bytes\n";
    std::cout << "Columnas:\n";
    for (const auto& [col, tipo, size, es_pk] : columnas) {
        std::cout << " - " << col << " | " << tipo << " | " << size << " bytes";
        if (es_pk) std::cout << " | PRIMARY KEY";
        std::cout << "\n";
    }
}



/*Tabla::Tabla(std::string Direccion_T) {
    Direccion_Tabla = Direccion_T;
}

bool Tabla::Cargar_Datos() {
    std::ifstream Data(Direccion_Tabla + "/Informacion_Tabla.txt");
    if (!Data.is_open()) {
        std::cout << "No se pudo abrir el documento Informacion_Tabla.txt\n";
        return false;
    }
    std::string Linea;
    while (std::getline(Data, Linea)) {
        if (Linea.empty()) continue;

        if (Linea.rfind("Columna:", 0) == 0) {
            std::string Nombre_A, Tipo_A;
            int Tamano_A = 0;
            size_t Posicion_Tipo = Linea.find("Tipo:");
            size_t Posicion_Tamano = Linea.find("Tamaño:");
            if (Posicion_Tipo == std::string::npos || Posicion_Tamano == std::string::npos) continue;
            Nombre_A = Linea.substr(9, Posicion_Tipo - 10);
            Nombre_A = Nombre_A.substr(0, Nombre_A.find_last_not_of(" ,") + 1);
            Tipo_A = Linea.substr(Posicion_Tipo + 6, Posicion_Tamano - Posicion_Tipo - 7);
            Tipo_A = Tipo_A.substr(0, Tipo_A.find_last_not_of(" ,") + 1);
            std::string Tamano_Texto = Linea.substr(Posicion_Tamano + 8);
            Tamano_Texto = Tamano_Texto.substr(0, Tamano_Texto.find_first_of(" "));
            Tamano_A = std::stoi(Tamano_Texto);
            Nombres.push_back(Nombre_A);
            Tipos.push_back(Tipo_A);
            Tamanos.push_back(Tamano_A);
        }
        else if (Linea.rfind("Cantidad total de bytes por fila:", 0) == 0) {
            size_t pos = Linea.find(':');
            std::string Texto = Linea.substr(pos + 1);
            Texto.erase(0, Texto.find_first_not_of(" "));
            Texto = Texto.substr(0, Texto.find_first_of(" "));
            Bytes_x_Fila = std::stoi(Texto);
        }
        else if (Linea.rfind("Cantidad de Columnas:", 0) == 0) {
            size_t pos = Linea.find(':');
            std::string Texto = Linea.substr(pos + 1);
            Texto.erase(0, Texto.find_first_not_of(" "));
            Texto = Texto.substr(0, Texto.find_first_of(" "));
            Numero_Columnas = std::stoi(Texto);
        }
        else if (Linea.rfind("Cantidad de Filas:", 0) == 0) {
            size_t pos = Linea.find(':');
            std::string Texto = Linea.substr(pos + 1);
            Texto.erase(0, Texto.find_first_not_of(" "));
            Texto = Texto.substr(0, Texto.find_first_of(" "));
            Numero_Filas = std::stoi(Texto);
        }
    }
    if (Numero_Columnas != Nombres.size()) {
        std::cout << "Error: El número de columnas no coincide con los datos leídos.\n";
        return false;
    }
    std::cout << "Transferencia de Datos Completada\n";
    Data.close();
    return true;
}


void Tabla::Print_Data() {
    std::cout << "Columnas: " << Numero_Columnas << "\n";
    std::cout << "Bytes por fila: " << Bytes_x_Fila << "\n";
    for (int i = 0; i < Numero_Columnas; ++i) {
        std::cout << " - " << Nombres[i] << " (" << Tipos[i] << ", " << Tamanos[i] << " bytes)\n";
    }
}

void Tabla::Ingresar_Fila(Disco_Manager& disco) {
    std::vector<char> Fila_Binaria;

    for (size_t i = 0; i < Nombres.size(); ++i) {
        std::string entrada;
        std::cout << "Ingrese valor para columna [" << Nombres[i] << "] (" << Tipos[i] << "): ";
        std::getline(std::cin, entrada);

        if (entrada.empty()) {
            std::cout << "Entrada vacía. Intente de nuevo.\n";
            return;
        }

        try {
            if (Tipos[i] == "int") {
                int valor = std::stoi(entrada);
                Fila_Binaria.insert(Fila_Binaria.end(), (char*)&valor, (char*)&valor + sizeof(int));
            }
            else if (Tipos[i] == "float") {
                float valor = std::stof(entrada);
                Fila_Binaria.insert(Fila_Binaria.end(), (char*)&valor, (char*)&valor + sizeof(float));
            }
            else if (Tipos[i] == "char") {
                Fila_Binaria.push_back(entrada[0]);
            }
            else if (Tipos[i] == "bool") {
                bool valor = (entrada == "true" || entrada == "1");
                Fila_Binaria.push_back(valor);
            }
            else if (Tipos[i].rfind("varchar", 0) == 0) {
                size_t ini = Tipos[i].find('(');
                size_t fin = Tipos[i].find(')');
                int max_len = 128;
                if (ini != std::string::npos && fin != std::string::npos && fin > ini) {
                    max_len = std::stoi(Tipos[i].substr(ini + 1, fin - ini - 1));
                }
                std::vector<char> buffer(max_len, '\0');
                std::memcpy(buffer.data(), entrada.c_str(), std::min((int)entrada.size(), max_len));
                Fila_Binaria.insert(Fila_Binaria.end(), buffer.begin(), buffer.end());
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error al convertir la entrada: " << e.what() << "\n";
            return;
        }
    }

    int offset = 0;
    std::string id_sector = disco.Buscar_Sector_Disponible(Fila_Binaria.size(), offset);
    if (id_sector.empty()) {
        std::cout << "No hay espacio disponible\n";
        return;
    }

    std::string path = disco.GenerarRutaSector(id_sector);
    std::ofstream out(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!out) {
        std::cout << "No se pudo abrir el archivo binario del sector.\n";
        return;
    }
    out.seekp(offset);
    out.write(Fila_Binaria.data(), Fila_Binaria.size());
    out.close();

    disco.Actualizar_Offset_Sector(id_sector, Fila_Binaria.size());
    std::cout << "Fila insertada en " << id_sector << " offset: " << offset << "\n";

    // Actualizar número de filas en el archivo
    Numero_Filas++;
    std::ifstream in(Direccion_Tabla + "/Informacion_Tabla.txt");
    std::stringstream buffer;
    std::string linea;
    while (std::getline(in, linea)) {
        if (linea.rfind("Cantidad de Filas:", 0) == 0) {
            buffer << "Cantidad de Filas: " << Numero_Filas << "\n";
        }
        else {
            buffer << linea << "\n";
        }
    }
    in.close();

    std::ofstream out_meta(Direccion_Tabla + "/Informacion_Tabla.txt");
    out_meta << buffer.str();
    out_meta.close();
}*/
