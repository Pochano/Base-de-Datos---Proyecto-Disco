#define _CRT_SECURE_NO_WARNINGS
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>   
#include <algorithm>
#include <iomanip>   
#include <cstdint>   
#include <filesystem> 
#include <tuple>     
#include <any>       
#include <regex>     

#include "AVL_Tree.h"
#include "Tabla.h"
#include "Disco_Manager.h"

namespace fs = std::filesystem;
extern std::string Trim(const std::string& str);
extern std::vector<std::string> SplitCSVLine(const std::string& line);

Disco_Manager* global_disco_manager = nullptr;


enum class Page {
    Configuration,
    InsertTableSQL,
    InsertTableCSV,
    QueryTables,
    ShowTables
};

Page currentPage = Page::Configuration;

// Página 1: Configuración del Disco
int diskPlates = 0;
int tracksPerPlate = 0;
int sectorsPerTrack = 0;
int sectorSize = 0;
char diskLocationBuffer[256] = "";

// Página 2: Insertar Tabla (SQL)
std::string sqlFilePath = "";
char sqlFilePathBuffer[256] = "";
std::string sqlParseMessage = "";

// Página 3: Insertar Tabla (CSV)
std::string csvFilePath = "";
char csvFilePathBuffer[256] = "";
std::string csvLoadMessage = "";
std::string selectedTableNameForCSV = "";
int selectedTableForCSVIndex = -1; // Para el ImGui::Combo

// Página 4 y 5: Tablas y Consultas
std::vector<std::vector<std::string>> queryResults;
std::string queryResultMessage = "";

// Entradas de la página de consulta (estas son las variables que mencionaste)
std::string selectInput = "";
char selectInputBuffer[128] = "";
std::string fromInput = "";
char fromInputBuffer[128] = "";
std::string whereInput = "";
char whereInputBuffer[256] = "";

// Página de Mostrar Tablas
std::string selectedTableToShow = "";

// Función para leer el contenido de un archivo.
std::string ReadFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Función auxiliar para manejar el contenido SQL simulado.
void HandleSimulatedSQLFile(const std::string& contentToParse) {
    std::string tempSqlFilePath = "temp_simulated_sql_script.txt";
    std::ofstream tempFile(tempSqlFilePath);
    if (tempFile.is_open()) {
        tempFile << contentToParse;
        tempFile.close();
        if (global_disco_manager) {
            global_disco_manager->CrearTablaDesdeArchivo(tempSqlFilePath);
            sqlParseMessage = "SQL simulado procesado. Tablas creadas/actualizadas en el disco.";
        }
        else {
            sqlParseMessage = "Error: Disco no inicializado.";
        }
    }
    else {
        sqlParseMessage = "Error: No se pudo crear el archivo temporal SQL.";
    }
}

// Función auxiliar para manejar el contenido CSV simulado.
void HandleSimulatedCSVFile(const std::string& contentToParse, const std::string& tableName) {
    std::string tempCsvFilePath = "temp_simulated_data.csv";
    std::ofstream tempFile(tempCsvFilePath);
    if (tempFile.is_open()) {
        tempFile << contentToParse;
        tempFile.close();
        if (global_disco_manager) {
            global_disco_manager->InsertarFilasDesdeCSV(tempCsvFilePath, tableName);
            csvLoadMessage = "CSV simulado cargado exitosamente en " + tableName + "!";
        }
        else {
            csvLoadMessage = "Error: Disco no inicializado.";
        }
    }
    else {
        csvLoadMessage = "Error: No se pudo crear el archivo temporal CSV.";
    }
}

// --- Funciones de Renderizado de Páginas ---

