#ifndef MEMMORY_H_
#define MEMMORY_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<pthread.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<pthread.h>
#include"serialization.h"
#include"networking.h"
#include"socket_headers.h"
#include"utils.h"
#include"process_utils.h"

void* header_handler(void *_client_socket);
t_log *logger;
t_memoryConfig *config;

#endif /* MEMMORY_H_ */
