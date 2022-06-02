#ifndef CPU_SRC_CPU_H_
#define CPU_SRC_CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<pthread.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<commons/collections/dictionary.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<assert.h>
#include"networking.h"
#include"serialization.h"
#include"socket_headers.h"
#include"process_utils.h"
#include"utils.h"

t_log *logger;
t_CPUConfig *config;
pthread_mutex_t mutex_log;

#endif /* CPU_SRC_CPU_H_ */
