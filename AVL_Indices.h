#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "Serializador.h"

using namespace std;

struct Nodo 
{
	string Clave;
	vector<int> Filas_ID;
	Nodo* Izquierda;
	Nodo* Derecha;
	int Altura;

	Nodo(string clave, int fila_ID) 
	{
		Clave = clave;
		Filas_ID.push_back(fila_ID);
		Izquierda = nullptr;
		Derecha = nullptr;
		Altura = 1;
	}
};


class AVL_Index 
{
private:

	Nodo* Raiz;
	Tipo_Dato Tipo_Columa;
	int Altura(Nodo* Nodo_R) 
	{
		if (Nodo_R == nullptr) { return 0; }
		return Nodo_R->Altura;
	}

	int Factor_Balance(Nodo* Nodo_R) 
	{
		if (Nodo_R == nullptr) { return 0; }
		return Altura(Nodo_R->Izquierda) - Altura(Nodo_R->Derecha);
	}

	Nodo* Rotacion_Derecha(Nodo* Y) 
	{
		Nodo* X = Y->Izquierda;
		Nodo* T2 = X->Derecha;
		X->Derecha = Y;
		Y->Izquierda = T2;
		Y->Altura = max(Altura(Y->Izquierda), Altura(Y->Derecha)) + 1;
		X->Altura = max(Altura(X->Izquierda), Altura(X->Derecha)) + 1;

		return X;
	}

	Nodo* Rotacion_Izquierda(Nodo* X) 
	{
		Nodo* Y = X->Derecha;
		Nodo* T2 = Y->Izquierda;
		Y->Izquierda = X;
		X->Derecha = T2;
		X->Altura = max(Altura(X->Izquierda), Altura(X->Derecha)) + 1;
		Y->Altura = max(Altura(Y->Izquierda), Altura(Y->Derecha)) + 1;

		return Y;
	}

	int Comparar(string a, string b) 
	{
		if (Tipo_Columa == Tipo_Dato::INTEGER) 
		{
			int A = stoi(a);
			int B = stoi(b);
			if (A < B) { return -1; }
			if (A > B) { return 1; }
			else return 0;
		}

		else if (Tipo_Columa == Tipo_Dato::FLOAT) 
		{
			float A = stof(a);
			float B = stof(b);
			if (A < B) { return -1; }
			if (A > B) { return 1; }
			else return 0;
		}

		else if (Tipo_Columa == Tipo_Dato::BOOL) 
		{
			bool A = Convertir_Bool(a);
			bool B = Convertir_Bool(b);
			if (A == B) { return 0; }
			if (A == false && B == true) { return -1; }
			else return 1;
		}

		if (a < b) { return -1; }
		else if (a > b) { return 1; }

		return 0;
	}

	Nodo* Insertar_Recursivo(Nodo* Nodo_R, string Clave, int Fila_ID) 
	{
		if (Nodo_R == nullptr) 
		{
			return new Nodo(Clave, Fila_ID);
		}

		int Comparacion = Comparar(Clave, Nodo_R->Clave);

		if (Comparacion < 0) 
		{
			Nodo_R->Izquierda = Insertar_Recursivo(Nodo_R->Izquierda, Clave, Fila_ID);
		}

		else if (Comparacion > 0) 
		{
			Nodo_R->Derecha = Insertar_Recursivo(Nodo_R->Derecha, Clave, Fila_ID);
		}

		else 
		{
			Nodo_R->Filas_ID.push_back(Fila_ID);
			return Nodo_R;
		}

		Nodo_R->Altura = 1 + max(Altura(Nodo_R->Izquierda), Altura(Nodo_R->Derecha));

		int Balance = Factor_Balance(Nodo_R);

		if (Balance > 1) 
		{
			if (Factor_Balance(Nodo_R->Izquierda) >= 0) {
				return Rotacion_Derecha(Nodo_R);
			}
			else {
				Nodo_R->Izquierda = Rotacion_Izquierda(Nodo_R->Izquierda);
				return Rotacion_Derecha(Nodo_R);
			}
		}

		if (Balance < -1) 
		{
			if (Factor_Balance(Nodo_R->Derecha) <= 0) 
			{
				return Rotacion_Izquierda(Nodo_R);
			}
			else 
			{
				Nodo_R->Derecha = Rotacion_Derecha(Nodo_R->Derecha);
				return Rotacion_Izquierda(Nodo_R);
			}
		}
		return Nodo_R;
	}

