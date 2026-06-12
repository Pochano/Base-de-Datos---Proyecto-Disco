#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Tablas.h"
#include "Disco_Manager.h"

using namespace std;

class Manager_Data 
{
private:

	string Limpiar(string Texto) 
	{
		while (!Texto.empty() && (Texto.front() == ' ' || Texto.front() == '\t' || Texto.front() == '\n' ||Texto.front() == '\r'))
		{
			Texto.erase(Texto.begin());
		}
		while (!Texto.empty() && (Texto.back() == ' ' || Texto.back() == '\t' || Texto.back() == '\n' || Texto.back() == '\r' || Texto.back() == ';')) 
		{
			Texto.pop_back();
		}
		return Texto;
	}

	string Leer_Archivo_Completo(string Ruta) 
	{
		ifstream Archivo(Ruta);
		if (!Archivo.is_open()) 
		{
			cout << "[Error]: No se pudo abrir el archivo (Leer_Archivo_Completo).\n";
			return "";
		}

		stringstream Buffer;

		Buffer << Archivo.rdbuf();
		Archivo.close();
		return Buffer.str();
	}

	vector<string> Serpar_Coma_SQL(string Texto) 
	{
		vector<string> Partes;
		string Actual = "";

		bool Dentro_Parentesis = false;
		bool Dentro_Comillas = false;

		for (char c : Texto) 
		{
			if (c == '\'' || c == '"') 
			{
				Dentro_Comillas = !Dentro_Comillas;
				Actual += c;
			}

			else if (c == '(' && !Dentro_Comillas) 
			{
				Dentro_Parentesis = true;
				Actual += c;
			}

			else if (c == ')' && !Dentro_Comillas) 
			{
				Dentro_Parentesis = false;
				Actual += c;
			}

			else if (c == ',' && !Dentro_Parentesis && !Dentro_Comillas) 
			{
				Partes.push_back(Limpiar(Actual));
				Actual = "";
			}

			else 
			{
				Actual += c;
			}
		}

		if (!Actual.empty()) 
		{
			Partes.push_back(Limpiar(Actual));
		}
		return Partes;
	}

	vector<string> Separar_Linea_CSV(string Linea) 
	{
		vector<string> Valores;
		string Actual = "";

		bool Dentro_Comilla = false;

		for (char c : Linea) 
		{
			if (c == '"') 
			{
				Dentro_Comilla = !Dentro_Comilla;
			}

			else if (c == ',' && !Dentro_Comilla) 
			{
				Valores.push_back(Limpiar(Actual));
				Actual = "";
			}

			else 
			{
				Actual += c;
			}
		}

		Valores.push_back(Limpiar(Actual));
		return Valores;

	}

	string Quitar_Comillas(string Texto) 
	{
		Texto = Limpiar(Texto);
		if (Texto.size() >= 2) 
		{
			if ((Texto.front() == '\'' && Texto.back() == '\'') || (Texto.front() == '"' && Texto.back() == '"')) 
			{
				Texto = Texto.substr(1, Texto.size() - 2);
			}
		}
		return Texto;
	}

public:

	Tabla Crear_Tabla_SQL(string Ruta_SQL) 
	{
		string Archivo_SQL = Leer_Archivo_Completo(Ruta_SQL);
		if (Archivo_SQL.empty()) 
		{
			cout << "[Error]: Achivo SQL vacio.\n";
			return Tabla("Error");
		}

		for (char& c : Archivo_SQL) 
		{
			if (c == '\n' || c == '\t' || c == '\r') 
			{
				c = ' ';
			}
		}

		string Palabra_Create = "CREATE TABLE";

		int Posicion_Create = Archivo_SQL.find(Palabra_Create);

		if (Posicion_Create == string::npos) 
		{
			cout << "[Error]: No se encontro CREATE TABLE.\n";
			return Tabla("Error");
		}

		int Posicion_Apertura = Archivo_SQL.find("(");
		int Posicion_Cierre = Archivo_SQL.rfind(")");

		if (Posicion_Apertura == string::npos || Posicion_Cierre == string::npos) 
		{
			cout << "[Error]: CREATE TABLE mal formado.\n";
			return Tabla("Error");
		}
		string Nombre_Tabla = Archivo_SQL.substr(Posicion_Create + Palabra_Create.size(), Posicion_Apertura - (Posicion_Create + Palabra_Create.size()));

		Nombre_Tabla = Limpiar(Nombre_Tabla);

		Tabla Nueva_Tabla(Nombre_Tabla);

		string Definicion_Columnas = Archivo_SQL.substr(Posicion_Apertura + 1,Posicion_Cierre - Posicion_Apertura - 1);

		vector<string> Columnas_SQL = Serpar_Coma_SQL(Definicion_Columnas);

		for (string Columna_SQL : Columnas_SQL) 
		{
			stringstream ss(Columna_SQL);
			string Nombre_Columna;
			string Tipo_SQL;

			ss >> Nombre_Columna;
			ss >> Tipo_SQL;

			if (Nombre_Columna.empty() || Tipo_SQL.empty()) 
			{
				cout << "[Error]: Columna mal definida: " << Columna_SQL << "\n";
				continue;
			}

			Tipo_Dato Tipo;
			int Size = 0;

			if (Tipo_SQL.find("VARCHAR") != string::npos) 
			{
				Tipo = Tipo_Dato::VARCHAR;

				int Posicion_A = Tipo_SQL.find("(");
				int Posicion_B = Tipo_SQL.find(")");

				if (Posicion_A == string::npos || Posicion_B == string::npos) 
				{
					cout << "[Error]: VARCHAR sin size: " << Tipo_SQL << "\n";
					continue;
				}

				string Numero = Tipo_SQL.substr(Posicion_A + 1, Posicion_B - Posicion_A - 1);
				Size = stoi(Numero);
			}

			else 
			{
				Tipo = String_a_Tipo(Tipo_SQL);

				if (Tipo == Tipo_Dato::INTEGER) 
				{
					Size = 4;
				}

				else if (Tipo == Tipo_Dato::FLOAT) 
				{
					Size = 4;
				}

				else if (Tipo == Tipo_Dato::BOOL) 
				{
					Size = 1;
				}

				else if (Tipo == Tipo_Dato::CHAR) 
				{
					Size = 1;
				}

				else 
				{
					cout << "[Error]: Tipo no reconocido: " << Tipo_SQL << "\n";
					continue;
				}
			}
			Nueva_Tabla.Agregar_Columna(Nombre_Columna, Tipo, Size);
		}
		cout << "[Tabla_SQL]: Tabla Creada desde SQL.\n";
		return Nueva_Tabla;
	}

	bool Cargar_CSV(string Ruta_CSV, Tabla& Tabla_R, Disco_Manager& Disco, bool Tiene_Encabezado = true) 
	{
		ifstream Archivo(Ruta_CSV);

		if (!Archivo.is_open()) 
		{
			cout << "[Error] No se puede abrir el csv.\n";
			return false;
		}

		string Linea;

		int Numero_Linea = 0;
		int Filas_Insertadas = 0;

		while (getline(Archivo, Linea)) 
		{
			Numero_Linea++;
			if (Linea.empty()) { continue; }
			if (Tiene_Encabezado && Numero_Linea == 1) { continue; }

			vector<string> Valores = Separar_Linea_CSV(Linea);

			for (string& Valor : Valores) 
			{
				Valor = Quitar_Comillas(Valor);
			}

			int Filas_Antes = Tabla_R.get_Cantidad_Filas();

			Tabla_R.Insertar_Fila(Valores, Disco);

			int Filas_Despues = Tabla_R.get_Cantidad_Filas();

			if (Filas_Despues > Filas_Antes) { Filas_Insertadas++; }
			else 
			{
				cout << "[Advertencia]: No se inserto la linea: " << Numero_Linea << " del csv.\n";
			}
		}
		Archivo.close();
		cout << "[CSV]: Filas insertadas: " << Filas_Insertadas << ".\n";
		return true;
	}
};



