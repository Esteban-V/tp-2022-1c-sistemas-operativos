#!/bin/bash

# Valida que ambos parametros sean enviados
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

if [ -z "$2" ]
then
	echo "Falta el parametro a reemplazar"
	exit 1
fi

if [ -z "$3" ]
then
	echo "Falta el valor con el que reemplazar $1"
	exit 1
fi

MODULE=$1
KEY=$2
VALUE=$3

FILE=./$MODULE/cfg/$MODULE.config
REPLACE="$KEY=$VALUE"

# sed -i -e "s/.*\b$KEY\b.*/$REPLACE/" ./$MODULE/cfg/$MODULE.config

sed -i \
    -e '/^#\?\(\s*'"${KEY}"'\s*=\s*\).*/{s//\1'"${VALUE}"'/;:a;n;ba;q}' \
    -e '$a'"${KEY}"'='"${VALUE}" $FILE