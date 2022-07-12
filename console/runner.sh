#!/bin/bash

# Algunas validaciones iniciales
if [ -z "$1" ]
then
	echo Falta la carpeta con los programas a ejecutar
	exit 1
fi

if [ ! -d "$1" ]
then
	echo "La carpeta de programas $1 no existe :("
	exit 1
fi

if [ -z "$2" ]
then
	echo Falta el archivo de config a usar
	exit 1
fi

if [ ! -f "$2" ]
then
	echo "El archivo de config $2 no existe :("
	exit 1
fi

if [ -z "$3" ]
then
	echo Falta especificar el tamaño del proceso: USANDO TAMAÑO DE PROCESO 4096
fi

if [ -z "$4" ]
then
	echo Falta especificar la cantidad de iteraciones: ITERANDO 1 SOLA VEZ
fi

# Los parametros en bash se referencian segun el orden, $1, $2, etc
FOLDER_WITH_PROGS=$1
CONFIG_FILE=$2
PROCESS_SIZE=${3:-4096}
SCRIPTS_QTY=${4:-1}

INTERVAL=0.5 # Para esperar, en segundos, entre cada ejecucion de la consola


for i in `seq 1 $SCRIPTS_QTY`
do
	echo Iniciando iteracion $i...
	for CODE in `ls $FOLDER_WITH_PROGS`
	do
		CODE_PATH=$FOLDER_WITH_PROGS/$CODE
		cp $CONFIG_FILE ./cfg/console.config
		LOG_PATH=./cfg/console_$CODE-$i.log
		echo Iniciando consola $CODE-$i en background y enviando log a $LOG_PATH...

		# Cada ejecucion redirecciona la salida estándar (la terminal) a un archivo propio 
		./console $CODE_PATH $PROCESS_SIZE  >$LOG_PATH &

		sleep $INTERVAL
	done
done 
