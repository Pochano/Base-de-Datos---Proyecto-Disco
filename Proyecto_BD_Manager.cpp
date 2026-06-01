#include <iostream>

#include "Disco_Manager.h"
#include "Tablas.h"
#include "Data_Manager.h"

int main() {

	std::string Ruta_Disco = "Disco_Inventory";

	Disco_Manager Disco(Ruta_Disco, 2, 10, 50, 128);

	Disco.Crear_Disco();

	Manager_Data Data_Manager;

	Tabla Inventory = Data_Manager.Crear_Tabla_SQL("C:/Users/Pochano/Desktop/Inventory.txt");

	Data_Manager.Cargar_CSV("C:/Users/Pochano/Desktop/Inventory_v4.csv", Inventory, Disco, false);

	std::cout << "\n===== ESQUEMA INVENTORY =====\n";
	Inventory.Mostrar_Esquema();

	std::cout << "\n===== PRIMERAS FILAS =====\n";
	for (int i = 0; i < 5 && i < Inventory.get_Cantidad_Filas(); i++) {
		Inventory.Mostrar_Fila(i, Disco);
	}

	std::cout << "\n===== BUSQUEDA IGUALDAD: location = LT-1 =====\n";
	Inventory.Buscar_Igualdad("location", "LT-1", Disco);

	std::cout << "\n===== BUSQUEDA IGUALDAD: velocityCode = A =====\n";
	Inventory.Buscar_Igualdad("velocityCode", "A", Disco);

	std::cout << "\n===== BUSQUEDA RANGO: quantity entre 20 y 30 =====\n";
	Inventory.Buscar_Rango("quantity", "20", "30", Disco);

	std::cout << "\n===== BUSQUEDA RANGO: value >= 50000 =====\n";
	Inventory.Buscar_Rango("value", "50000", "-", Disco);

	std::cout << "\nFin del programa.\n";
	system("pause");

	return 0;
}