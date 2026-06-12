#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std;

enum class Tipo_Dato 
{
    INTEGER,
    FLOAT,
    BOOL,
    CHAR,
    VARCHAR
};

struct Columna 
{
    string Nombre;
    Tipo_Dato Tipo;
    int Size;

    Columna(string nombre, Tipo_Dato tipo, int size) 
    {
        Nombre = nombre;
        Tipo = tipo;
        Size = size;
    }
};

Tipo_Dato String_a_Tipo(string Tipo) 
{
    if (Tipo == "INTEGER") return Tipo_Dato::INTEGER;
    if (Tipo == "FLOAT") return Tipo_Dato::FLOAT;
    if (Tipo == "BOOL") return Tipo_Dato::BOOL;
    if (Tipo == "CHAR") return Tipo_Dato::CHAR;
    if (Tipo == "VARCHAR") return Tipo_Dato::VARCHAR;

    return Tipo_Dato::VARCHAR;
}

string Tipo_a_String(Tipo_Dato Tipo) 
{
    if (Tipo == Tipo_Dato::INTEGER) return "INTEGER";
    if (Tipo == Tipo_Dato::FLOAT) return "FLOAT";
    if (Tipo == Tipo_Dato::BOOL) return "BOOL";
    if (Tipo == Tipo_Dato::CHAR) return "CHAR";
    if (Tipo == Tipo_Dato::VARCHAR) return "VARCHAR";

    return "VARCHAR";
}

bool Convertir_Bool(string Valor) 
{
    if (Valor == "1") return true;
    if (Valor == "true") return true;
    if (Valor == "TRUE") return true;
    if (Valor == "True") return true;

    return false;
}

class Serializador 
{
public:

    string Serializar_Integer(int32_t Valor) 
    {
        string Bytes(4, '\0');

        Bytes[0] = Valor & 0xFF;
        Bytes[1] = (Valor >> 8) & 0xFF;            
        Bytes[2] = (Valor >> 16) & 0xFF;                               
        Bytes[3] = (Valor >> 24) & 0xFF;
        
        return Bytes;
    }

    int32_t Deserializar_Integer(string Bytes)
    {
        int32_t Valor = 0;

        Valor |= (uint8_t)Bytes[0];
        Valor |= (uint8_t)Bytes[1] << 8;
        Valor |= (uint8_t)Bytes[2] << 16;
        Valor |= (uint8_t)Bytes[3] << 24;

        return Valor;
    }

    string Serializar_Float(float Valor) 
    {
        string Bytes(4, '\0');
        memcpy(&Bytes[0], &Valor, 4);
        return Bytes;
    }

    float Deserializar_Float(string Bytes) 
    {
        float Valor;
        memcpy(&Valor, Bytes.data(), 4);
        return Valor;
    }

    string Serializar_Bool(bool Valor) 
    {
        string Bytes(1, '\0');
        Bytes[0] = Valor;
        return Bytes;
    }

    bool Deserializar_Bool(string Bytes) 
    {
        return Bytes[0] == 1;
    }

    string Serializar_Char(char Valor) 
    {
        string Bytes(1, '\0');
        Bytes[0] = Valor;
        return Bytes;
    }

    char Deserializar_Char(string Bytes) 
    {
        return Bytes[0];
    }

    string Serializar_Varchar(string Valor, int Size_Varchar) 
    {
        if ((int)Valor.size() > Size_Varchar) 
        {
            Valor = Valor.substr(0, Size_Varchar);
        }

        while ((int)Valor.size() < Size_Varchar) 
        {
            Valor += '_';
        }
        return Valor;
    }

    string Deserializar_Varchar(string Bytes) 
    {
        while (!Bytes.empty() && Bytes.back() == '_') 
        {
            Bytes.pop_back();
        }
        return Bytes;
    }

    string Serializar_Valor(string Valor, Tipo_Dato Tipo, int Size_Op) 
    {
        if (Tipo == Tipo_Dato::INTEGER) 
        {
            return Serializar_Integer(stoi(Valor));
        }

        else if (Tipo == Tipo_Dato::FLOAT) 
        {
            return Serializar_Float(stof(Valor));
        }

        else if (Tipo == Tipo_Dato::BOOL) 
        {
            return Serializar_Bool(Convertir_Bool(Valor));
        }

        else if (Tipo == Tipo_Dato::CHAR) 
        {
            if (Valor.empty()) return Serializar_Char('\0');
            return Serializar_Char(Valor[0]);
        }
        
        else if (Tipo == Tipo_Dato::VARCHAR) 
        {
            return Serializar_Varchar(Valor, Size_Op);
        }
        return "";
    }


    string Deserializar_Valor(string Bytes, Tipo_Dato Tipo) 
    {
        if (Tipo == Tipo_Dato::INTEGER) 
        {
            return to_string(Deserializar_Integer(Bytes));
        }

        else if (Tipo == Tipo_Dato::FLOAT) 
        {
            return to_string(Deserializar_Float(Bytes));
        }

        else if (Tipo == Tipo_Dato::BOOL) 
        {
            return Deserializar_Bool(Bytes) ? "true" : "false";
        }

        else if (Tipo == Tipo_Dato::CHAR) 
        {
            return string(1, Deserializar_Char(Bytes));
        }

        else if (Tipo == Tipo_Dato::VARCHAR) 
        {
            return Deserializar_Varchar(Bytes);
        }
        return "";
    }
};

