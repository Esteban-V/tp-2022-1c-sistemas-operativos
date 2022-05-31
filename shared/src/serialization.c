#include"serialization.h"

t_stream_buffer* create_stream(size_t size) {
	t_stream_buffer *stream = malloc(sizeof(t_stream_buffer));
	if (stream == NULL) {
		return NULL;
	}

	stream->offset = 0;
	stream->malloc_size = size;

	stream->stream = malloc(size);
	if (stream->stream == NULL) {
		free(stream);
		return NULL;
	}

	return stream;
}

void stream_destroy(t_stream_buffer *stream) {
	free(stream->stream);
	free(stream);
}

void stream_add(t_stream_buffer *stream, void *source, size_t size) {
	// TODO: Que no agregue tamano de +
	while (stream->malloc_size < stream->offset + size) {
		stream->malloc_size += STREAM_SIZE_DEF;
		stream->stream = realloc(stream->stream, stream->malloc_size);
	}
	memcpy(stream->stream + stream->offset, source, size);
	stream->offset += size;
}

void stream_add_UINT32P(t_stream_buffer *stream, void *source) {
	stream_add(stream, source, sizeof(uint32_t));
}

void stream_add_UINT32(t_stream_buffer *stream, uint32_t value) {
	stream_add_UINT32P(stream, (void*) &value);
}

void stream_add_STRINGP(t_stream_buffer *stream, void *source) {
	uint32_t size = string_length((char*) source) + 1;
	stream_add_UINT32(stream, size);
	stream_add(stream, source, size);
}

void stream_add_STRING(t_stream_buffer *stream, char *source) {
	stream_add_STRINGP(stream, (void*) source);
}

void stream_add_LIST(t_stream_buffer *stream, t_list *source,
		void (*stream_add_ELEMP)(t_stream_buffer*, void*)) {

	void _stream_add_ELEMP(void *elem) {
		stream_add_ELEMP(stream, elem);
	}

	uint32_t size = source->elements_count;
	stream_add_UINT32(stream, size);
	list_iterate(source, _stream_add_ELEMP);
}

void stream_take(t_stream_buffer *stream, void **dest, size_t size) {
	if (*dest == NULL)
		*dest = calloc(1, size);
	memcpy(*dest, stream->stream + stream->offset, size);
	stream->offset += size;
}

void stream_take_UINT32P(t_stream_buffer *stream, void **dest) {
	stream_take(stream, dest, sizeof(uint32_t));
}

uint32_t stream_take_UINT32(t_stream_buffer *stream) {
	uint32_t uint;
	uint32_t *uint_p = &uint;
	stream_take_UINT32P(stream, (void**) &uint_p);
	return uint;
}

void stream_take_STRINGP(t_stream_buffer *stream, void **dest) {
	uint32_t size = stream_take_UINT32(stream);
	stream_take(stream, dest, size);
}

char* stream_take_STRING(t_stream_buffer *stream) {
	char *string = NULL;
	stream_take_STRINGP(stream, (void**) &string);
	return string;
}

void stream_take_LISTP(t_stream_buffer *stream, t_list **source,
		void (*stream_take_ELEMP)(t_stream_buffer*, void**)) {
	if (*source == NULL)
		*source = list_create();
	uint32_t size = stream_take_UINT32(stream);

	for (uint32_t i = 0; i < size; i++) {
		void *elem = NULL;
		stream_take_ELEMP(stream, &elem);
		list_add(*source, elem);
	}
}

t_list* stream_take_LIST(t_stream_buffer *stream,
		void (*stream_take_ELEMP)(t_stream_buffer*, void**)) {
	t_list *tmp = NULL;
	stream_take_LISTP(stream, &tmp, stream_take_ELEMP);
	return tmp;
}

