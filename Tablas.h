#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Serializador.h"
#include "Disco_Manager.h"
#include "AVL_Indices.h"

using namespace std;


class Tabla 
{
private:

	string Nombre_Tabla;
	vector<Columna> Columnas;
	int Size_Fila;
	int Cantidad_Filas;
	Serializador Serializador_Local;
	map<string, AVL_Index> Indices;

public:

	Tabla(string nombre) 
	{
		Nombre_Tabla = nombre;
		Size_Fila = 0;
		Cantidad_Filas = 0;
	}

	AVL_Index* get_Indice(string nombre) 
	{
		auto it = Indices.find(nombre);
		if (it == Indices.end()) return nullptr;
		return &it->second;
	}

	void Agregar_Columna(string Nombre, Tipo_Dato Tipo, int Size) 
	{
		if (Tipo == Tipo_Dato::INTEGER) { Size = 4; }
		else if (Tipo == Tipo_Dato::FLOAT) { Size = 4; }
		else if (Tipo == Tipo_Dato::BOOL) { Size = 1; }
		else if (Tipo == Tipo_Dato::CHAR) { Size = 1; }
		
		Columna Nueva_Columna(Nombre, Tipo, Size);
		Columnas.push_back(Nueva_Columna);
		Indices[Nombre] = AVL_Index(Tipo);
		Size_Fila += Size;
	}

	int get_Size_Fila() { return Size_Fila; }
	int get_Cantidad_Filas() { return Cantidad_Filas; }
													
	string Serializar_Fila(vector<string> Valores) 
	{
		if (Valores.size() != Columnas.size()) 
		{
			cout << "[Error]: Cantidad de valores no coinciden con la cantidad de columnas (Serializar_Fila).\n";
			return "";
		}

		string Fila_Serializada = "";

		for (int i = 0; i < Columnas.size(); i++) 
		{
			string Bytes = Serializador_Local.Serializar_Valor(Valores[i], Columnas[i].Tipo, Columnas[i].Size);
			Fila_Serializada += Bytes;
		}
		return Fila_Serializada;
	}

	void Insertar_Fila(vector<string> Valores, Disco_Manager& Disco) 
	{
		string Fila_Serializada = Serializar_Fila(Valores);
		if ((int)Fila_Serializada.size() != Size_Fila) 
		{
			cout << "[Error]: La fila serializada no tiene el tamaño esperado (Insertar_Fila).\n";
			return;
		}

		int Fila_ID = Cantidad_Filas;

		bool Exito = Disco.Escribir_Sectores(Fila_ID, Fila_Serializada, Size_Fila);

		if (Exito) 
		{
			for (int i = 0; i < Columnas.size(); i++) 
			{
				string Clave_Indice = Valores[i];
				if (Columnas[i].Tipo == Tipo_Dato::BOOL) 
				{
					Clave_Indice = Convertir_Bool(Valores[i]) ? "true" : "false";
				}
				Indices[Columnas[i].Nombre].Insertar(Clave_Indice, Fila_ID);
			}
			Cantidad_Filas++;
		}
	}

	string Leer_Fila_Bytes(int Fila_ID, Disco_Manager& Disco) 
	{
		if (Fila_ID < 0 || Fila_ID >= Cantidad_Filas) 
		{
			cout << "[Error]: Fila_ID Fuera de rango (Leer_Fila_Bytes).\n";
			return "";
		}

		vector<Fragmento> Fragmentos = Disco.Calcular_Fragmentos_Fila(Fila_ID, Size_Fila);

		if (Fragmentos.empty()) 
		{
			cout << "[Error]: No se pudieron calcular fragmentos de la fila (Leer_Fila_Bytes).\n";
			return "";
		}

		string Fila_Bytes = "";

		for (Fragmento Fragmento_R : Fragmentos)
		{
			Direccion_Fisica Posicion_Fisica;
			Posicion_Fisica.Plato = Fragmento_R.Plato;
			Posicion_Fisica.Superficie = Fragmento_R.Superficie;
			Posicion_Fisica.Pista = Fragmento_R.Pista;
			Posicion_Fisica.Sector = Fragmento_R.Sector;
			
			int Cantidad_Bytes = Fragmento_R.Byte_Fin - Fragmento_R.Byte_Inicio + 1;
			vector<char> Partes = Disco.Leer_Sectores(Posicion_Fisica, Fragmento_R.Byte_Inicio, Cantidad_Bytes);

			if (Partes.empty()) 
			{
				cout << "[Error]: No se pudo leer un fragmento de la fila (Leer_Fila_Bytes).\n";
				return "";
			}

			Fila_Bytes.append(Partes.data(), Partes.size());			
		}
		return Fila_Bytes;
	}

