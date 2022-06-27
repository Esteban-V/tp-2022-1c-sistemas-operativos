#ifndef SOCKET_HEADERS_H_
#define SOCKET_HEADERS_H_

typedef enum headers {
	NEW_PROCESS = 0,
	IOCALL = 1,
	EXIT = 2,
	MEMORY_INFO = 3,
	PCB_TO_CPU = 4,
	INTERRUPT = 5,
	INTERRUPT_READY = 6,
	// Respuestas a consola
	PROCESS_OK = 7,
	PROCESS_FAILURE = 8,
	PCB_TO_KERNEL = 9,
} headers;

#endif /* SOCKET_HEADERS_H_ */
