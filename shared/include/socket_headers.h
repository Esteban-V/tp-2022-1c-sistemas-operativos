#ifndef SOCKET_HEADERS_H_
#define SOCKET_HEADERS_H_

#define CPU_MEM_SECRET 37562
typedef enum console_headers
{
	// Kernel response headers
	PROCESS_OK = 0,
	PROCESS_FAILURE = 1,
} console_headers;

typedef enum kernel_headers
{
	// From console
	NEW_PROCESS = 0,
	// From CPU
	IO_CALL = 1,
	EXIT_CALL = 2,
	INTERRUPT_DISPATCH = 3,
	// From memory
	PROCESS_MEMORY_READY = 4,
	PROCESS_EXIT_READY = 5,
	PROCESS_SUSPENSION_READY = 6,
} kernel_headers;

typedef enum cpu_headers
{
	// From kernel
	PCB_TO_CPU = 0,
	INTERRUPT = 1,
	// From memory
	VALUE_TO_CPU = 2,
	TABLE_INFO_TO_CPU = 3, // Handshake
	TABLE2_TO_CPU = 4,
	FRAME_TO_CPU = 5,
} cpu_headers;

typedef enum memory_headers
{
	// From kernel
	PROCESS_NEW = 0,
	PROCESS_SUSPEND = 6,
	PROCESS_EXIT = 7,
	// From CPU
	MEM_HANDSHAKE = 1,
	LVL1_TABLE = 2,
	LVL2_TABLE = 3,
	READ_CALL = 4,
	WRITE_CALL = 5,
} memory_headers;

#endif /* SOCKET_HEADERS_H_ */
