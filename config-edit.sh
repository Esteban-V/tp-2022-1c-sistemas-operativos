#!/bin/bash

# Valida que el modulo exista y tenga su directorio
if [ -z "$1" ]
then
	echo "Falta el modulo a editar"
	exit 1
fi

if [ ! -d "./$1" ]
then
	echo "El modulo no existe"
	exit 1
fi

# Valida que la key y el value a reemplazar/agregar sean enviados
if [ -z "$2" ]
then
	echo "Falta el parametro a reemplazar"
	exit 1
fi

if [ -z "$3" ]
then
	echo "Falta el valor de $2"
	exit 1
fi

MODULE=$1
KEY=$2
VALUE=$3

FILE=./$MODULE/cfg/$MODULE.config

# Edita el valor de la key si la encuentra, la crea al final del archivo si no
sed -i \
    -e '/^#\?\(\s*'"${KEY}"'\s*=\s*\).*/{s//\1'"${VALUE}"'/;:a;n;ba;q}' \
    -e '$a'"${KEY}"'='"${VALUE}" $FILE