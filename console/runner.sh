#!/bin/bash

# Valida que la carpeta de "codigo" exista y sea un directorio
if [ -z "$1" ]
then
	echo "Falta la carpeta con los programas a ejecutar"
	exit 1
fi

if [ ! -d "$1" ]
then
	echo "La carpeta de programas $1 no existe :("
	exit 1
fi

# Avisa del tamaño default de las consolas al no ser especificado
if [ -z "$2" ]
then
	echo "Falta especificar el tamaño del proceso: USANDO 4096"
fi

FOLDER_WITH_PROGS=$1
# Usa 4096 para cada consola si no fue especificado como 2do parametro
PROCESS_SIZE=${2:-4096}
# Realiza una sola iteracion de cada consola si no fue especificado como 3er parametro
SCRIPTS_QTY=${3:-1}

for i in `seq 1 $SCRIPTS_QTY`
do
	if [ "$SCRIPTS_QTY" -gt "1" ]
	then
		echo Iniciando iteracion $i...
	fi

	for CODE in `ls $FOLDER_WITH_PROGS`
	do
		#Crea la carpeta de logs si no existe, y redirecciona los logs de cada consola alli
		mkdir -p ./run-logs
		LOG_PATH=./run-logs/console-$CODE-$i.log

		echo "Iniciando consola $CODE-$i en background y enviando log a $LOG_PATH..."

		# Obtiene el "codigo" a correr, y ejecuta la consola
		CODE_PATH=$FOLDER_WITH_PROGS/$CODE
		./console $CODE_PATH $PROCESS_SIZE  >$LOG_PATH &

		# Espera el intervalo especificado (en segundos) para ejecutar la proxima consola
		sleep 0.5
	done
done 
