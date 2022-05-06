/*
 * serialization.h
 *
 *  Created on: Apr 30, 2022
 *      Author: utn-so
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>


#define STREAM_SIZE_DEF 64

typedef struct t_stream_buffer {
    uint32_t offset;
    size_t malloc_size;
    char* stream;
} t_stream_buffer;


t_stream_buffer* create_stream(size_t size);
void stream_destroy(t_stream_buffer *stream);

void stream_add(t_stream_buffer *stream, void *source, size_t size);

void stream_add_UINT32P(t_stream_buffer *stream, void *source);
void stream_add_UINT32(t_stream_buffer *stream, uint32_t value);
void stream_add_STRINGP(t_stream_buffer *stream, void *source);
void streama_add_STRING(t_stream_buffer *stream, char *source);


void stream_take(t_stream_buffer *stream, void **dest, size_t size);

void stream_take_UINT32P(t_stream_buffer *stream, void **dest);
uint32_t stream_take_UINT32(t_stream_buffer *stream);
void stream_take_STRINGP(t_stream_buffer *stream, void **dest);
char* stream_take_STRING(t_stream_buffer *stream);


#endif /* SERIALIZATION_H_ */