void RenderPageConfiguration() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Pag 1: Configuracion Disco", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Cantidad de platos");
    ImGui::InputInt("##CantidadPlatos", &diskPlates);

    ImGui::Text("Cantidad de pistas por plato");
    ImGui::InputInt("##CantidadPistas", &tracksPerPlate);

    ImGui::Text("Cantidad de sectores por pista");
    ImGui::InputInt("##CantidadSectores", &sectorsPerTrack);

    ImGui::Text("Tamano de cada sector (solo valores 2^n)");
    ImGui::InputInt("##TamanoSector", &sectorSize);

    ImGui::Text("Ubicacion donde se creara el disco (ej: C:\\simulated_disk\\)");
    ImGui::InputText("##DiskLocation", diskLocationBuffer, sizeof(diskLocationBuffer));

    if (ImGui::Button("Guardar")) {
        if (diskPlates <= 0 || tracksPerPlate <= 0 || sectorsPerTrack <= 0 || sectorSize <= 0) {
            std::cerr << "Error: Todos los campos de configuración del disco deben ser mayores que cero.\n";
        }
        else if ((sectorSize & (sectorSize - 1)) != 0) {
            std::cerr << "Error: El tamaño del sector debe ser una potencia de 2.\n";
        }
        else if (strlen(diskLocationBuffer) == 0) {
            std::cerr << "Error: La ubicación del disco no puede estar vacía.\n";
        }
        else {
            if (global_disco_manager) {
                delete global_disco_manager;
            }
            global_disco_manager = new Disco_Manager(diskPlates, tracksPerPlate, sectorsPerTrack, sectorSize);
            global_disco_manager->Asignar_Ruta(diskLocationBuffer);
            if (global_disco_manager->Creacion_de_Disco_en_Memoria()) {
                currentPage = Page::InsertTableSQL;
            }
            else {
                std::cerr << "Fallo la creacion del disco en memoria. Verifique los permisos o la ruta." << std::endl;
            }
        }
    }

    ImGui::End();
}

void RenderPageInsertTableSQL() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Pag 2: Insertar tabla (SQL)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Ruta del archivo SQL para crear tablas:");

    ImGui::InputText("##SQLFilePath", sqlFilePathBuffer, sizeof(sqlFilePathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Seleccionar archivo .txt (simulado)")) {
        sqlFilePath = "simulated_sql_script.txt";
        strcpy_s(sqlFilePathBuffer, sizeof(sqlFilePathBuffer), sqlFilePath.c_str());
        sqlParseMessage = "Archivo SQL simulado seleccionado. Presiona 'LEER TXT' para procesarlo.";
    }

    ImGui::Separator();

    if (ImGui::Button("LEER TXT")) {
        if (!global_disco_manager) {
            sqlParseMessage = "Error: El disco no ha sido inicializado. Vuelve a la página de Configuración.";
        }
        else {
            std::string contentToParse;
            if (strlen(sqlFilePathBuffer) > 0) {
                if (strcmp(sqlFilePathBuffer, "simulated_sql_script.txt") == 0) {
                    contentToParse = "CREATE TABLE PRODUCTO(\n"
                        "Index INTEGER PRIMARY KEY,\n"
                        "Item VARCHAR(40) NOT NULL,\n"
                        "Cost FLOAT NOT NULL,\n"
                        "Tax FLOAT NOT NULL,\n"
                        "Total FLOAT NOT NULL\n"
                        ");\n"
                        "CREATE TABLE PERSONA(\n"
                        "DNI INTEGER PRIMARY KEY,\n"
                        "Nombre VARCHAR(50) NOT NULL,\n"
                        "Apellido VARCHAR(50) NOT NULL,\n"
                        "Edad INTEGER\n"
                        ");";
                    HandleSimulatedSQLFile(contentToParse);
                }
                else {
                    contentToParse = ReadFileContent(sqlFilePathBuffer);
                    if (contentToParse.empty()) {
                        sqlParseMessage = "Error: No se pudo leer el archivo en la ruta especificada. Verifica la ruta y permisos.";
                    }
                    else {
                        global_disco_manager->CrearTablaDesdeArchivo(sqlFilePathBuffer);
                        sqlParseMessage = "Archivo SQL procesado. Tablas creadas/actualizadas en el disco.";
                    }
                }
            }
            else {
                sqlParseMessage = "Por favor, ingresa una ruta de archivo o selecciona uno.";
            }
        }

        if (sqlParseMessage.find("Error") == std::string::npos) {
            currentPage = Page::InsertTableCSV;
        }
    }

    if (!sqlParseMessage.empty()) {
        ImGui::Text(sqlParseMessage.c_str());
    }

    ImGui::End();
}