	Nodo* Buscar_Nodo_Recusivo(Nodo* Nodo_R, string Clave) 
	{
		if (Nodo_R == nullptr) { return nullptr; }
		int Comparacion = Comparar(Clave, Nodo_R->Clave);
		if (Comparacion < 0) 
		{
			return Buscar_Nodo_Recusivo(Nodo_R->Izquierda, Clave);
		}
		else if (Comparacion > 0) 
		{
			return Buscar_Nodo_Recusivo(Nodo_R->Derecha, Clave);
		}
		else 
		{
			return Nodo_R;
		}
	}

	void Busqueda_Rango_Recusiva(Nodo* Nodo_R, string Menor_Valor, string Mayor_Valor, bool Usa_Maximo, bool Usa_Minimo, vector<int>& Resultados) 
	{
		if (Nodo_R == nullptr) { return; }

		bool Puede_ir_Derecha = true;
		bool Puede_ir_Izquierda = true;
		bool Puede_Agregar = true;

		if (Usa_Minimo) 
		{
			if (Comparar(Nodo_R->Clave, Menor_Valor) < 0) 
			{
				Puede_Agregar = false;
				Puede_ir_Izquierda = false;
			}
		}

		if (Usa_Maximo) 
		{
			if (Comparar(Nodo_R->Clave, Mayor_Valor) > 0) 
			{
				Puede_Agregar = false;
				Puede_ir_Derecha = false;
			}
		}

		if (Puede_ir_Izquierda) 
		{
			Busqueda_Rango_Recusiva(Nodo_R->Izquierda, Menor_Valor, Mayor_Valor, Usa_Maximo, Usa_Minimo, Resultados);
		}

		if (Puede_Agregar) 
		{
			for (int id : Nodo_R->Filas_ID) 
			{
				Resultados.push_back(id);
			}
		}

		if (Puede_ir_Derecha) 
		{
			Busqueda_Rango_Recusiva(Nodo_R->Derecha, Menor_Valor, Mayor_Valor, Usa_Maximo, Usa_Minimo, Resultados);
		}
	}

	void Recorrido_InOrder_Recursivo(Nodo* Nodo_R) 
	{
		if (Nodo_R == nullptr) { return; }
		Recorrido_InOrder_Recursivo(Nodo_R->Izquierda);
		cout << Nodo_R->Clave << " : [";
		for (int i = 0; i < Nodo_R->Filas_ID.size(); i++) 
		{
			cout << Nodo_R->Filas_ID[i];
			if (i + 1 < Nodo_R->Filas_ID.size()) 
			{
				cout << ", ";
			}
		}
		cout << "].\n";

		Recorrido_InOrder_Recursivo(Nodo_R->Derecha);
	}

public:

	Nodo* get_Raiz() { return Raiz; }

	AVL_Index() {
		Raiz = nullptr;
		Tipo_Columa = Tipo_Dato::VARCHAR;
	}

	AVL_Index(Tipo_Dato tipo) 
	{
		Raiz = nullptr;
		Tipo_Columa = tipo;
	}

	void Insertar(string Clave, int Fila_ID) 
	{
		Raiz = Insertar_Recursivo(Raiz, Clave, Fila_ID);
	}

	vector<int> Buscar_Igualdad(string Clave) 
	{
		Nodo* Nodo_Encontrado = Buscar_Nodo_Recusivo(Raiz, Clave);
		if (Nodo_Encontrado == nullptr) { return {}; }
		return Nodo_Encontrado->Filas_ID;
	}

	vector<int> Buscar_Rango(string Valor_Minimo, string Valor_Maximo) 
	{
		vector<int> Resultado;

		bool Usa_Maximo = true;
		bool Usa_Minimo = true;

		if (Valor_Minimo == "-") 
		{
			Usa_Minimo = false;
		}
		if (Valor_Maximo == "-") 
		{
			Usa_Maximo = false;
		}

		Busqueda_Rango_Recusiva(Raiz,Valor_Minimo,Valor_Maximo,Usa_Maximo,Usa_Minimo,Resultado);
		return Resultado;
	}

	void Motrar_In_Order()
	{	
		cout << "\n[Arbol AVL - " << Tipo_a_String(Tipo_Columa) << "]\n";
		Recorrido_InOrder_Recursivo(Raiz);
	}
};
