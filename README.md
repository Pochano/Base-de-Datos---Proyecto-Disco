# Simulador de Disco - Base de Datos II

Proyecto en C++ que simula el almacenamiento físico de una tabla dentro de un disco.  
El sistema crea una estructura de disco usando platos, superficies, pistas y sectores. Luego permite cargar la estructura de una tabla desde un archivo SQL, insertar registros desde un archivo CSV y realizar búsquedas rápidas usando índices AVL.

---

## Objetivo del proyecto

El objetivo principal es simular cómo una base de datos puede almacenar registros en un medio físico, considerando:

- Configuración del disco.
- Tamaño fijo de filas.
- Escritura secuencial de registros.
- Fragmentación de filas entre sectores.
- Cálculo de direcciones físicas.
- Lectura de filas desde sectores.
- Indexación por columnas usando árboles AVL.
- Búsqueda por igualdad.
- Búsqueda por rango.

---

## Archivos del proyecto

| Archivo | Descripción |
|---|---|
| `Proyecto_BD_Manager.cpp` | Archivo principal del programa. Crea el disco, carga la tabla, inserta los datos del CSV y ejecuta consultas de prueba. |
| `Disco_Manager.h` | Administra el disco simulado. Crea platos, superficies, pistas y sectores. También calcula dónde se guarda cada fila. |
| `Tablas.h` | Representa una tabla. Maneja columnas, filas, serialización, escritura en disco, lectura desde disco e índices AVL por columna. |
| `Serializador.h` | Convierte valores de texto a bytes y bytes a texto. Soporta `INTEGER`, `FLOAT`, `BOOL`, `CHAR` y `VARCHAR`. |
| `AVL_Indices.h` | Implementa índices AVL para realizar búsquedas rápidas por igualdad y por rango. |
| `Data_Manager.h` | Lee el archivo SQL para crear la tabla y lee el archivo CSV para insertar registros. |
| `Inventory.txt` | Archivo SQL con la estructura de la tabla `INVENTORY`. |
| `Inventory_v4.csv` | Archivo CSV con los registros que se insertan en la tabla. |
| `.gitignore` | Evita subir archivos generados por Visual Studio, ejecutables y carpetas creadas por el disco simulado. |

---

## Funcionamiento general

El programa trabaja con el siguiente flujo:

```txt
Inventory.txt
    ↓
Data_Manager
    ↓
Crear tabla INVENTORY

Inventory_v4.csv
    ↓
Data_Manager
    ↓
Insertar filas en Tabla
    ↓
Serializador
    ↓
Disco_Manager
    ↓
Guardar bytes en sectores físicos

Consultas
    ↓
AVL_Index
    ↓
Obtener Fila_ID
    ↓
Disco_Manager
    ↓
Leer sectores
    ↓
Reconstruir fila
```

---

## Estructura del disco simulado

El disco se representa mediante carpetas y archivos.

Ejemplo:

```txt
Disco_Inventory/
├── Plato_0/
│   ├── Superficie_A/
│   │   ├── Pista_0/
│   │   │   ├── Sector_0.txt
│   │   │   ├── Sector_1.txt
│   │   │   └── ...
│   │   ├── Pista_1/
│   │   └── ...
│   └── Superficie_B/
│       ├── Pista_0/
│       ├── Pista_1/
│       └── ...
├── Plato_1/
│   ├── Superficie_A/
│   └── Superficie_B/
└── ...
```

Cada sector se guarda como un archivo `.txt`, pero se trabaja en modo binario para poder almacenar correctamente enteros y flotantes en bytes reales.

---

## Configuración del disco

En el archivo principal se crea el disco de esta forma:

```cpp
std::string Ruta_Disco = "Disco_Inventory";

Disco_Manager Disco(Ruta_Disco, 2, 10, 50, 128);
Disco.Crear_Disco();
```

Esto significa:

```txt
Ruta del disco: Disco_Inventory
Platos: 2
Superficies por plato: 2
Pistas por superficie: 10
Sectores por pista: 50
Capacidad por sector: 128 bytes
```

