#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

using namespace std;

struct Direccion_Fisica 
{
	int Plato;
	int Superficie;
	int Pista;
	int Sector;
};

struct Fragmento 
{
	int Plato;
	int Superficie;
	int Pista;
	int Sector;
	int Byte_Inicio;
	int Byte_Fin;
};

class Disco_Manager
{
private:
	string Ruta_Base;
	int Platos_Cantidad;
	int Pistas_Cantidad;
	int Sectores_Cantidad;
	long Capacidad_Sector;
	const int Superficies_Cantidad = 2;

public:
	Disco_Manager( string& Ruta, int Platos, int Pistas, int Sectores, long Capacidad) 
	{
		Ruta_Base = Ruta;
		Platos_Cantidad = Platos;
		Pistas_Cantidad = Pistas;
		Sectores_Cantidad = Sectores;
		Capacidad_Sector = Capacidad;
	}
	bool Crear_Disco() 
	{
		if ( filesystem::exists(Ruta_Base)) 
		{
			 cout << "[Info]: Eliminando disco anterior en " << Ruta_Base << "\n";
			 filesystem::remove_all(Ruta_Base);
		}

		filesystem::create_directories(Ruta_Base);

		for (int plato = 0; plato < Platos_Cantidad; plato++) 
		{
			for (int superficie = 0; superficie < Superficies_Cantidad; superficie++) 
			{
				string Superficie_Cara = (superficie == 0) ? "/Superficie_A" : "/Superficie_B";
				for (int pista = 0; pista < Pistas_Cantidad; pista++) 
				{
					string Ruta_Pista = Ruta_Base + "/Plato_" +  to_string(plato) + Superficie_Cara + "/Pista_" +  to_string(pista);
					filesystem::create_directories(Ruta_Pista);

					for (int sector = 0; sector < Sectores_Cantidad; sector++) 
					{
						 string Ruta_Sector = Ruta_Pista + "/Sector_" +  to_string(sector) + ".txt";
						 ofstream Archivo(Ruta_Sector);

						if (!Archivo.is_open()) 
						{
						    cout << "[Error]: Error al crear sector (Crear_Disco).\n";
							return false;
						}

						 string Vacio_Simulado(Capacidad_Sector, '_');

						Archivo << Vacio_Simulado;
						Archivo.close();
					}
				}
			}
		}
		cout << "[Creado]: Disco creado correctamente en " + Ruta_Base << ".\n";
		return true;
	}

	int get_Platos() { return Platos_Cantidad; }
	int get_Pistas() { return Pistas_Cantidad; }
	int get_Sectores() { return Sectores_Cantidad; }
	int get_Capacidad_Sectores() { return Capacidad_Sector; }
	int get_Total_Sectores() { return (Sectores_Cantidad * Pistas_Cantidad * Superficies_Cantidad * Platos_Cantidad); }
	int get_Total_Capacidad() { return get_Total_Sectores() * Capacidad_Sector; }; 

	Direccion_Fisica Convertir_Sector_Global(int Sector_Global) 
	{
		Direccion_Fisica Posicion_Fisica;
		int Sectores_por_Superficie = Pistas_Cantidad * Sectores_Cantidad;
		int Sectores_por_Plato = Sectores_por_Superficie * Superficies_Cantidad;
		Posicion_Fisica.Plato = Sector_Global / Sectores_por_Plato;
		int Resto_Plato = Sector_Global % Sectores_por_Plato;
		Posicion_Fisica.Superficie = Resto_Plato / Sectores_por_Superficie;
		int Resto_Superficie = Resto_Plato % Sectores_por_Superficie;
		Posicion_Fisica.Pista = Resto_Superficie / Sectores_Cantidad;
		Posicion_Fisica.Sector = Resto_Superficie % Sectores_Cantidad;

		return Posicion_Fisica;
	}

	 string Obtener_Ruta_Sector(Direccion_Fisica& Posicion_Fisica) 
	 {
		string Nombre_Superficie = (Posicion_Fisica.Superficie == 0) ? "/Superficie_A" : "/Superficie_B";
		string Ruta_Sector = Ruta_Base + "/Plato_" +  to_string(Posicion_Fisica.Plato) + Nombre_Superficie + "/Pista_" +  to_string(Posicion_Fisica.Pista) + "/Sector_" +  to_string(Posicion_Fisica.Sector) + ".txt";

		return Ruta_Sector;
	 }

	bool Sector_Existente(Direccion_Fisica& Posicion_Fisica) 
	{
		string Ruta_Sector = Obtener_Ruta_Sector(Posicion_Fisica);
		return  filesystem::exists(Ruta_Sector);
	}

	bool Disco_con_Espacio(int Fila_ID, int Size_Fila) 
	{
		int Offset_Final = (Fila_ID * Size_Fila) + Size_Fila;
		return Offset_Final <= get_Total_Capacidad();
	}

