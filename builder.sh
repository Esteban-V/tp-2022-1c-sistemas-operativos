#!/bin/bash

# Valida que la carpeta de configs exista y sea un directorio
if [ -z "$1" ]
then
	echo "Falta la carpeta con las config a aplicar"
	exit 1
fi

if [ ! -d "$1" ]
then
	echo "La carpeta de configs $1 no existe :("
	exit 1
fi

FOLDER_WITH_CONFIGS=$1

for CONFIG in `ls $FOLDER_WITH_CONFIGS`
do
	# Identifica a que modulo pertenece la config
	MODULE=$(echo $CONFIG| cut -d. -f1)

	#Crea la carpeta de logs si no existe, y redirecciona los logs de build de cada modulo alli
	mkdir -p ./build-logs
	LOG_PATH=./build-logs/$MODULE-$(basename $FOLDER_WITH_CONFIGS).log

	# Obtiene la config a utilizar y la copia en el directorio correspondiente de cada modulo
	CONFIG_FILE=$FOLDER_WITH_CONFIGS/$CONFIG
	cp $CONFIG_FILE ./$MODULE/cfg/$CONFIG

	# Buildea el modulo
	echo "Buildeando $MODULE con config $CONFIG_FILE en background y enviando log a $LOG_PATH..."
	(cd ./$MODULE && make) >$LOG_PATH &
done