---

## Fórmulas importantes

### 1. Cantidad total de sectores

Cada plato tiene 2 superficies.

```txt
Total_Sectores = Platos * 2 * Pistas * Sectores_por_Pista
```

Ejemplo:

```txt
Platos = 2
Superficies = 2
Pistas = 10
Sectores_por_Pista = 50

Total_Sectores = 2 * 2 * 10 * 50
Total_Sectores = 2000 sectores
```

---

### 2. Capacidad total del disco

```txt
Capacidad_Total = Total_Sectores * Capacidad_Sector
```

Ejemplo:

```txt
Total_Sectores = 2000
Capacidad_Sector = 128 bytes

Capacidad_Total = 2000 * 128
Capacidad_Total = 256000 bytes
```

---

### 3. Tamaño fijo de una fila

Cada fila tiene tamaño fijo.  
El tamaño se calcula sumando el tamaño de todas sus columnas.

Ejemplo de la tabla `INVENTORY`:

```txt
productID                  VARCHAR(15)  = 15 bytes
location                   VARCHAR(7)   = 7 bytes
inventoryType              VARCHAR(10)  = 10 bytes
quantity                   INTEGER      = 4 bytes
quantityUnits              VARCHAR(5)   = 5 bytes
value                      INTEGER      = 4 bytes
valueCurrency              VARCHAR(5)   = 5 bytes
reservationOrders          INTEGER      = 4 bytes
daysOfSupply               INTEGER      = 4 bytes
shelfLife                  INTEGER      = 4 bytes
reorderLevel               INTEGER      = 4 bytes
expectedLeadTime           INTEGER      = 4 bytes
quantityUpperThreshold     INTEGER      = 4 bytes
quantityLowerThreshold     INTEGER      = 4 bytes
daysOfSupplyUpperThreshold INTEGER      = 4 bytes
daysOfSupplyLowerThreshold INTEGER      = 4 bytes
plannerCode                VARCHAR(40)  = 40 bytes
velocityCode               VARCHAR(5)   = 5 bytes
inventoryParentType        VARCHAR(3)   = 3 bytes
class                      VARCHAR(5)   = 5 bytes
```

Entonces:

```txt
Size_Fila = 139 bytes
```

---

### 4. Offset global de una fila

Cada fila se guarda de forma secuencial.  
Para saber dónde empieza una fila se usa:

```txt
Offset_Global = Fila_ID * Size_Fila
```

Ejemplo:

```txt
Size_Fila = 139 bytes

Fila_ID = 0  → Offset_Global = 0 * 139 = 0
Fila_ID = 1  → Offset_Global = 1 * 139 = 139
Fila_ID = 2  → Offset_Global = 2 * 139 = 278
Fila_ID = 3  → Offset_Global = 3 * 139 = 417
```

---

### 5. Sector global y byte inicial

Con el offset global se calcula el sector donde empieza la fila:

```txt
Sector_Global = Offset_Global / Capacidad_Sector
Byte_Inicio   = Offset_Global % Capacidad_Sector
```

Ejemplo:

```txt
Offset_Global = 139
Capacidad_Sector = 128

Sector_Global = 139 / 128 = 1
Byte_Inicio = 139 % 128 = 11
```

Esto significa que la fila empieza en el sector global 1, desde el byte 11.

---

### 6. Conversión de sector global a dirección física

Primero se calculan estos valores:

```txt
Sectores_por_Superficie = Pistas * Sectores_por_Pista
Sectores_por_Plato      = Sectores_por_Superficie * 2
```

Luego se convierte el sector global a dirección física:

```txt
Plato = Sector_Global / Sectores_por_Plato

Resto_Plato = Sector_Global % Sectores_por_Plato

Superficie = Resto_Plato / Sectores_por_Superficie

Resto_Superficie = Resto_Plato % Sectores_por_Superficie

Pista = Resto_Superficie / Sectores_por_Pista

Sector = Resto_Superficie % Sectores_por_Pista
```