void RenderPageInsertTableCSV() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Pag 3: Insertar datos de la tabla (CSV)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Seleccionar tabla de destino:");
    std::vector<const char*> tableNamesCStr;
    std::vector<std::string> tempTableNames;

    if (global_disco_manager) {
        for (const auto& pair : global_disco_manager->tablas_cargadas) {
            tempTableNames.push_back(pair.first);
        }
    }
    for (const std::string& name : tempTableNames) {
        tableNamesCStr.push_back(name.c_str());
    }

    if (ImGui::Combo("##TableSelectForCSV", &selectedTableForCSVIndex, tableNamesCStr.data(), static_cast<int>(tableNamesCStr.size()))) {
        if (selectedTableForCSVIndex >= 0 && selectedTableForCSVIndex < tableNamesCStr.size()) {
            selectedTableNameForCSV = tableNamesCStr[selectedTableForCSVIndex];
            csvLoadMessage = "Tabla '" + selectedTableNameForCSV + "' seleccionada para cargar CSV.";
        }
    }

    ImGui::Text("Ruta del archivo CSV para insertar datos:");
    ImGui::InputText("##CSVFilePath", csvFilePathBuffer, sizeof(csvFilePathBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Seleccionar archivo .CSV (simulado)")) {
        csvFilePath = "simulated_data.csv";
        strcpy_s(csvFilePathBuffer, sizeof(csvFilePathBuffer), csvFilePath.c_str());
        csvLoadMessage = "Archivo CSV simulado seleccionado. Presiona 'LEER .CSV' para cargarlo.";
    }

    ImGui::Separator();

    if (ImGui::Button("LEER .CSV")) {
        if (!global_disco_manager) {
            csvLoadMessage = "Error: El disco no ha sido inicializado. Vuelve a la página de Configuración.";
        }
        else if (selectedTableNameForCSV.empty()) {
            csvLoadMessage = "Error: Por favor, selecciona una tabla de destino primero.";
        }
        else {
            std::string csvContent;
            if (strlen(csvFilePathBuffer) > 0) {
                if (strcmp(csvFilePathBuffer, "simulated_data.csv") == 0) {
                    if (selectedTableNameForCSV == "PRODUCTO") {
                        csvContent = "\"Index\", \"Item\", \"Cost\", \"Tax\", \"Total\"\n"
                            "1, \"Fruit of the Loom Girl's Socks\", 7.97, 0.60, 8.57\n"
                            "2, \"Rawlings Little League Baseball\", 2.97, 0.22, 3.19\n"
                            "3, \"Secret Antiperspirant\", 1.29, 0.10, 1.39\n"
                            "4, \"Deadpool DVD\", 14.96, 1.12, 16.08\n"
                            "5, \"Maxwell House Coffee 28 oz\", 7.28, 0.55, 7.83\n"
                            "6, \"Banana Boat Sunscreen, 8 oz\", 6.68, 0.50, 7.18\n"
                            "7, \"Wrench Set, 10 pieces\", 10.00, 0.75, 10.75\n"
                            "8, \"M and M, 42 oz\", 8.98, 0.67, 9.65\n"
                            "9, \"Bertoli Alfredo Sauce\", 2.12, 0.16, 2.28\n"
                            "10, \"Large Paperclips, 10 boxes\", 6.19, 0.46, 6.65";
                        HandleSimulatedCSVFile(csvContent, selectedTableNameForCSV);
                    }
                    else if (selectedTableNameForCSV == "PERSONA") {
                        csvContent = "\"DNI\", \"Nombre\", \"Apellido\", \"Edad\"\n"
                            "71590181, \"Jose\", \"Cuadros\", 19\n"
                            "12345678, \"Maria\", \"Lopez\", 25\n"
                            "98765432, \"Carlos\", \"Perez\", 30";
                        HandleSimulatedCSVFile(csvContent, selectedTableNameForCSV);
                    }
                    else {
                        csvLoadMessage = "Error: No hay datos CSV simulados para la tabla seleccionada.";
                        csvContent = "";
                    }
                }
                else {
                    csvContent = ReadFileContent(csvFilePathBuffer);
                    if (csvContent.empty()) {
                        csvLoadMessage = "Error: No se pudo leer el archivo en la ruta especificada. Verifica la ruta y permisos.";
                    }
                    else {
                        global_disco_manager->InsertarFilasDesdeCSV(csvFilePathBuffer, selectedTableNameForCSV);
                        csvLoadMessage = "Datos CSV cargados exitosamente en " + selectedTableNameForCSV + "!";
                    }
                }
            }
            else {
                csvLoadMessage = "Por favor, ingresa una ruta de archivo o selecciona uno.";
            }
        }
    }

    if (!csvLoadMessage.empty()) {
        ImGui::Text(csvLoadMessage.c_str());
    }

    ImGui::Separator();
    if (ImGui::Button("CONTINUAR")) {
        currentPage = Page::QueryTables;
    }

    ImGui::End();
}

