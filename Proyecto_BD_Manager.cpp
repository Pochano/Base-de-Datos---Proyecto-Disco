#define _CRT_SECURE_NO_WARNINGS
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstring>
#include <cstdio>
#include <algorithm>

#include "Disco_Manager.h"
#include "Tablas.h"
#include "Data_Manager.h"

// estado global
static Disco_Manager* g_Disco = nullptr;
static Tabla* g_Tabla = nullptr;
static Manager_Data   g_DM;

enum class Pagina { Configuracion, CargarDatos, VistaTabla };
static Pagina pagina = Pagina::Configuracion;

// Configuración
static char buf_ruta[256] = "Disco_Inventory";
static int  inp_platos = 2, inp_pistas = 10, inp_sectores = 50, inp_cap = 128;
static std::string msg_config;

// Cargar datos
static char buf_sql[512] = "";
static char buf_csv[512] = "";
static bool tiene_encabezado = false;
static std::string msg_sql, msg_csv;
static bool tabla_lista = false, csv_listo = false;

// Búsquedas
static char buf_col_ig[64] = "", buf_val_ig[128] = "";
static char buf_col_rg[64] = "", buf_min[64] = "-", buf_max[64] = "-";
static std::vector<std::vector<std::string>> res_ig, res_rg;
static std::string msg_ig, msg_rg;

// Cache de filas
static std::vector<std::vector<std::string>> filas_cache;
static bool cache_lista = false;

// AVL
static char buf_col_avl[64] = "";
static std::set<std::string> avl_destacados;
static bool mostrar_avl = false;

struct NodoDibujo { std::string clave; float x, y; Nodo* ptr; };

static void BuildLayout(Nodo* n, int depth, float& xc,
    std::vector<NodoDibujo>& out,
    std::unordered_map<Nodo*, size_t>& idx_map)
{
    if (!n) return;
    BuildLayout(n->Izquierda, depth + 1, xc, out, idx_map);
    idx_map[n] = out.size();
    out.push_back({ n->Clave, xc * 58.f, depth * 68.f, n });
    xc++;
    BuildLayout(n->Derecha, depth + 1, xc, out, idx_map);
}

static void DrawEdgesAVL(Nodo* n,
    std::unordered_map<Nodo*, size_t>& idx_map,
    std::vector<NodoDibujo>& nodes,
    ImDrawList* dl, ImVec2 origin)
{
    if (!n) return;
    ImVec2 p = { origin.x + nodes[idx_map[n]].x, origin.y + nodes[idx_map[n]].y };
    auto edge = [&](Nodo* child) {
        ImVec2 c = { origin.x + nodes[idx_map[child]].x, origin.y + nodes[idx_map[child]].y };
        dl->AddLine(p, c, IM_COL32(120, 120, 120, 255), 1.5f);
        };
    if (n->Izquierda) { edge(n->Izquierda); DrawEdgesAVL(n->Izquierda, idx_map, nodes, dl, origin); }
    if (n->Derecha) { edge(n->Derecha);   DrawEdgesAVL(n->Derecha, idx_map, nodes, dl, origin); }
}


static bool EsPot2(int n) { return n > 0 && (n & (n - 1)) == 0; }

static void ColorMensaje(const std::string& s) {
    bool err = s.find("Error") != std::string::npos || s.find("error") != std::string::npos;
    ImGui::PushStyleColor(ImGuiCol_Text, err ? ImVec4(1.f, .35f, .35f, 1.f) : ImVec4(.35f, 1.f, .35f, 1.f));
    ImGui::TextWrapped("%s", s.c_str());
    ImGui::PopStyleColor();
}

