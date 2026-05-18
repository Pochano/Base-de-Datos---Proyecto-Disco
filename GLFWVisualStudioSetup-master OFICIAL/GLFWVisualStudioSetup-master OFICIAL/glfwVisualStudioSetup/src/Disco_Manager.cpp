#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <map>
#include <tuple>
#include <vector>
#include <algorithm>
#include <string>
#include <any>
#include <tuple>
#include <cstdint>
#include <cstring>
#include "AVL_Index.h"
#include "Tabla.h"
#include "Disco_Manager.h"

namespace fs = std::filesystem;
std::string Trim(const std::string& str);
std::vector<std::string> SplitCSVLine(const std::string& line);

Disco_Manager::Disco_Manager(int Platos, int Pistas, int Sectores, int Capacidad_De_Sector)
    : Platos_Cantidad(Platos), Pistas_por_Plato(Pistas), Sectores_por_Pista(Sectores),
    Capacidad_de_Sectores(Capacidad_De_Sector), Ruta_Predefinida("") {}

// Asigna la ruta base para la creación/gestión del disco
void Disco_Manager::Asignar_Ruta(std::string Ruta) {
    Ruta_Predefinida = Ruta;
}

// Crea la estructura de directorios y archivos binarios que simulan el disco
bool Disco_Manager::Creacion_de_Disco_en_Memoria() {
    std::string Ruta = Ruta_Predefinida + "/Disco";
    if (!fs::exists(Ruta)) {
        // Si el disco no existe, procede a crearlo
        fs::create_directory(Ruta);
        fs::create_directory(Ruta + "/MetaData");
        std::ofstream IS_Doc(Ruta + "/MetaData/Informacion_Sectores.txt"); // Archivo para registrar el uso de los sectores
        fs::create_directory(Ruta + "/MetaData/Tablas");

        if (!IS_Doc.is_open()) {
            std::cout << "No se pudo crear el documento Informacion_Sectores.txt\n";
            return false;
        }

        // Crea la jerarquía de directorios (Platos/Pistas/Sectores) y los archivos .bin
        for (int D = 0; D < Platos_Cantidad; ++D) {
            std::string Nombre_Plato = Ruta + "/Plato_" + std::to_string(D);
            fs::create_directory(Nombre_Plato);
            for (int P = 0; P < Pistas_por_Plato; ++P) {
                std::string Nombre_Pista = Nombre_Plato + "/Pista_" + std::to_string(P);
                fs::create_directory(Nombre_Pista);
                for (int S = 0; S < Sectores_por_Pista; ++S) {
                    std::string Nombre_Sector = Nombre_Pista + "/Sector_" + std::to_string(S) + ".bin";
                    std::ofstream Sector_Bin(Nombre_Sector, std::ios::binary);

                    // Llenar el sector con ceros para asegurar el tamaño predefinido
                    std::vector<char> empty_bytes(Capacidad_de_Sectores, 0);
                    Sector_Bin.write(empty_bytes.data(), Capacidad_de_Sectores);
                    Sector_Bin.close(); // Cierra el archivo binario del sector

                    // Registra el estado inicial del sector (0 bytes usados / Capacidad total)
                    IS_Doc << "D" << D << "P" << P << "S" << S << ":0/" << Capacidad_de_Sectores << "\n";
                }
            }
        }
        IS_Doc.close();
        std::cout << "El disco fue creado exitosamente en: " << Ruta << "\n";
        Ruta_Predefinida += "/Disco";
        return true;
    }
    else {
        std::cout << "La carpeta ya existe en: " << Ruta << "\n";
        // Asegura que Ruta_Predefinida apunte al directorio del disco si aún no lo hace
        if (Ruta_Predefinida.find("/Disco") == std::string::npos) {
            Ruta_Predefinida += "/Disco";
        }
        return true;
    }
}

// Borra la estructura completa del disco simulado
void Disco_Manager::Borrar_Disco_Duro() {
    if (fs::exists(Ruta_Predefinida)) {
        fs::remove_all(Ruta_Predefinida); // Elimina el directorio y todo su contenido
        std::cout << "Se eliminó el disco en: " << Ruta_Predefinida << "\n";
        tablas_cargadas.clear(); // Limpia las tablas cargadas en memoria
    }
    else {
        std::cout << "La carpeta no existe en: " << Ruta_Predefinida << "\n";
    }
}