void RenderPageQueryTables() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Once);
    ImGui::Begin("Pag 4: Tablas y Consultas", nullptr);

    ImGui::BeginChild("TablesPane", ImVec2(250, 0), true);
    ImGui::Text("Tablas");
    ImGui::Separator();
    if (!global_disco_manager || global_disco_manager->tablas_cargadas.empty()) {
        ImGui::Text("No hay tablas cargadas.");
    }
    else {
        for (const auto& pair : global_disco_manager->tablas_cargadas) {
            ImGui::Text("%s:", pair.first.c_str());
            for (const auto& col : pair.second.getColumnas()) {
                ImGui::BulletText("%s (%s%s)", std::get<0>(col).c_str(), std::get<1>(col).c_str(), std::get<3>(col) ? ", PK" : "");
            }
            ImGui::Spacing();
        }
    }
    if (ImGui::Button("MOSTRAR TABLAS")) {
        currentPage = Page::ShowTables;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Realizar consultas");

    ImGui::AlignTextToFramePadding();
    ImGui::Text("SELECT"); ImGui::SameLine(); ImGui::SetNextItemWidth(150);
    ImGui::InputText("##SelectInput", selectInputBuffer, sizeof(selectInputBuffer));
    ImGui::SameLine(); ImGui::Text("FROM"); ImGui::SameLine(); ImGui::SetNextItemWidth(150);
    ImGui::InputText("##FromInput", fromInputBuffer, sizeof(fromInputBuffer));
    ImGui::SameLine(); ImGui::Text("WHERE"); ImGui::SameLine(); ImGui::SetNextItemWidth(150);
    ImGui::InputText("##WhereInput", whereInputBuffer, sizeof(whereInputBuffer));

    if (ImGui::Button("REALIZAR CONSULTA")) {
        selectInput = selectInputBuffer;
        fromInput = fromInputBuffer;
        whereInput = whereInputBuffer;

        queryResults.clear();
        queryResultMessage = "";

        std::cout << "[DEBUG] RenderPageQueryTables: Botón REALIZAR CONSULTA presionado." << std::endl;
        std::cout << "[DEBUG] RenderPageQueryTables: Select: '" << selectInput << "', From: '" << fromInput << "', Where: '" << whereInput << "'" << std::endl;


        if (fromInput.empty()) {
            queryResultMessage = "Error: La clausula FROM no puede estar vacía.";
        }
        else if (!global_disco_manager || global_disco_manager->tablas_cargadas.find(fromInput) == global_disco_manager->tablas_cargadas.end()) {
            queryResultMessage = "Error: La tabla '" + fromInput + "' no existe.";
        }
        else {
            queryResults = global_disco_manager->ExecuteSQLQuery(selectInput, fromInput, whereInput);
            std::cout << "[DEBUG] RenderPageQueryTables: ExecuteSQLQuery retornó " << queryResults.size() << " filas." << std::endl;

            if (queryResults.empty()) {
                queryResultMessage = "No se encontraron resultados o la consulta fue inválida.";
            }
            else {
                queryResultMessage = "Consulta ejecutada. Resultados mostrados abajo.";
            }
        }
    }

    if (!queryResultMessage.empty()) {
        ImGui::Text(queryResultMessage.c_str());
    }

    ImGui::Text("Mostrar resultados de consulta:");
    ImGui::Separator();

    if (!queryResults.empty()) {
        // Calcular el ancho máximo de cada columna según su contenido (encabezado + datos)
        std::vector<float> colWidths(queryResults[0].size(), 0.0f);
        for (const auto& row : queryResults) {
            for (size_t j = 0; j < row.size() && j < colWidths.size(); ++j) {
                float w = ImGui::CalcTextSize(row[j].c_str()).x + ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;
                if (w > colWidths[j]) colWidths[j] = w;
            }
        }

        if (ImGui::BeginTable("QueryResultsTable", static_cast<int>(queryResults[0].size()),
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX, ImVec2(0, 200))) {

            for (size_t i = 0; i < queryResults[0].size(); ++i) {
                ImGui::TableSetupColumn(queryResults[0][i].c_str(), ImGuiTableColumnFlags_WidthFixed, colWidths[i]);
            }
            ImGui::TableSetupScrollFreeze(0, 1); // Fijar encabezados al hacer scroll vertical
            ImGui::TableHeadersRow();

            for (size_t i = 1; i < queryResults.size(); ++i) { // Empezar desde 1 para saltar los encabezados
                ImGui::TableNextRow();
                for (size_t j = 0; j < queryResults[i].size(); ++j) {
                    ImGui::TableSetColumnIndex(static_cast<int>(j));
                    ImGui::Text("%s", queryResults[i][j].c_str());
                }
            }
            ImGui::EndTable();
        }
    }
    else {
        ImGui::Text("No hay resultados para mostrar.");
    }

    ImGui::EndGroup();
    ImGui::End();
}