static void RenderTablaImGui(const char* id,
    const std::vector<std::string>& cols,
    const std::vector<std::vector<std::string>>& filas,
    float alto = 250.f)
{
    if (cols.empty() || filas.empty()) return;

    std::vector<float> colWidths(cols.size(), 0.f);
    for (int j = 0; j < (int)cols.size(); j++) {
        float w = ImGui::CalcTextSize(cols[j].c_str()).x + ImGui::GetStyle().CellPadding.x * 2.f + 8.f;
        if (w > colWidths[j]) colWidths[j] = w;
    }
    for (auto& fila : filas) {
        for (int j = 0; j < (int)fila.size() && j < (int)cols.size(); j++) {
            float w = ImGui::CalcTextSize(fila[j].c_str()).x + ImGui::GetStyle().CellPadding.x * 2.f + 8.f;
            if (w > colWidths[j]) colWidths[j] = w;
        }
    }

    if (ImGui::BeginTable(id, (int)cols.size(),
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX,
        ImVec2(0, alto)))
    {
        for (int j = 0; j < (int)cols.size(); j++)
            ImGui::TableSetupColumn(cols[j].c_str(), ImGuiTableColumnFlags_WidthFixed, colWidths[j]);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (auto& fila : filas) {
            ImGui::TableNextRow();
            for (int j = 0; j < (int)fila.size() && j < (int)cols.size(); j++) {
                ImGui::TableSetColumnIndex(j);
                ImGui::TextUnformatted(fila[j].c_str());
            }
        }
        ImGui::EndTable();
    }
}

static std::vector<std::string> NombresColumnas() {
    std::vector<std::string> r;
    if (g_Tabla) for (auto& c : g_Tabla->get_Columnas()) r.push_back(c.Nombre);
    return r;
}

static std::string FormatearUbicacion(int fila_id) {
    auto frags = g_Disco->Calcular_Fragmentos_Fila(fila_id, g_Tabla->get_Size_Fila());
    std::string r;
    for (int i = 0; i < (int)frags.size(); i++) {
        if (i > 0) r += ", ";
        r += std::to_string(frags[i].Plato) + "/" + std::to_string(frags[i].Superficie) + "/" + std::to_string(frags[i].Pista) + "/"  + std::to_string(frags[i].Sector) + "/" + std::to_string(frags[i].Byte_Inicio) + "-" + std::to_string(frags[i].Byte_Fin);
    }
    return r;
}

// configuración
static void PagConfiguracion() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({ io.DisplaySize.x * .5f, io.DisplaySize.y * .5f }, ImGuiCond_Always, { .5f,.5f });
    ImGui::SetNextWindowSize({ 520, 0 }, ImGuiCond_Always);
    ImGui::Begin("Configuracion del Disco", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Parametros");
    ImGui::Spacing();
    ImGui::Text("Ruta del disco:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ruta", buf_ruta, sizeof(buf_ruta));
    ImGui::Spacing();

    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Platos:");                 ImGui::NextColumn(); ImGui::SetNextItemWidth(-1); ImGui::InputInt("##pl", &inp_platos);    ImGui::NextColumn();
    ImGui::Text("Pistas por plato:");       ImGui::NextColumn(); ImGui::SetNextItemWidth(-1); ImGui::InputInt("##pi", &inp_pistas);    ImGui::NextColumn();
    ImGui::Text("Sectores por pista:");     ImGui::NextColumn(); ImGui::SetNextItemWidth(-1); ImGui::InputInt("##se", &inp_sectores);  ImGui::NextColumn();
    ImGui::Text("Bytes por sector (2^n):"); ImGui::NextColumn(); ImGui::SetNextItemWidth(-1); ImGui::InputInt("##ca", &inp_cap);       ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Spacing();
    if (inp_platos > 0 && inp_pistas > 0 && inp_sectores > 0 && EsPot2(inp_cap)) {
        long long tot = (long long)inp_platos * 2 * inp_pistas * inp_sectores * inp_cap;
        ImGui::TextDisabled("Capacidad total: %lld bytes  (%.2f KB / %.4f MB)", tot, tot / 1024.0, tot / 1048576.0);
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (!msg_config.empty()) { ColorMensaje(msg_config); ImGui::Spacing(); }

    bool valido = inp_platos > 0 && inp_pistas > 0 && inp_sectores > 0
        && EsPot2(inp_cap) && strlen(buf_ruta) > 0;
    if (!valido) ImGui::BeginDisabled();
    if (ImGui::Button("Crear Disco", { -1, 0 })) {
        delete g_Disco; g_Disco = nullptr;
        delete g_Tabla; g_Tabla = nullptr;
        tabla_lista = csv_listo = cache_lista = false;
        msg_sql = msg_csv = "";
        std::string ruta(buf_ruta);
        g_Disco = new Disco_Manager(ruta, inp_platos, inp_pistas, inp_sectores, inp_cap);
        if (g_Disco->Crear_Disco()) {
            msg_config = "Disco creado correctamente.";
            pagina = Pagina::CargarDatos;
        }
        else {
            msg_config = "Error al crear el disco. Verifica ruta y permisos.";
            delete g_Disco; g_Disco = nullptr;
        }
    }
    if (!valido) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("  * La capacidad del sector debe ser potencia de 2 (ej: 64, 128, 256)");
    }
    ImGui::End();
}

