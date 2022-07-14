#ifndef SOCKET_HEADERS_H_
#define SOCKET_HEADERS_H_

typedef enum console_headers
{
	PROCESS_OK = 0,
	PROCESS_FAILURE = 1,
} console_headers;

typedef enum kernel_headers
{
	NEW_PROCESS = 0,
	IO_CALL = 1,
	EXIT_CALL = 2,
	INTERRUPT_DISPATCH = 3,
	SUSPEND = 4
} kernel_headers;

typedef enum cpu_headers
{
	PCB_TO_CPU = 0,
	INTERRUPT = 1,
} cpu_headers;

typedef enum memory_headers
{
	MEMORY_PID = 0,
	SWAP_OK = 1,
	SWAP_ERROR = 2,
	FRAME = 3,
	MEMORY_INFO = 4,
	HANDSHAKE = 5
} memory_headers;

#endif /* SOCKET_HEADERS_H_ */