void RenderPageShowTables() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Once);
    ImGui::Begin("Pag 5: Mostrar Tablas", nullptr);

    ImGui::BeginChild("TableSelectionPane", ImVec2(150, 0), true);
    ImGui::Text("Seleccionar Tabla");
    ImGui::Separator();
    if (!global_disco_manager || global_disco_manager->tablas_cargadas.empty()) {
        ImGui::Text("No hay tablas.");
    }
    else {
        for (const auto& pair : global_disco_manager->tablas_cargadas) {
            if (ImGui::Selectable(pair.first.c_str(), selectedTableToShow == pair.first)) {
                selectedTableToShow = pair.first;
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginGroup();
    if (!selectedTableToShow.empty() && global_disco_manager->tablas_cargadas.count(selectedTableToShow)) {
        const Tabla& table = global_disco_manager->tablas_cargadas.at(selectedTableToShow);
        ImGui::Text("Datos de: %s (Filas: %d)", table.getNombre().c_str(), table.getCantidadFilas());
        ImGui::Separator();

        // Preparar datos para ImGui::BeginTable
        std::vector<std::vector<std::string>> displayData;
        std::vector<std::string> headerRow;
        for (const auto& col : table.getColumnas()) {
            headerRow.push_back(std::get<0>(col));
        }
        headerRow.push_back("Ubicación"); // Añadir la columna de ubicación
        if (!headerRow.empty()) {
            displayData.push_back(headerRow);
        }

        std::string ruta_dir_filas = global_disco_manager->Ruta_Predefinida + "/MetaData/Tablas/" + selectedTableToShow + "/Direccion_De_Filas.txt";
        std::ifstream dir_filas_in(ruta_dir_filas);
        if (!dir_filas_in.is_open()) {
            ImGui::Text("Error: No se pudo abrir el archivo de direcciones de filas.");
            std::cerr << "[ERROR] RenderPageShowTables: No se pudo abrir Direccion_De_Filas.txt para " << selectedTableToShow << std::endl;
        }
        else {
            std::string current_location_line;
            for (int row_idx = 0; row_idx < table.getCantidadFilas(); ++row_idx) {
                std::vector<std::any> rowDataAny = global_disco_manager->LeerFilaPorNumero(table.getNombre(), row_idx);
                if (!std::getline(dir_filas_in, current_location_line)) {
                    current_location_line = "[Ubicación Desconocida]";
                }

                if (!rowDataAny.empty()) {
                    std::vector<std::string> currentRow;
                    for (size_t i = 0; i < rowDataAny.size(); ++i) {
                        const auto& col_info = table.getColumnas()[i];
                        std::string col_type = std::get<1>(col_info);
                        std::transform(col_type.begin(), col_type.end(), col_type.begin(), ::toupper);
                        col_type.erase(col_type.begin(), std::find_if(col_type.begin(), col_type.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                        col_type.erase(std::find_if(col_type.rbegin(), col_type.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), col_type.end());

                        try {
                            if (col_type == "INTEGER") currentRow.push_back(std::to_string(std::any_cast<int>(rowDataAny[i])));
                            else if (col_type == "FLOAT") {
                                std::stringstream ss_float;
                                ss_float << std::fixed << std::setprecision(2) << std::any_cast<float>(rowDataAny[i]);
                                currentRow.push_back(ss_float.str());
                            }
                            else if (col_type == "CHAR") currentRow.push_back(std::string(1, std::any_cast<char>(rowDataAny[i])));
                            else if (col_type == "BOOL") currentRow.push_back(std::any_cast<bool>(rowDataAny[i]) ? "true" : "false");
                            else if (col_type == "VARCHAR") currentRow.push_back(std::any_cast<std::string>(rowDataAny[i]));
                            else currentRow.push_back("[TIPO DESCONOCIDO]");
                        }
                        catch (const std::bad_any_cast& e) {
                            currentRow.push_back("[ERROR CASTEO]");
                            std::cerr << "Error de casteo al mostrar tabla: " << e.what() << std::endl;
                        }
                    }
                    currentRow.push_back(current_location_line); // Añadir la ubicación de la fila
                    displayData.push_back(currentRow);
                }
            }
            dir_filas_in.close();
        }

        if (!displayData.empty() && displayData[0].size() > 0) {
            // Calcular el ancho máximo de cada columna según su contenido (encabezado + datos)
            std::vector<float> colWidths(displayData[0].size(), 0.0f);
            for (const auto& row : displayData) {
                for (size_t j = 0; j < row.size() && j < colWidths.size(); ++j) {
                    float w = ImGui::CalcTextSize(row[j].c_str()).x + ImGui::GetStyle().CellPadding.x * 2.0f + 8.0f;
                    if (w > colWidths[j]) colWidths[j] = w;
                }
            }

            if (ImGui::BeginTable("DataTable", static_cast<int>(displayData[0].size()),
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX, ImVec2(0, 300))) {

                for (size_t i = 0; i < displayData[0].size(); ++i) {
                    ImGui::TableSetupColumn(displayData[0][i].c_str(), ImGuiTableColumnFlags_WidthFixed, colWidths[i]);
                }
                ImGui::TableSetupScrollFreeze(0, 1); // Fijar encabezados al hacer scroll vertical
                ImGui::TableHeadersRow();

                for (size_t i = 1; i < displayData.size(); ++i) {
                    ImGui::TableNextRow();
                    for (size_t j = 0; j < displayData[i].size(); ++j) {
                        ImGui::TableSetColumnIndex(static_cast<int>(j));
                        ImGui::Text("%s", displayData[i][j].c_str());
                    }
                }
                ImGui::EndTable();
            }
        }
        else {
            ImGui::Text("La tabla no contiene datos o columnas.");
            std::cout << "[DEBUG] RenderPageShowTables: displayData está vacía o no tiene columnas." << std::endl;
        }
    }
    else {
        ImGui::Text("Por favor, seleccione una tabla para mostrar sus datos.");
    }

    ImGui::Spacing();
    if (ImGui::Button("Volver a Consultas")) {
        currentPage = Page::QueryTables;
    }

    ImGui::EndGroup();
    ImGui::End();
}


// --- Función principal ---

int main(int, char**) {
    // Configuración de errores de GLFW
    glfwSetErrorCallback([](int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
        });
    if (!glfwInit())
        return 1;

    // Configuración de la versión de OpenGL y GLSL
#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Creación de la ventana GLFW
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Simulador de Base de Datos - ImGui", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Fallo al crear la ventana de GLFW" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Habilitar VSync

    // Inicialización de GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Fallo al inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Configuración del contexto de ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Habilitar Controles de Teclado
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Habilitar Controles de Gamepad
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Habilitar Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Habilitar Multi-Viewport / Ventanas de Plataforma

    // Configuración del estilo de ImGui
    ImGui::StyleColorsDark();

    // Cuando los viewports están habilitados, ajustamos WindowRounding/WindowBg para que las ventanas de plataforma se vean idénticas a las normales.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Configuración de backends de Plataforma/Renderizador
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Bucle principal de la aplicación
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Iniciar el frame de Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Renderizar la página seleccionada
        switch (currentPage) {
        case Page::Configuration:
            RenderPageConfiguration();
            break;
        case Page::InsertTableSQL:
            RenderPageInsertTableSQL();
            break;
        case Page::InsertTableCSV:
            RenderPageInsertTableCSV();
            break;
        case Page::QueryTables:
            RenderPageQueryTables();
            break;
        case Page::ShowTables:
            RenderPageShowTables();
            break;
        }

        // Renderizado
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Actualizar y renderizar ventanas de plataforma adicionales
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Limpieza
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // Liberar el Disco_Manager global (si se inicializa en otro lugar)
    if (global_disco_manager) {
        delete global_disco_manager;
        global_disco_manager = nullptr;
    }

    return 0;
}