La dirección física final queda formada por:

```txt
Plato
Superficie
Pista
Sector
Byte_Inicio
Byte_Fin
```

---

### 7. Fragmentación de filas

Si una fila no entra completa en un sector, se divide en fragmentos.

Se calcula:

```txt
Espacio_Disponible = Capacidad_Sector - Byte_Inicio

Bytes_Tomados = min(Bytes_Restantes, Espacio_Disponible)
```

Luego se escribe ese fragmento y se continúa en el siguiente sector hasta guardar toda la fila.

Ejemplo:

```txt
Capacidad_Sector = 128 bytes
Size_Fila = 139 bytes

Una fila puede ocupar:
- parte de un sector
- el siguiente sector
- e incluso más sectores si fuera más grande
```

---

## Tipos de datos soportados

| Tipo | Tamaño | Descripción |
|---|---:|---|
| `INTEGER` | 4 bytes | Entero guardado como bytes reales. |
| `FLOAT` | 4 bytes | Decimal guardado como bytes reales. |
| `BOOL` | 1 byte | Valor booleano. |
| `CHAR` | 1 byte | Un solo carácter. |
| `VARCHAR(N)` | N bytes | Texto de tamaño fijo. |

---

## Serialización

La serialización convierte los valores leídos como texto desde el CSV a bytes.

Ejemplo:

```csv
PS-SL-A287,LT-1,PRODUCT,25,EA,31250,USD
```

Se convierte en una fila de bytes según el esquema de la tabla.

Ejemplo:

```txt
productID     → VARCHAR(15)
location      → VARCHAR(7)
inventoryType → VARCHAR(10)
quantity      → INTEGER, 4 bytes
value         → INTEGER, 4 bytes
```

Los `INTEGER` no se guardan como texto.  
Se guardan como 4 bytes reales.

Los `VARCHAR(N)` se guardan con tamaño fijo:

```txt
Si el texto es menor que N, se rellena con "_".
Si el texto es mayor que N, se recorta.
```

Ejemplo:

```txt
VARCHAR(10)

"Pepe"      → "Pepe______"
"UniversidadCatolica" → "Universid"
```

---

## Archivo SQL

El archivo `Inventory.txt` contiene la definición de la tabla.

Ejemplo:

```sql
CREATE TABLE INVENTORY(
productID VARCHAR(15),		
location VARCHAR(7),
inventoryType VARCHAR(10),
quantity INTEGER NOT NULL,	
quantityUnits VARCHAR(5),
value INTEGER,
valueCurrency VARCHAR(5),	
reservationOrders INTEGER,	
daysOfSupply INTEGER,	
shelfLife INTEGER,	
reorderLevel INTEGER,	
expectedLeadTime INTEGER,	
quantityUpperThreshold INTEGER,	
quantityLowerThreshold INTEGER,	
daysOfSupplyUpperThreshold INTEGER,	
daysOfSupplyLowerThreshold INTEGER,		
plannerCode VARCHAR(40),	
velocityCode VARCHAR(5),	
inventoryParentType VARCHAR(3),	
class VARCHAR(5)
);
```

El programa lee este archivo y crea automáticamente una tabla con sus columnas y tipos de datos.

---

## Archivo CSV

El archivo `Inventory_v4.csv` contiene los registros que se insertan en la tabla.

Cada línea representa una fila.

Ejemplo:

```csv
PS-SL-A287,LT-1,PRODUCT,25,EA,31250,USD,2,19,365,50,30,100,10,30,10,Street Lighting,B,ONHAND,NEW
```

El CSV debe respetar el mismo orden de columnas definido en el archivo SQL.

En este proyecto, el CSV no tiene encabezado.  
Por eso se carga con:

```cpp
Data_Manager.Cargar_CSV("Inventory_v4.csv", Inventory, Disco, false);
```

Si el CSV tuviera encabezado, se usaría:

```cpp
Data_Manager.Cargar_CSV("Inventory_v4.csv", Inventory, Disco, true);
```

---

## Uso del programa

### 1. Crear el disco

En `Proyecto_BD_Manager.cpp`:

```cpp
std::string Ruta_Disco = "Disco_Inventory";

Disco_Manager Disco(Ruta_Disco, 2, 10, 50, 128);
Disco.Crear_Disco();
```

---

### 2. Crear el manejador de datos

```cpp
Manager_Data Data_Manager;
```

---

### 3. Crear la tabla desde SQL

```cpp
Tabla Inventory = Data_Manager.Crear_Tabla_SQL("Inventory.txt");
```

---

### 4. Cargar los datos desde CSV

```cpp
Data_Manager.Cargar_CSV("Inventory_v4.csv", Inventory, Disco, false);
```

---

### 5. Mostrar el esquema

```cpp
Inventory.Mostrar_Esquema();
```

Salida esperada:

```txt
Tabla: INVENTORY
Size_Fila: 139 | Cantidad_Filas: 154
```

---

### 6. Mostrar algunas filas

```cpp
for (int i = 0; i < 5 && i < Inventory.get_Cantidad_Filas(); i++) {
    Inventory.Mostrar_Fila(i, Disco);
}
```

---

## Índices AVL

Cada columna tiene su propio índice AVL.

Ejemplo:

```txt
Columna productID      → AVL de productID
Columna location       → AVL de location
Columna quantity       → AVL de quantity
Columna value          → AVL de value
Columna velocityCode   → AVL de velocityCode
```

Cada nodo del AVL almacena:

```txt
Clave
Lista de Fila_ID donde aparece esa clave
Altura
Hijo izquierdo
Hijo derecho
```

Ejemplo:

```txt
Clave: LT-1
Filas_ID: [0, 1, 2, 3, 4, ...]
```

Esto permite que una búsqueda no tenga que recorrer todo el disco.

---

## Búsquedas disponibles

El programa permite dos tipos principales de búsqueda:

```txt
1. Búsqueda por igualdad.
2. Búsqueda por rango.
```

---

## Búsqueda por igualdad

Busca las filas donde una columna tenga un valor exacto.

Ejemplo:

```cpp
Inventory.Buscar_Igualdad("location", "LT-1", Disco);
```

Esto busca:

```txt
location = LT-1
```

Otro ejemplo:

```cpp
Inventory.Buscar_Igualdad("velocityCode", "A", Disco);
```

Esto busca:

```txt
velocityCode = A
```

Funcionamiento:

```txt
1. Se busca la clave en el AVL de esa columna.
2. El AVL devuelve los Fila_ID encontrados.
3. Con cada Fila_ID se calcula dónde está la fila en disco.
4. Se leen los sectores necesarios.
5. Se reconstruye la fila.
6. Se muestra el resultado.
```

---

## Búsqueda por rango

Busca las filas donde una columna esté dentro de un rango.

Ejemplo:

```cpp
Inventory.Buscar_Rango("quantity", "20", "30", Disco);
```

Esto busca:

```txt
20 <= quantity <= 30
```

También se pueden usar rangos abiertos usando `-`.

Ejemplo:

```cpp
Inventory.Buscar_Rango("value", "50000", "-", Disco);
```

Esto significa:

```txt
value >= 50000
```

Ejemplo:

```cpp
Inventory.Buscar_Rango("quantity", "-", "30", Disco);
```

Esto significa:

```txt
quantity <= 30
```

Ejemplo:

```cpp
Inventory.Buscar_Rango("quantity", "-", "-", Disco);
```

Esto devolvería todos los valores de esa columna.

---

## Ejemplo de consultas usadas