// cargar datos
static void PagCargarDatos() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({ io.DisplaySize.x * .5f, io.DisplaySize.y * .5f }, ImGuiCond_Always, { .5f,.5f });
    ImGui::SetNextWindowSize({ 620, 0 }, ImGuiCond_Always);
    ImGui::Begin("Cargar Datos", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Paso 1 - Esquema de tabla (.txt con CREATE TABLE)");
    ImGui::SetNextItemWidth(-140.f);
    ImGui::InputText("##sql", buf_sql, sizeof(buf_sql));
    ImGui::SameLine();
    if (ImGui::Button("Cargar SQL", { 130, 0 })) {
        delete g_Tabla; g_Tabla = nullptr;
        tabla_lista = csv_listo = cache_lista = false;
        Tabla t = g_DM.Crear_Tabla_SQL(std::string(buf_sql));
        if (t.get_Nombre_Tabla() != "Error") {
            g_Tabla = new Tabla(t);
            tabla_lista = true;
            msg_sql = "OK - Tabla: " + g_Tabla->get_Nombre_Tabla()
                + "  |  " + std::to_string(g_Tabla->get_Columnas().size()) + " columnas";
        }
        else {
            msg_sql = "Error: no se pudo leer el archivo SQL.";
        }
    }
    if (!msg_sql.empty()) { ImGui::Spacing(); ColorMensaje(msg_sql); }
    ImGui::Spacing();

    bool bloqueado = !tabla_lista;
    if (bloqueado) ImGui::BeginDisabled();
    ImGui::SeparatorText("Paso 2 - Datos de la tabla (.csv)");
    ImGui::Checkbox("El CSV tiene fila de encabezado", &tiene_encabezado);
    ImGui::SetNextItemWidth(-140.f);
    ImGui::InputText("##csv", buf_csv, sizeof(buf_csv));
    ImGui::SameLine();
    if (ImGui::Button("Cargar CSV", { 130, 0 })) {
        cache_lista = false; csv_listo = false;
        if (g_DM.Cargar_CSV(std::string(buf_csv), *g_Tabla, *g_Disco, tiene_encabezado)) {
            csv_listo = true; cache_lista = false;
            msg_csv = "OK - " + std::to_string(g_Tabla->get_Cantidad_Filas()) + " filas cargadas.";
        }
        else {
            msg_csv = "Error: no se pudo cargar el CSV.";
        }
    }
    if (!msg_csv.empty()) { ImGui::Spacing(); ColorMensaje(msg_csv); }
    if (bloqueado) ImGui::EndDisabled();

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    if (ImGui::Button("< Volver", { 120, 0 })) pagina = Pagina::Configuracion;
    ImGui::SameLine();
    if (!csv_listo) ImGui::BeginDisabled();
    if (ImGui::Button("Ver Tabla >", { -1, 0 })) pagina = Pagina::VistaTabla;
    if (!csv_listo) ImGui::EndDisabled();
    ImGui::End();
}