	vector<string> Leer_Fila(int Fila_ID, Disco_Manager& Disco) 
	{
		vector<string> Valores;
		string Fila_Bytes = Leer_Fila_Bytes(Fila_ID, Disco);

		if ((int)Fila_Bytes.size() != Size_Fila) 
		{
			cout << "[Error]: La fila leida no tiene el size esperado (Leer_Fila).\n";
			return {};
		}

		int Posicion = 0;

		for (Columna Columna_R : Columnas) 
		{
			string Bytes_Campo = Fila_Bytes.substr(Posicion, Columna_R.Size);
			string Bytes_Serializados = Serializador_Local.Deserializar_Valor(Bytes_Campo, Columna_R.Tipo);
			Valores.push_back(Bytes_Serializados);
			Posicion += Columna_R.Size;
		}
		return Valores;
	}

	void Mostrar_Fila(int Fila_ID, Disco_Manager& Disco) 
	{
		vector<string> Valores = Leer_Fila(Fila_ID, Disco);

		if (Valores.empty()) 
		{
			cout << "[Error]: No se pudo mostrar la fila (Mostrar_Fila).\n";
			return;
		}

		for (auto Valor_Columna : Valores) 
		{
			cout << Valor_Columna << " | ";
		}
		cout << "\n";
	}

	void Mostrar_Esquema()
	{
		cout << "\nTabla: " << Nombre_Tabla << "\n";
		cout << "Size_Fila: " << Size_Fila << " | Cantidad_Filas: " << Cantidad_Filas << "\n";
		for (Columna ColumnaR : Columnas) 
		{
			cout << "- " << ColumnaR.Nombre << ": " << Tipo_a_String(ColumnaR.Tipo) << ", " << ColumnaR.Size << "\n";
		}
		cout << "\n";
	}

	void Buscar_Igualdad(string Nombre_Columna, string Valor, Disco_Manager& Disco) 
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) 
		{
			cout << "[Error]: No existe la columna " << Nombre_Columna << " (Buscar_Igualdad).\n";
			return;
		}

		vector<int> Filas = Indices[Nombre_Columna].Buscar_Igualdad(Valor);

		if (Filas.empty()) 
		{
			cout << "[Resultado_Busqueda]: No hay resultados.\n";
			return;
		}

		cout << "\n[Busqueda igualdad] " << Nombre_Columna << " = " << Valor << "\n";

		for (int Fila_ID : Filas) 
		{
			Mostrar_Fila(Fila_ID, Disco);
		}
	}

	void Buscar_Rango(string Nombre_Columna, string Menor_Valor, string Mayor_Valor, Disco_Manager& Disco) 
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) 
		{
			cout << "[Error]: No existe la columna " << Nombre_Columna << " (Buscar_Igualdad).\n";
			return;
		}
		vector<int> Filas = Indices[Nombre_Columna].Buscar_Rango(Menor_Valor, Mayor_Valor);

		if (Filas.empty()) 
		{
			cout << "[Resultado_Busqueda]: No hay resultados en el rango.\n";
			return;
		}
		cout << "\n[Busqueda rango] " << Nombre_Columna << " entre " << Menor_Valor << " y " << Mayor_Valor << "\n";
		
		for (int Fila_ID : Filas) 
		{
			Mostrar_Fila(Fila_ID, Disco);
		}
	}

	string get_Nombre_Tabla() { return Nombre_Tabla; }
	vector<Columna> get_Columnas() { return Columnas; }

	vector<vector<string>> Buscar_Igualdad_GUI(
		string Nombre_Columna, string Valor, Disco_Manager& Disco)
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) return {};
		vector<int> IDs = Indices[Nombre_Columna].Buscar_Igualdad(Valor);
		vector<vector<string>> Resultados;
		for (int id : IDs) Resultados.push_back(Leer_Fila(id, Disco));
		return Resultados;
	}

	vector<vector<string>> Buscar_Rango_GUI(
		string Nombre_Columna, string Minimo, string Maximo, Disco_Manager& Disco)
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) return {};
		vector<int> IDs = Indices[Nombre_Columna].Buscar_Rango(Minimo, Maximo);
		vector<vector<string>> Resultados;
		for (int id : IDs) Resultados.push_back(Leer_Fila(id, Disco));
		return Resultados;
	}

	vector<int> Buscar_Igualdad_IDs(string Nombre_Columna, string Valor) 
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) return {};
		return Indices[Nombre_Columna].Buscar_Igualdad(Valor);
	}
	vector<int> Buscar_Rango_IDs(string Nombre_Columna, string Min, string Max) 
	{
		if (Indices.find(Nombre_Columna) == Indices.end()) return {};
		return Indices[Nombre_Columna].Buscar_Rango(Min, Max);
	}
};