```cpp
std::cout << "\n===== BUSQUEDA IGUALDAD: location = LT-1 =====\n";
Inventory.Buscar_Igualdad("location", "LT-1", Disco);

std::cout << "\n===== BUSQUEDA IGUALDAD: velocityCode = A =====\n";
Inventory.Buscar_Igualdad("velocityCode", "A", Disco);

std::cout << "\n===== BUSQUEDA RANGO: quantity entre 20 y 30 =====\n";
Inventory.Buscar_Rango("quantity", "20", "30", Disco);

std::cout << "\n===== BUSQUEDA RANGO: value >= 50000 =====\n";
Inventory.Buscar_Rango("value", "50000", "-", Disco);
```

---

## Ejemplo de salida esperada

```txt
[Info]: Eliminando disco anterior en Disco_Inventory
[Creado]: Disco creado correctamente en Disco_Inventory.
[Tabla_SQL]: Tabla Creada desde SQL.
[CSV]: Filas insertadas: 154.

===== ESQUEMA INVENTORY =====

Tabla: INVENTORY
Size_Fila: 139 | Cantidad_Filas: 154
```

Ejemplo de búsqueda:

```txt
===== BUSQUEDA RANGO: quantity entre 20 y 30 =====

[Busqueda rango] quantity entre 20 y 30
PS-SL-A288 | LT-1 | PRODUCT | 20 | EA | 25000 | USD | ...
PS-PL-S236 | LT-2 | PRODUCT | 23 | EA | 28750 | USD | ...
PS-SL-A287 | LT-1 | PRODUCT | 25 | EA | 31250 | USD | ...
```

---

## Compilación

Este proyecto usa C++17 porque utiliza `std::filesystem`.

### Compilar en Visual Studio

Configurar:

```txt
Propiedades del proyecto
    → C/C++
    → Lenguaje
    → Estándar del lenguaje C++
    → ISO C++17
```

Luego ejecutar el proyecto normalmente.

## Recomendaciones para ejecutar

Los archivos:

```txt
Inventory.txt
Inventory_v4.csv
```

deben estar en la misma carpeta desde donde se ejecuta el `.exe`.

También se pueden usar rutas absolutas.

Ejemplo:

```cpp
Tabla Inventory = Data_Manager.Crear_Tabla_SQL(
    "C:/Users/Pochano/Desktop/UCSP/VII Semestre/Base de Datos II/Disco_Simulado/Inventory.txt"
);

Data_Manager.Cargar_CSV(
    "C:/Users/Pochano/Desktop/UCSP/VII Semestre/Base de Datos II/Disco_Simulado/Inventory_v4.csv",
    Inventory,
    Disco,
    false
);
```

---

## Notas importantes

- El disco simulado se crea como una carpeta.
- Si ya existe una carpeta de disco anterior, el programa puede eliminarla y crear una nueva.
- Los registros se guardan secuencialmente.
- Cada fila tiene tamaño fijo.
- Las filas pueden fragmentarse entre varios sectores.
- Cada columna tiene su propio índice AVL.
- Las búsquedas usan primero el AVL.
- El AVL devuelve los `Fila_ID`.
- Luego se leen los sectores correspondientes.
- Finalmente se reconstruye la fila usando el serializador.
- Los valores `VARCHAR(N)` pueden recortarse si superan el tamaño definido.
- Los enteros se guardan como 4 bytes reales, no como texto.
- Los sectores se manejan en modo binario para conservar correctamente los bytes.

---

## Resultado actual del proyecto

Con la configuración actual, el programa carga correctamente:

```txt
154 filas
Size_Fila = 139 bytes
Tabla = INVENTORY
```

Además, las consultas por igualdad y por rango funcionan correctamente sobre columnas como:

```txt
location
velocityCode
quantity
value
```

---

Agregar cambios (Verifiquen el branch plis):

```bash
git add .
```

Crear commit:

```bash
git commit -m "Información del cambio"
```

Subir cambios:

```bash
git push
```

Si el repositorio remoto tiene cambios que no están localmente:

```bash
git pull origin main --rebase
git push
```

---

Proyecto desarrollado para el curso de Base de Datos II.

Universidad Católica San Pablo.
