#ifndef SOCKET_HEADERS_H_
#define SOCKET_HEADERS_H_

typedef enum headers {
	NEW_PROCESS = 0,
	IO=1,
	EXIT=2,
	MEMORY_INFO = 3,
	PCB_TO_CPU=4,
	INTERRUPT=5
} headers;

#endif /* SOCKET_HEADERS_H_ */