// vista tabla
static void PagVistaTabla() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::Begin("Vista", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // barra superior
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(.1f, .12f, .2f, 1.f));
    ImGui::BeginChild("topbar", { -1.f, 46.f }, false);
    ImGui::SetCursorPos({ 10.f, 13.f });
    ImGui::Text("  Tabla: %s   |   %d filas   |   Disco: %d platos · %d pistas · %d sectores · %d B/sector   |   Cap: %.1f KB",
        g_Tabla->get_Nombre_Tabla().c_str(), g_Tabla->get_Cantidad_Filas(),
        g_Disco->get_Platos(), g_Disco->get_Pistas(),
        g_Disco->get_Sectores(), g_Disco->get_Capacidad_Sectores(),
        g_Disco->get_Total_Capacidad() / 1024.f);
    ImGui::SameLine(io.DisplaySize.x - 280.f);
    ImGui::SetCursorPosY(10.f);
    if (ImGui::Button("Arbol AVL", { 110.f, 28.f })) mostrar_avl = !mostrar_avl;
    ImGui::SameLine();
    if (ImGui::Button("< Cargar datos", { 130.f, 28.f })) pagina = Pagina::CargarDatos;
    ImGui::EndChild();
    ImGui::PopStyleColor();

    float h = io.DisplaySize.y - 62.f;

    // panel izquierdo esquema
    ImGui::BeginChild("esquema", { 220.f, h }, true);
    ImGui::SeparatorText("Esquema");
    ImGui::Text("Tabla:    %s", g_Tabla->get_Nombre_Tabla().c_str());
    ImGui::Text("Filas:    %d", g_Tabla->get_Cantidad_Filas());
    ImGui::Text("Tam fila: %d bytes", g_Tabla->get_Size_Fila());
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    auto cols = g_Tabla->get_Columnas();
    ImGui::Text("Columnas (%d):", (int)cols.size());
    ImGui::Spacing();
    for (auto& c : cols)
        ImGui::BulletText("%s\n  %s · %d bytes", c.Nombre.c_str(), Tipo_a_String(c.Tipo).c_str(), c.Size);
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::SeparatorText("Disco");
    ImGui::Text("Platos:      %d", g_Disco->get_Platos());
    ImGui::Text("Pistas:      %d", g_Disco->get_Pistas());
    ImGui::Text("Sectores:    %d", g_Disco->get_Sectores());
    ImGui::Text("Cap/sector:  %d B", g_Disco->get_Capacidad_Sectores());
    ImGui::Text("Total sect:  %d", g_Disco->get_Total_Sectores());
    ImGui::Text("Capacidad:   %.1f KB", g_Disco->get_Total_Capacidad() / 1024.f);
    ImGui::EndChild();

    ImGui::SameLine();

    // panel derecho pestañas
    ImGui::BeginChild("content", { -1.f, h }, false);
    if (ImGui::BeginTabBar("tabs")) {

        // tab datos
        if (ImGui::BeginTabItem("Datos")) {
            if (!cache_lista) {
                filas_cache.clear();
                for (int i = 0; i < g_Tabla->get_Cantidad_Filas(); i++) {
                    auto fila = g_Tabla->Leer_Fila(i, *g_Disco);
                    fila.push_back(FormatearUbicacion(i));
                    filas_cache.push_back(fila);
                }
                cache_lista = true;
            }
            ImGui::TextDisabled("Mostrando %d filas", (int)filas_cache.size());
            ImGui::Spacing();
            auto headers = NombresColumnas();
            headers.push_back("Ubicacion");
            RenderTablaImGui("t_datos", headers, filas_cache, h - 55.f);
            ImGui::EndTabItem();
        }

        //tab búsqueda igualdad
        if (ImGui::BeginTabItem("Busqueda Igualdad")) {
            ImGui::Spacing();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Columna:"); ImGui::SameLine(90.f); ImGui::SetNextItemWidth(220.f);
            ImGui::InputText("##col_ig", buf_col_ig, sizeof(buf_col_ig));
            ImGui::Text("Valor:");   ImGui::SameLine(90.f); ImGui::SetNextItemWidth(220.f);
            ImGui::InputText("##val_ig", buf_val_ig, sizeof(buf_val_ig));
            ImGui::Spacing();
            if (ImGui::Button("Buscar##ig", { 120.f, 0 })) {
                auto ids = g_Tabla->Buscar_Igualdad_IDs(buf_col_ig, buf_val_ig);
                res_ig.clear();
                avl_destacados.clear();
                strcpy(buf_col_avl, buf_col_ig);
                auto tcols = g_Tabla->get_Columnas();
                int col_idx = -1;
                for (int i = 0; i < (int)tcols.size(); i++)
                    if (tcols[i].Nombre == buf_col_ig) { col_idx = i; break; }
                for (int id : ids) {
                    auto fila = g_Tabla->Leer_Fila(id, *g_Disco);
                    if (col_idx >= 0) avl_destacados.insert(fila[col_idx]);
                    fila.push_back(FormatearUbicacion(id));
                    res_ig.push_back(fila);
                }
                msg_ig = res_ig.empty() ? "Sin resultados." : std::to_string(res_ig.size()) + " resultado(s).";
            }
            if (!msg_ig.empty()) { ImGui::SameLine(); ImGui::TextDisabled("%s", msg_ig.c_str()); }
            ImGui::Spacing();
            if (!res_ig.empty()) {
                auto headers_ig = NombresColumnas();
                headers_ig.push_back("Ubicacion");
                RenderTablaImGui("t_ig", headers_ig, res_ig, h - 150.f);
            }
            ImGui::EndTabItem();
        }

        //tab búsqueda rango
        if (ImGui::BeginTabItem("Busqueda Rango")) {
            ImGui::Spacing();
            ImGui::TextDisabled("Usa \"-\" para dejar un extremo abierto.");
            ImGui::Spacing();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Columna:"); ImGui::SameLine(90.f); ImGui::SetNextItemWidth(220.f);
            ImGui::InputText("##col_rg", buf_col_rg, sizeof(buf_col_rg));
            ImGui::Text("Minimo:");  ImGui::SameLine(90.f); ImGui::SetNextItemWidth(220.f);
            ImGui::InputText("##min", buf_min, sizeof(buf_min));
            ImGui::Text("Maximo:");  ImGui::SameLine(90.f); ImGui::SetNextItemWidth(220.f);
            ImGui::InputText("##max", buf_max, sizeof(buf_max));
            ImGui::Spacing();
            if (ImGui::Button("Buscar##rg", { 120.f, 0 })) {
                auto ids = g_Tabla->Buscar_Rango_IDs(buf_col_rg, buf_min, buf_max);
                res_rg.clear();
                avl_destacados.clear();
                strcpy(buf_col_avl, buf_col_rg);
                auto tcols = g_Tabla->get_Columnas();
                int col_idx = -1;
                for (int i = 0; i < (int)tcols.size(); i++)
                    if (tcols[i].Nombre == buf_col_rg) { col_idx = i; break; }
                for (int id : ids) {
                    auto fila = g_Tabla->Leer_Fila(id, *g_Disco);
                    if (col_idx >= 0) avl_destacados.insert(fila[col_idx]);
                    fila.push_back(FormatearUbicacion(id));
                    res_rg.push_back(fila);
                }
                msg_rg = res_rg.empty() ? "Sin resultados." : std::to_string(res_rg.size()) + " resultado(s).";
            }
            if (!msg_rg.empty()) { ImGui::SameLine(); ImGui::TextDisabled("%s", msg_rg.c_str()); }
            ImGui::Spacing();
            if (!res_rg.empty()) {
                auto headers_rg = NombresColumnas();
                headers_rg.push_back("Ubicacion");
                RenderTablaImGui("t_rg", headers_rg, res_rg, h - 175.f);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    //ventana flotante árbol avl
    if (mostrar_avl) {
        ImGui::SetNextWindowSize({ 600.f, 500.f }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({ 300.f, 100.f }, ImGuiCond_FirstUseEver);
        ImGui::Begin("Arbol AVL", &mostrar_avl, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Text("Columna:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputText("##col_avl", buf_col_avl, sizeof(buf_col_avl));
        ImGui::TextDisabled("Verde = nodos resultado de la ultima busqueda");
        ImGui::Separator();

        if (g_Tabla && strlen(buf_col_avl) > 0) {
            AVL_Index* avl = g_Tabla->get_Indice(buf_col_avl);
            if (avl && avl->get_Raiz()) {
                std::vector<NodoDibujo> nodes;
                std::unordered_map<Nodo*, size_t> idx_map;
                float xc = 0.f;
                BuildLayout(avl->get_Raiz(), 0, xc, nodes, idx_map);

                float max_x = 0, max_y = 0;
                for (auto& n : nodes) {
                    if (n.x > max_x) max_x = n.x;
                    if (n.y > max_y) max_y = n.y;
                }

                ImGui::BeginChild("avl_canvas", { -1.f, -1.f }, false,
                    ImGuiWindowFlags_HorizontalScrollbar);
                ImVec2 origin = {
                    ImGui::GetCursorScreenPos().x + 35.f,
                    ImGui::GetCursorScreenPos().y + 35.f
                };
                ImGui::Dummy({ max_x + 80.f, max_y + 60.f });
                ImDrawList* dl = ImGui::GetWindowDrawList();

                const float R = 18.f;
                DrawEdgesAVL(avl->get_Raiz(), idx_map, nodes, dl, origin);
                for (auto& nd : nodes) {
                    ImVec2 c = { origin.x + nd.x, origin.y + nd.y };
                    bool hit = avl_destacados.count(nd.ptr->Clave) > 0;
                    dl->AddCircleFilled(c, R, hit ? IM_COL32(40, 160, 80, 255) : IM_COL32(55, 85, 140, 255));
                    dl->AddCircle(c, R, hit ? IM_COL32(100, 255, 130, 255) : IM_COL32(110, 150, 220, 255), 0, 2.f);
                    std::string label = nd.clave.size() > 9 ? nd.clave.substr(0, 8) + "~" : nd.clave;
                    ImVec2 ts = ImGui::CalcTextSize(label.c_str());
                    dl->AddText({ c.x - ts.x * .5f, c.y - ts.y * .5f },
                        IM_COL32(255, 255, 255, 255), label.c_str());
                }
                ImGui::EndChild();
            }
            else {
                ImGui::TextDisabled("Sin datos en ese indice.");
            }
        }
        else {
            ImGui::TextDisabled("Ingresa el nombre de una columna.");
        }
        ImGui::End();
    }

    ImGui::End(); 
}

int main(int, char**) {
    glfwSetErrorCallback([](int e, const char* d) { fprintf(stderr, "GLFW Error %d: %s\n", e, d); });
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720,
        "Simulador de Base de Datos en Disco", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        switch (pagina) {
        case Pagina::Configuracion: PagConfiguracion(); break;
        case Pagina::CargarDatos:   PagCargarDatos();   break;
        case Pagina::VistaTabla:    PagVistaTabla();    break;
        }

        ImGui::Render();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(.1f, .1f, .13f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    delete g_Disco;
    delete g_Tabla;
    return 0;
}