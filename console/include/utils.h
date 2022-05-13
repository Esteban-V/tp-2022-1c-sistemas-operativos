/*
 * utils.h
 *
 *  Created on: May 9, 2022
 *      Author: utn-so
 */

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#include<commons/config.h>
#include<commons/log.h>

t_log *new_logger;
t_config *new_config;

FILE* open_file(char *path);

t_log* create_logger();
t_config* create_config();

#endif /* INCLUDE_UTILS_H_ */