	bool Escribir_Sectores(int Fila_ID,  string Fila_Data, int Size_Fila) 
	{
		if ((int)Fila_Data.size() != Size_Fila) 
		{
		    cout << "[Error]: La fila no tiene el tama?o indicado (Escribir_Sectores).\n";
			return false;
		}
		vector<Fragmento> Fragmentos = Calcular_Fragmentos_Fila(Fila_ID, Size_Fila);

		if (Fragmentos.empty()) 
		{
			cout << "[Error]: No se pudo calcular la ubicacion de la fila (Escribir_Sectores).\n";
			return false;
		}

		int Posicion_Fila = 0;

		for (Fragmento Frag : Fragmentos) 
		{
			Direccion_Fisica Posicion_Fisica;
			Posicion_Fisica.Plato = Frag.Plato;
			Posicion_Fisica.Superficie = Frag.Superficie;
			Posicion_Fisica.Pista = Frag.Pista;
			Posicion_Fisica.Sector = Frag.Sector;
			 string Ruta_Sector = Obtener_Ruta_Sector(Posicion_Fisica);
			 fstream Archivo(Ruta_Sector,  ios::in |  ios::out |  ios::binary);

			if (!Archivo.is_open()) 
			{
				cout << "[Error] No se pudo abrir el archivo. (Escribir_Sectores)\n";
				return false;
			}

			int Bytes_a_Escribir = Frag.Byte_Fin - Frag.Byte_Inicio + 1;
			Archivo.seekp( Frag.Byte_Inicio );
			Archivo.write( Fila_Data.data() + Posicion_Fila, Bytes_a_Escribir );
			Archivo.close();
			Posicion_Fila += Bytes_a_Escribir;
		}
		return true;
	}

	 vector<char> Leer_Sectores(Direccion_Fisica Posicion_Fisica, int Byte_Inicio, int Cantidad_Bytes) 
	 {
		 vector<char> Buffer(Cantidad_Bytes);

		if (!Sector_Existente(Posicion_Fisica)) 
		{
			cout << "[Error]: El sector no existe. (Leer_Sectores).\n";
			return {};
		}

		if (Byte_Inicio < 0 || Byte_Inicio >= Capacidad_Sector) 
		{
			cout << "[Error]: Byte de inicio no valido (Leer_Sectores).\n";
			return {};
		}

		if (Byte_Inicio + Cantidad_Bytes > Capacidad_Sector) 
		{
			cout << "[Error]: Cantidad de bytes a leer excede la capacidad (Leer_Sectores).\n";
			return {};
		}

		 string Ruta_Sector = Obtener_Ruta_Sector(Posicion_Fisica);

		 ifstream Archivo(Ruta_Sector,  ios::binary);

		if (!Archivo.is_open()) 
		{
			 cout << "[Error]: No se pudo abrir el archivo (Leer_Sectores).\n";
		}

		Archivo.seekg(Byte_Inicio);
		Archivo.read(Buffer.data(), Cantidad_Bytes);
		Archivo.close();
		return Buffer;
	}

	 vector<Fragmento> Calcular_Fragmentos_Fila(int Fila_ID, int Size_Fila)
	 {
		if (!Disco_con_Espacio(Fila_ID, Size_Fila)) 
		{
			cout << "[Error]: No hay espacio suficiente en disco (Calcular_Fragmentos_Fila).\n";
			return {};
		}

		 vector<Fragmento> Fragmentos;

		int Offset_Global = Fila_ID * Size_Fila;
		int Sector_Global = Offset_Global / Capacidad_Sector;
		int Byte_Inicio = Offset_Global % Capacidad_Sector;
		int Bytes_Restantes = Size_Fila;

		while (Bytes_Restantes > 0) 
		{
			int Espacio_Disponible = Capacidad_Sector - Byte_Inicio;
			int Bytes_Tomados =  min(Bytes_Restantes, Espacio_Disponible);

			Direccion_Fisica Posicion_Fisica = Convertir_Sector_Global(Sector_Global);

			Fragmento Fragmentito;

			Fragmentito.Plato = Posicion_Fisica.Plato;
			Fragmentito.Superficie = Posicion_Fisica.Superficie;
			Fragmentito.Pista = Posicion_Fisica.Pista;
			Fragmentito.Sector = Posicion_Fisica.Sector;
			Fragmentito.Byte_Inicio = Byte_Inicio;
			Fragmentito.Byte_Fin = Byte_Inicio + Bytes_Tomados - 1;

			Fragmentos.push_back(Fragmentito);

			Bytes_Restantes -= Bytes_Tomados;
			Sector_Global++;
			Byte_Inicio = 0;
		}
		return Fragmentos;
	}
};