// Crea una nueva tabla a partir de un archivo SQL
void Disco_Manager::CrearTablaDesdeArchivo(const std::string& ruta_archivo_sql) {
    std::ifstream archivo(ruta_archivo_sql);
    if (!archivo.is_open()) {
        std::cerr << "No se pudo abrir el archivo: " << ruta_archivo_sql << "\n";
        return;
    }

    std::string linea, nombre_tabla;
    std::vector<std::tuple<std::string, std::string, int, bool>> columnas;
    bool primary_key_definida = false;

    // 1. Leer y verificar la línea CREATE TABLE
    std::getline(archivo, linea);
    std::smatch match_tabla;
    // Expresión regular para encontrar "CREATE TABLE NombreTabla("
    std::regex regex_tabla(R"(CREATE\s+TABLE\s+(\w+)\s*\()", std::regex::icase);
    if (std::regex_search(linea, match_tabla, regex_tabla)) {
        nombre_tabla = match_tabla[1]; // Captura el nombre de la tabla
    }
    else {
        std::cerr << "Error: la primera línea debe ser 'CREATE TABLE NombreTabla('\n";
        return;
    }

    std::string ruta_base = Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla;
    // Verifica si la tabla ya existe en el sistema de archivos
    if (fs::exists(ruta_base)) {
        std::cerr << "La tabla '" << nombre_tabla << "' ya existe.\n";

        // Si la tabla no está cargada en memoria, intenta cargarla
        if (tablas_cargadas.find(nombre_tabla) == tablas_cargadas.end()) {
            std::cout << "Cargando información de la tabla '" << nombre_tabla << "'...\n";
            Tabla nueva_tabla(Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Data_Tabla.txt");
            if (nueva_tabla.getNombre() != "ERROR_NO_EXISTE") { // Si la carga fue exitosa
                auto result = tablas_cargadas.insert({ nombre_tabla, nueva_tabla });
                if (result.second) { // Si se insertó correctamente
                    result.first->second.MostrarInformacion();
                }
            }
            else {
                std::cerr << "Error al cargar metadata de la tabla existente: " << nombre_tabla << "\n";
            }
        }
        else {
            std::cout << "La tabla '" << nombre_tabla << "' ya está cargada en memoria.\n";
        }
        return;
    }

    // 2. Leer todas las líneas de definición de columnas
    std::vector<std::string> lineas_columnas;
    while (std::getline(archivo, linea)) {
        linea = Trim(linea); // Elimina espacios en blanco al inicio y al final
        if (linea == ");") break; // Fin de la definición de la tabla
        lineas_columnas.push_back(linea);
    }

    // 3. Procesar cada línea de columna
    std::map<std::string, int> tipos_base = {
        {"INTEGER", 4}, {"CHAR", 1}, {"FLOAT", 4}, {"BOOL", 1}
    };

    for (size_t i = 0; i < lineas_columnas.size(); ++i) {
        std::string current_line = lineas_columnas[i];

        // Verifica la sintaxis de las comas y paréntesis
        if (i < lineas_columnas.size() - 1 && current_line.back() != ',') {
            std::cerr << "Error: Revisar comas y/o parentesis en linea " << i + 2 << "\n";
            return;
        }
        if (current_line.back() == ',') current_line.pop_back(); // Elimina la coma final

        std::smatch match_col;
        // Expresión regular para parsear "NombreCol Tipo(Tamaño) [FLAGS]"
        std::regex regex_col(R"((\w+)\s+([A-Z]+)(\((\d+)(,\s*\d+)?\))?)", std::regex::icase);
        if (!std::regex_search(current_line, match_col, regex_col)) {
            std::cerr << "Error de sintaxis en columna: " << current_line << "\n";
            return;
        }

        std::string nombre_col = match_col[1];
        std::string tipo_raw = match_col[2];
        std::string tipo_upper = tipo_raw;
        std::transform(tipo_upper.begin(), tipo_upper.end(), tipo_upper.begin(), ::toupper); // Convierte a mayúsculas

        int bytes = 0;
        if (tipo_upper == "VARCHAR") {
            int tam = std::stoi(match_col[4]); // Captura el tamaño para VARCHAR
            bytes = std::min(tam, 256); // Limita VARCHAR a 256 bytes
        }
        else if (tipo_upper == "DECIMAL") { // Mapea DECIMAL a FLOAT
            tipo_upper = "FLOAT";
            bytes = 4;
        }
        else {
            auto it = tipos_base.find(tipo_upper);
            if (it != tipos_base.end()) {
                bytes = it->second; // Obtiene el tamaño en bytes del tipo base
            }
            else {
                std::cerr << "Tipo no soportado: " << tipo_upper << "\n";
                return;
            }
        }

        // Validar flags de la línea (PRIMARY KEY, NOT NULL)
        std::string line_upper_flags = current_line;
        std::transform(line_upper_flags.begin(), line_upper_flags.end(), line_upper_flags.begin(), ::toupper);

        bool es_pk = false;
        if (line_upper_flags.find("PRIMARY KEY") != std::string::npos) {
            if (primary_key_definida) {
                std::cerr << "Solo puede haber una columna PRIMARY KEY.\n";
                return;
            }
            es_pk = true;
            primary_key_definida = true;
        }

        if (!es_pk && line_upper_flags.find("NOT NULL") == std::string::npos) {
            std::cerr << "La columna '" << nombre_col << "' no tiene NOT NULL.\n";
            return;
        }

        columnas.emplace_back(nombre_col, tipo_upper, bytes, es_pk); // Añade la columna
    }

    // 4. Crear estructura de carpetas para la nueva tabla
    fs::create_directories(ruta_base);

    // 5. Guardar archivo de metadata de la tabla (Data_Tabla.txt)
    std::ofstream data_file(ruta_base + "/Data_Tabla.txt");
    data_file << nombre_tabla << "\n";
    data_file << "C:" << columnas.size() << "\n";
    data_file << "F:0\n"; // Inicialmente 0 filas

    int tamano_total_fila = 0;
    for (auto& [col, tipo, size, pk] : columnas) {
        data_file << col << ", " << tipo << ", " << size << ", " << (pk ? 1 : 0) << "\n";
        tamano_total_fila += size;
    }

    data_file << "T:" << tamano_total_fila << "\n"; // Tamaño total de una fila
    data_file.flush();
    data_file.close();

    // 6. Crear archivo vacío de direcciones de filas (Direccion_De_Filas.txt)
    std::ofstream dir_file(ruta_base + "/Direccion_De_Filas.txt");
    dir_file.close();

    std::cout << "Tabla '" << nombre_tabla << "' creada con éxito.\n";

    // 7. Crear instancia de clase Tabla y registrarla en el mapa de tablas cargadas
    Tabla nueva_tabla(Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Data_Tabla.txt");
    if (nueva_tabla.getNombre() != "ERROR_NO_EXISTE") {
        auto result = tablas_cargadas.insert({ nombre_tabla, nueva_tabla });
        if (result.second) {
            result.first->second.MostrarInformacion();
        }
    }
    else {
        std::cerr << "Error al crear y cargar metadata de la tabla: " << nombre_tabla << "\n";
    }
}

void Disco_Manager::ActualizarCantidadFilas(const std::string& nombre_tabla, int cantidad_a_sumar) {
    std::string ruta_data = Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Data_Tabla.txt";
    std::ifstream data_in(ruta_data);
    if (!data_in.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir Data_Tabla.txt para actualizar cantidad de filas\n";
        return;
    }

    std::stringstream buffer;
    std::string linea;
    while (std::getline(data_in, linea)) {
        if (linea.rfind("F:", 0) == 0) {
            int f_actual = std::stoi(linea.substr(2));
            linea = "F:" + std::to_string(f_actual + cantidad_a_sumar);
        }
        buffer << linea << "\n";
    }
    data_in.close();

    std::ofstream data_out(ruta_data);
    data_out << buffer.str();
    data_out.close();
}


// Inserta filas en una tabla desde un archivo CSV
void Disco_Manager::InsertarFilasDesdeCSV(const std::string& ruta_csv, const std::string& nombre_tabla) {
    // 1. Verificar si la tabla está cargada
    if (tablas_cargadas.find(nombre_tabla) == tablas_cargadas.end()) {
        std::cerr << "[ERROR] La tabla '" << nombre_tabla << "' no está cargada.\n";
        return;
    }

    Tabla& tabla = tablas_cargadas[nombre_tabla];
    const auto& columnas = tabla.getColumnas();

    std::ifstream archivo_csv(ruta_csv);
    if (!archivo_csv.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir el archivo CSV: " << ruta_csv << "\n";
        return;
    }

    std::string linea;
    std::getline(archivo_csv, linea);  // Saltar cabecera

    int fila_num = 0;

    while (std::getline(archivo_csv, linea)) {
        std::stringstream ss(linea);
        std::string valor;
        std::vector<uint8_t> fila_en_bytes;

        std::cout << "Fila: -> ";

        for (size_t i = 0; i < columnas.size(); ++i) {
            std::getline(ss, valor, ',');

            // Eliminar espacios al inicio y fin
            valor = std::regex_replace(valor, std::regex("^\\s+|\\s+$"), "");

            if (valor.empty()) {
                std::cerr << "[ERROR] Campo vacío en columna " << i << "\n";
                break;  // No continuamos esta fila
            }

            const auto& columna = columnas[i];
            std::string tipo = std::regex_replace(std::get<1>(columna), std::regex("^\\s+|\\s+$"), "");
            std::transform(tipo.begin(), tipo.end(), tipo.begin(), ::toupper);
            int size = std::get<2>(columna);

            try {
                if (tipo == "INTEGER") {
                    int numero = std::stoi(valor);
                    uint8_t* ptr = reinterpret_cast<uint8_t*>(&numero);
                    fila_en_bytes.insert(fila_en_bytes.end(), ptr, ptr + size);
                    std::cout << numero << "\t";
                }
                else if (tipo == "FLOAT") {
                    float f = std::stof(valor);
                    uint8_t* ptr = reinterpret_cast<uint8_t*>(&f);
                    fila_en_bytes.insert(fila_en_bytes.end(), ptr, ptr + size);
                    std::cout << f << "\t";
                }
                else if (tipo == "CHAR") {
                    fila_en_bytes.push_back(static_cast<uint8_t>(valor[0]));
                    std::cout << valor[0] << "\t";
                }
                else if (tipo == "VARCHAR") {
                    for (int j = 0; j < size; ++j) {
                        if (j < valor.size())
                            fila_en_bytes.push_back(valor[j]);
                        else
                            fila_en_bytes.push_back(0);  // Padding
                    }
                    std::cout << valor << "\t";
                }
                else {
                    std::cerr << "[ERROR] Tipo no soportado: " << tipo << "\n";
                    break;
                }
            }
            catch (std::exception& e) {
                std::cerr << "[ERROR] Conversión fallida en columna " << i << ": " << valor << "\n";
                break;
            }
        }

        if (!fila_en_bytes.empty()) {
            std::cout << "\n";
            GuardarFilaEnSectores(fila_en_bytes, nombre_tabla);
            fila_num++;
        }
        else {
            std::cerr << "[WARN] Fila no insertada por errores.\n";
        }
    }

    archivo_csv.close();
    ActualizarCantidadFilas(nombre_tabla, fila_num);

    tablas_cargadas[nombre_tabla].cantidad_filas += fila_num;
    tablas_cargadas[nombre_tabla].MostrarInformacion();

    std::cout << "\n Inserción desde CSV completada. Filas insertadas: " << fila_num << "\n";
}

// Guarda una fila en formato binario en los sectores del disco
void Disco_Manager::GuardarFilaEnSectores(const std::vector<uint8_t>& fila_bytes, const std::string& nombre_tabla) {
    std::string ruta_info = Ruta_Predefinida + "/MetaData/Informacion_Sectores.txt";
    std::ifstream info_in(ruta_info);
    if (!info_in.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir Informacion_Sectores.txt\n";
        return;
    }

    // Lee la información actual de uso de todos los sectores
    std::vector<std::tuple<int, int, int, int>> sectores_info; // Plato, Pista, Sector, bytes_usados
    std::string linea;
    std::regex regex_sector(R"(D(\d+)P(\d+)S(\d+):(\d+)/(\d+))");

    while (std::getline(info_in, linea)) {
        std::smatch match;
        if (std::regex_match(linea, match, regex_sector)) {
            if (match.size() == 6) {
                int plato = std::stoi(match[1]);
                int pista = std::stoi(match[2]);
                int sector = std::stoi(match[3]);
                int usado = std::stoi(match[4]);
                sectores_info.emplace_back(plato, pista, sector, usado);
            }
        }
        else if (!linea.empty()) {
            std::cerr << "[WARN] Línea inválida en Informacion_Sectores.txt: " << linea << "\n";
        }
    }
    info_in.close();

    Tabla& tabla = tablas_cargadas[nombre_tabla];
    std::string ruta_dir = Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Direccion_De_Filas.txt";
    std::ofstream dir_out(ruta_dir, std::ios::app); // Abre en modo append para añadir la nueva dirección

    std::string fila_posicion_str; // Cadena para almacenar la ubicación de la fila
    size_t total_bytes_a_escribir = fila_bytes.size();
    size_t bytes_escritos_total = 0;

    // Itera sobre los sectores disponibles para escribir la fila
    for (auto& sector_entry : sectores_info) {
        if (bytes_escritos_total >= total_bytes_a_escribir) break; // Si ya se escribió toda la fila, sale

        int plato = std::get<0>(sector_entry);
        int pista = std::get<1>(sector_entry);
        int sector = std::get<2>(sector_entry);
        int usado_en_sector = std::get<3>(sector_entry);

        int libre_en_sector = Capacidad_de_Sectores - usado_en_sector;
        if (libre_en_sector == 0) continue; // Si el sector está lleno, pasa al siguiente

        size_t bytes_restantes_fila = total_bytes_a_escribir - bytes_escritos_total;
        size_t bytes_a_escribir_en_este_sector = std::min<size_t>(libre_en_sector, bytes_restantes_fila);

        std::string ruta_bin = Ruta_Predefinida + "/Plato_" + std::to_string(plato) +
            "/Pista_" + std::to_string(pista) + "/Sector_" + std::to_string(sector) + ".bin";

        std::fstream sector_bin(ruta_bin, std::ios::in | std::ios::out | std::ios::binary);
        if (!sector_bin.is_open()) {
            std::cerr << "[ERROR] No se pudo abrir el archivo binario del sector: " << ruta_bin << "\n";
            return;
        }

        sector_bin.seekp(usado_en_sector); // Posiciona el puntero de escritura al final de los datos usados
        sector_bin.write(reinterpret_cast<const char*>(&fila_bytes[bytes_escritos_total]), bytes_a_escribir_en_este_sector);
        sector_bin.close();

        // Construye la cadena de posición de la fila
        if (!fila_posicion_str.empty()) fila_posicion_str += ",";
        fila_posicion_str += std::to_string(plato) + "/" + std::to_string(pista) + "/" + std::to_string(sector) + "/" +
            std::to_string(usado_en_sector) + "-" + std::to_string(usado_en_sector + bytes_a_escribir_en_este_sector);

        bytes_escritos_total += bytes_a_escribir_en_este_sector;

        // Actualiza el estado del sector en memoria para la próxima iteración
        std::get<3>(sector_entry) += bytes_a_escribir_en_este_sector;
    }

    if (bytes_escritos_total < total_bytes_a_escribir) {
        std::cerr << "[ERROR] No se pudo escribir la fila completa. Bytes escritos: " << bytes_escritos_total
            << ", total: " << total_bytes_a_escribir << ". Disco posiblemente lleno.\n";
        Disco_Esta_Lleno = true; // Marca el disco como lleno si no se pudo escribir todo
        return;
    }

    // Reescribe Informacion_Sectores.txt con los nuevos usos de los sectores
    std::ofstream info_out_rewrite(ruta_info);
    if (!info_out_rewrite.is_open()) {
        std::cerr << "[ERROR] No se pudo reescribir Informacion_Sectores.txt\n";
        return;
    }
    for (const auto& entry : sectores_info) {
        info_out_rewrite << "D" << std::get<0>(entry) << "P" << std::get<1>(entry) << "S" << std::get<2>(entry)
            << ":" << std::get<3>(entry) << "/" << Capacidad_de_Sectores << "\n";
    }
    info_out_rewrite.close();

    // Guarda la ubicación completa de la fila en Direccion_De_Filas.txt
    dir_out << fila_posicion_str << "\n";
    dir_out.close();

    const auto& columnas = tabla.getColumnas();
    const std::string& nombre_pk = tabla.getNombrePK();

    // Indexa la clave primaria de la fila recién insertada
    int index_pk = -1;
    size_t offset_pk = 0;
    for (size_t i = 0; i < columnas.size(); ++i) {
        if (std::get<0>(columnas[i]) == nombre_pk) {
            index_pk = i;
            break;
        }
        offset_pk += std::get<2>(columnas[i]); // Suma el tamaño de las columnas anteriores para obtener el offset
    }
    if (index_pk == -1) {
        std::cerr << "[WARN] Columna PRIMARY KEY no encontrada para indexación.\n";
        return;
    }

    std::string tipo_pk = std::get<1>(columnas[index_pk]);
    int size_pk = std::get<2>(columnas[index_pk]);

    std::string clave_pk_str; // Valor de la clave primaria como string

    if (offset_pk + size_pk > fila_bytes.size()) {
        std::cerr << "[ERROR] Intento de leer PK fuera de los límites de fila_bytes.\n";
        return;
    }

    // Castea el valor de la clave primaria a string para el índice AVL
    std::string tipo_pk_normalized = Trim(tipo_pk);
    std::transform(tipo_pk_normalized.begin(), tipo_pk_normalized.end(), tipo_pk_normalized.begin(), ::toupper);

    if (tipo_pk_normalized == "INTEGER") {
        int valor;
        std::memcpy(&valor, fila_bytes.data() + offset_pk, sizeof(int));
        clave_pk_str = std::to_string(valor);
    }
    else if (tipo_pk_normalized == "FLOAT") {
        float valor;
        std::memcpy(&valor, fila_bytes.data() + offset_pk, sizeof(float));
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << valor; // Formato para flotantes
        clave_pk_str = ss.str();
    }
    else if (tipo_pk_normalized == "CHAR") {
        char valor_char = static_cast<char>(fila_bytes[offset_pk]);
        clave_pk_str = std::string(1, valor_char);
    }
    else if (tipo_pk_normalized == "VARCHAR") {
        std::string temp_str(reinterpret_cast<const char*>(fila_bytes.data() + offset_pk), size_pk);
        temp_str.erase(std::find(temp_str.begin(), temp_str.end(), '\0'), temp_str.end()); // Eliminar padding
        clave_pk_str = temp_str;
    }
    else if (tipo_pk_normalized == "BOOL") {
        bool valor_bool = static_cast<bool>(fila_bytes[offset_pk]);
        clave_pk_str = valor_bool ? "true" : "false";
    }
    else {
        std::cerr << "[ERROR] Tipo de PK no soportado para indexación: " << tipo_pk << "\n";
        return;
    }

    tabla.getIndicePK().insertar(clave_pk_str, fila_posicion_str); // Inserta en el índice AVL

    // Actualiza el número de filas en el archivo de metadata de la tabla
    std::string ruta_data = Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Data_Tabla.txt";
    std::ifstream data_in(ruta_data);
    std::stringstream data_buf;
    int current_rows = 0;
    bool rows_updated = false;

    while (std::getline(data_in, linea)) {
        if (linea.rfind("F:", 0) == 0) {
            current_rows = std::stoi(linea.substr(2));
            linea = "F:" + std::to_string(current_rows + 1); // Incrementa el contador de filas
            rows_updated = true;
        }
        data_buf << linea << "\n";
    }
    data_in.close();

    std::ofstream data_out(ruta_data);
    data_out << data_buf.str();
    data_out.close();

    std::cout << "[SUCCESS] Fila insertada en: " << fila_posicion_str << "\n";
}

// Lee una fila específica del disco por su número de fila y la retorna casteada
std::vector<std::any> Disco_Manager::LeerFilaPorNumero(const std::string& nombre_tabla, int numero_fila) {
    std::vector<std::any> resultado;

    if (tablas_cargadas.find(nombre_tabla) == tablas_cargadas.end()) {
        std::cerr << "[ERROR] La tabla '" << nombre_tabla << "' no está cargada.\n";
        return resultado;
    }

    Tabla& tabla = tablas_cargadas[nombre_tabla];
    const auto& columnas = tabla.getColumnas();

    std::string ruta_dir = Ruta_Predefinida + "/MetaData/Tablas/" + nombre_tabla + "/Direccion_De_Filas.txt";
    std::ifstream dir_in(ruta_dir);
    if (!dir_in.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir Direccion_De_Filas.txt para la tabla '" << nombre_tabla << "'\n";
        return resultado;
    }

    std::string direccion_linea;
    // Lee hasta la línea correspondiente al numero_fila
    for (int i = 0; i <= numero_fila; ++i) {
        if (!std::getline(dir_in, direccion_linea)) {
            std::cerr << "[ERROR] No se pudo encontrar la fila " << numero_fila << " en Direccion_De_Filas.txt\n";
            dir_in.close();
            return resultado;
        }
    }
    dir_in.close();

    if (direccion_linea.empty()) {
        std::cerr << "[ERROR] No se encontro la direccion de la fila." << std::endl;
        return resultado;
    }

    std::vector<uint8_t> fila_bytes; // Almacena los bytes crudos de la fila
    std::stringstream ss(direccion_linea);
    std::string fragmento;

    // Expresión regular para parsear fragmentos de dirección "plato/pista/sector/inicio-fin"
    std::regex frag_regex(R"((\d+)/(\d+)/(\d+)/(\d+)-(\d+))");

    // Lee y ensambla los fragmentos de la fila desde los sectores del disco
    while (std::getline(ss, fragmento, ',')) {
        std::smatch match_frag;
        if (std::regex_match(fragmento, match_frag, frag_regex)) {
            int plato = std::stoi(match_frag[1]);
            int pista = std::stoi(match_frag[2]);
            int sector = std::stoi(match_frag[3]);
            int desde = std::stoi(match_frag[4]);
            int hasta = std::stoi(match_frag[5]);

            std::string ruta_sector = Ruta_Predefinida + "/Plato_" + std::to_string(plato) +
                "/Pista_" + std::to_string(pista) + "/Sector_" + std::to_string(sector) + ".bin";
            std::ifstream sector_bin(ruta_sector, std::ios::binary);

            if (!sector_bin.is_open()) {
                std::cerr << "[ERROR] No se pudo abrir sector: " << ruta_sector << "\n";
                return resultado;
            }

            int cantidad = hasta - desde;
            std::vector<uint8_t> buffer(cantidad);
            sector_bin.seekg(desde); // Posiciona el puntero de lectura
            sector_bin.read(reinterpret_cast<char*>(buffer.data()), cantidad); // Lee los bytes
            sector_bin.close();

            fila_bytes.insert(fila_bytes.end(), buffer.begin(), buffer.end()); // Añade al vector de bytes de la fila
        }
        else {
            std::cerr << "[ERROR] Formato de fragmento de dirección inválido: " << fragmento << "\n";
            return resultado;
        }
    }

    // Deserializa los bytes de la fila a los tipos de datos originales
    size_t offset = 0;
    for (const auto& [nombre_col, tipo, tamanio, es_pk] : columnas) {
        std::string tipo_normalizado = Trim(tipo);
        std::transform(tipo_normalizado.begin(), tipo_normalizado.end(), tipo_normalizado.begin(), ::toupper);

        if (offset + tamanio > fila_bytes.size()) {
            std::cerr << "[ERROR] Intento de leer fuera de los límites de fila_bytes para columna " << nombre_col << "\n";
            return {}; // Retorna vacío en caso de error
        }

        if (tipo_normalizado == "INTEGER") {
            int val;
            std::memcpy(&val, fila_bytes.data() + offset, sizeof(int));
            resultado.push_back(val);
            offset += sizeof(int);
        }
        else if (tipo_normalizado == "FLOAT") {
            float val;
            std::memcpy(&val, fila_bytes.data() + offset, sizeof(float));
            resultado.push_back(val);
            offset += sizeof(float);
        }
        else if (tipo_normalizado == "CHAR") {
            char val = static_cast<char>(fila_bytes[offset]);
            resultado.push_back(val);
            offset += 1;
        }
        else if (tipo_normalizado == "BOOL") {
            bool val = static_cast<bool>(fila_bytes[offset]);
            resultado.push_back(val);
            offset += 1;
        }
        else if (tipo_normalizado == "VARCHAR") {
            std::string str(reinterpret_cast<const char*>(fila_bytes.data() + offset), tamanio);
            str.erase(std::find(str.begin(), str.end(), '\0'), str.end()); // Elimina el padding de nulos
            resultado.push_back(str);
            offset += tamanio;
        }
        else {
            std::cerr << "[ERROR] Tipo no reconocido: " << tipo_normalizado << "\n";
            return {}; // Retorna vacío en caso de error
        }
    }

    return resultado;
}

// Muestra una fila casteada a sus tipos originales (para depuración en consola)
void Disco_Manager::MostrarFilaCasteada(const Tabla& tabla, const std::vector<std::any>& fila) {
    const auto& columnas = tabla.getColumnas();

    if (fila.size() != columnas.size()) {
        std::cerr << "[ERROR] Tamaño de datos no coincide con la cantidad de columnas\n";
        return;
    }

    for (size_t i = 0; i < columnas.size(); ++i) {
        const auto& [nombre_col, tipo, tamanio, es_pk] = columnas[i];
        std::string tipo_normalizado = Trim(tipo);
        std::transform(tipo_normalizado.begin(), tipo_normalizado.end(), tipo_normalizado.begin(), ::toupper);

        try {
            if (tipo_normalizado == "INTEGER") {
                std::cout << *std::any_cast<int>(&fila[i]);
            }
            else if (tipo_normalizado == "FLOAT") {
                std::cout << *std::any_cast<float>(&fila[i]);
            }
            else if (tipo_normalizado == "CHAR") {
                std::cout << *std::any_cast<char>(&fila[i]);
            }
            else if (tipo_normalizado == "BOOL") {
                std::cout << (*std::any_cast<bool>(&fila[i]) ? "true" : "false");
            }
            else if (tipo_normalizado == "VARCHAR") {
                std::cout << "\"" << *std::any_cast<std::string>(&fila[i]) << "\"";
            }
            else {
                std::cout << "[TIPO DESCONOCIDO]";
            }
        }
        catch (const std::bad_any_cast& e) {
            std::cout << "[ERROR DE CASTEO: " << e.what() << "]";
        }
        std::cout << (i == columnas.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;
}

// Implementación de ExecuteSQLQuery como método de Disco_Manager
std::vector<std::vector<std::string>> Disco_Manager::ExecuteSQLQuery(const std::string& selectStr, const std::string& fromStr, const std::string& whereStr) {
    std::vector<std::vector<std::string>> results;
    // No usamos queryResultMessage directamente aquí, lo manejaremos en la UI

    if (tablas_cargadas.find(fromStr) == tablas_cargadas.end()) {
        // La tabla no se encontró
        return results; // Retorna un vector vacío
    }

    const Tabla& sourceTabla = tablas_cargadas.at(fromStr);
    const auto& sourceColumns = sourceTabla.getColumnas();
    std::vector<int> selectedColumnIndices;
    std::vector<std::string> resultHeaders;

    // Parseo de la cláusula SELECT
    if (selectStr == "*" || selectStr.empty()) {
        for (size_t i = 0; i < sourceColumns.size(); ++i) {
            selectedColumnIndices.push_back(static_cast<int>(i));
            resultHeaders.push_back(std::get<0>(sourceColumns[i]));
        }
    }
    else {
        std::stringstream ss_select(selectStr);
        std::string col_name_token;
        while (std::getline(ss_select, col_name_token, ',')) {
            std::string trimmed_col_name = Trim(col_name_token);
            bool found = false;
            for (size_t i = 0; i < sourceColumns.size(); ++i) {
                if (std::get<0>(sourceColumns[i]) == trimmed_col_name) {
                    selectedColumnIndices.push_back(static_cast<int>(i));
                    resultHeaders.push_back(std::get<0>(sourceColumns[i]));
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Columna no encontrada
                return {}; // Retorna vacío para indicar error
            }
        }
    }

    resultHeaders.push_back("Ubicación"); // Añadir la nueva columna de ubicación
    results.push_back(resultHeaders); // Añadir encabezados como la primera fila

    // Procesamiento de las filas y cláusula WHERE
    std::string ruta_dir_filas = Ruta_Predefinida + "/MetaData/Tablas/" + fromStr + "/Direccion_De_Filas.txt";
    std::ifstream dir_filas_in(ruta_dir_filas);
    if (!dir_filas_in.is_open()) {
        // Error al abrir el archivo de direcciones
        return {};
    }

    std::string current_location_line;
    for (int row_idx = 0; row_idx < sourceTabla.getCantidadFilas(); ++row_idx) {
        std::vector<std::any> rowDataAny = LeerFilaPorNumero(fromStr, row_idx);
        // La variable 'current_location_line' se define y utiliza correctamente aquí.
        // No hay un uso de 'SplitCSVLine(line)' en esta función.
        if (!std::getline(dir_filas_in, current_location_line)) {
            current_location_line = "[Ubicación Desconocida]"; // Fallback si no se puede leer la ubicación
        }

        if (rowDataAny.empty()) {
            continue; // Saltar filas que no se pudieron leer
        }

        bool rowMatches = true;
        if (!whereStr.empty()) {
            std::string filterColName;
            std::string op;
            std::string filterValueStr;

            size_t opPos = std::string::npos;
            if (whereStr.find('=') != std::string::npos) opPos = whereStr.find('=');
            else if (whereStr.find('>') != std::string::npos) opPos = whereStr.find('>');
            else if (whereStr.find('<') != std::string::npos) opPos = whereStr.find('<');

            if (opPos != std::string::npos) {
                filterColName = Trim(whereStr.substr(0, opPos));
                op = whereStr.substr(opPos, 1);
                filterValueStr = Trim(whereStr.substr(opPos + 1));

                if (filterValueStr.length() >= 2 && filterValueStr.front() == '\'' && filterValueStr.back() == '\'') {
                    filterValueStr = filterValueStr.substr(1, filterValueStr.length() - 2);
                }

                int filterColIndex = -1;
                bool isFilterColPrimaryKey = false;
                for (size_t i = 0; i < sourceColumns.size(); ++i) {
                    if (std::get<0>(sourceColumns[i]) == filterColName) {
                        filterColIndex = static_cast<int>(i);
                        isFilterColPrimaryKey = std::get<3>(sourceColumns[i]);
                        break;
                    }
                }

                if (isFilterColPrimaryKey) {
                    // Error: No se permite filtrar directamente por una columna PRIMARY KEY.
                    dir_filas_in.close();
                    return {};
                }

                if (filterColIndex != -1 && filterColIndex < static_cast<int>(rowDataAny.size())) {
                    const std::string actualValueStr = [&]() -> std::string {
                        const auto& col_info = sourceColumns[filterColIndex];
                        std::string col_type = std::get<1>(col_info);
                        std::transform(col_type.begin(), col_type.end(), col_type.begin(), ::toupper);
                        col_type.erase(col_type.begin(), std::find_if(col_type.begin(), col_type.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                        col_type.erase(std::find_if(col_type.rbegin(), col_type.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), col_type.end());

                        try {
                            if (col_type == "INTEGER") return std::to_string(std::any_cast<int>(rowDataAny[filterColIndex]));
                            if (col_type == "FLOAT") {
                                std::stringstream ss_float;
                                ss_float << std::fixed << std::setprecision(2) << std::any_cast<float>(rowDataAny[filterColIndex]);
                                return ss_float.str();
                            }
                            if (col_type == "CHAR") return std::string(1, std::any_cast<char>(rowDataAny[filterColIndex]));
                            if (col_type == "BOOL") return std::any_cast<bool>(rowDataAny[filterColIndex]) ? "true" : "false";
                            if (col_type == "VARCHAR") return std::any_cast<std::string>(rowDataAny[filterColIndex]);
                        }
                        catch (const std::bad_any_cast& e) {
                            std::cerr << "Error de casteo en columna de filtro: " << e.what() << std::endl;
                        }
                        return ""; // Fallback
                        }();

                    if (op == "=") {
                        if (actualValueStr != filterValueStr) {
                            rowMatches = false;
                        }
                    }
                    else if (op == ">") {
                        try {
                            float actualVal = std::stof(actualValueStr);
                            float filterVal = std::stof(filterValueStr);
                            if (!(actualVal > filterVal)) {
                                rowMatches = false;
                            }
                        }
                        catch (const std::exception& e) {
                            // Error: Valor no numerico en comparacion de tipo '>'.
                            dir_filas_in.close();
                            return {};
                        }
                    }
                    else if (op == "<") {
                        try {
                            float actualVal = std::stof(actualValueStr);
                            float filterVal = std::stof(filterValueStr);
                            if (!(actualVal < filterVal)) {
                                rowMatches = false;
                            }
                        }
                        catch (const std::exception& e) {
                            // Error: Valor no numerico en comparacion de tipo '<'.
                            dir_filas_in.close();
                            return {};
                        }
                    }
                }
                else {
                    rowMatches = false; // Columna de filtro no encontrada o índice fuera de rango
                }
            }
            else {
                // Operador no reconocido
                rowMatches = false;
            }
        }

        if (rowMatches) {
            std::vector<std::string> resultRow;
            for (int colIdx : selectedColumnIndices) {
                if (colIdx < static_cast<int>(rowDataAny.size())) {
                    const auto& col_info = sourceColumns[colIdx];
                    std::string col_type = std::get<1>(col_info);
                    std::transform(col_type.begin(), col_type.end(), col_type.begin(), ::toupper);
                    col_type.erase(col_type.begin(), std::find_if(col_type.begin(), col_type.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                    col_type.erase(std::find_if(col_type.rbegin(), col_type.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), col_type.end());

                    try {
                        if (col_type == "INTEGER") resultRow.push_back(std::to_string(std::any_cast<int>(rowDataAny[colIdx])));
                        else if (col_type == "FLOAT") {
                            std::stringstream ss_float;
                            ss_float << std::fixed << std::setprecision(2) << std::any_cast<float>(rowDataAny[colIdx]);
                            resultRow.push_back(ss_float.str());
                        }
                        else if (col_type == "CHAR") resultRow.push_back(std::string(1, std::any_cast<char>(rowDataAny[colIdx])));
                        else if (col_type == "BOOL") resultRow.push_back(std::any_cast<bool>(rowDataAny[colIdx]) ? "true" : "false");
                        else if (col_type == "VARCHAR") resultRow.push_back(std::any_cast<std::string>(rowDataAny[colIdx]));
                        else resultRow.push_back("[TIPO DESCONOCIDO]");
                    }
                    catch (const std::bad_any_cast& e) {
                        resultRow.push_back("[ERROR CASTEO]");
                        std::cerr << "Error de casteo al mostrar resultados: " << e.what() << std::endl;
                    }
                }
                else {
                    resultRow.push_back("N/A"); // Si el índice de columna no es válido
                }
            }
            resultRow.push_back(current_location_line); // Añadir la ubicación de la fila
            results.push_back(resultRow);
        }
    }
    dir_filas_in.close();
    return results;
}


// --- Funciones auxiliares (implementación) ---

// Elimina espacios en blanco al inicio y al final de una cadena.
std::string Trim(const std::string& str) {
    const char* whitespace = " \t\n\r";
    size_t first = str.find_first_not_of(whitespace);
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(whitespace);
    return str.substr(first, (last - first + 1));
}

// Divide una línea CSV, manejando comillas y comas escapadas.
std::vector<std::string> SplitCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string currentCellContent;
    bool inQuote = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '"') {
            if (inQuote) {
                if (i + 1 < line.length() && line[i + 1] == '"') {
                    currentCellContent += '"';
                    i++; // Consume la segunda comilla (para comillas escapadas "")
                }
                else {
                    inQuote = false; // Cierra la comilla
                }
            }
            else {
                inQuote = true; // Abre la comilla
            }
        }
        else if (c == ',' && !inQuote) {
            result.push_back(Trim(currentCellContent)); // Añade la celda y la limpia
            currentCellContent = ""; // Reinicia para la siguiente celda
        }
        else {
            currentCellContent += c; // Añade el carácter al contenido de la celda
        }
    }
    result.push_back(Trim(currentCellContent)); // Añade la última celda
    return result;
}
