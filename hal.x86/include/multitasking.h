#ifndef MULTITASKING_H
#define MULTITASKING_H

#include "types.h"
#include "cpu.h"
#include "vmm.h"
#include "kmessage.h"

#define MESSAGE_BUFFER_SIZE 10

#define PROCESS_RUNNABLE 0
#define PROCESS_RUNNING 1
#define PROCESS_SLEEPING 2
#define PROCESS_PAUSED 3

#define ROOT_UID 0

struct MESSAGE_NODE
{
	message_t message;
	struct MESSAGE_NODE* next;
	struct MESSAGE_NODE* prev;
};

struct MESSAGE_LIST
{ 
	struct MESSAGE_NODE* first;
	struct MESSAGE_NODE* last;
	int num_messages;
};

typedef struct process_struct
{
	struct cpu* state;
	vmm_context_t* context;
	uint32_t pid;

	//message_t messages[MESSAGE_BUFFER_SIZE];
	struct MESSAGE_LIST messages;
	
	uint32_t sbrk_state;	

	uint8_t* stack;
	uint8_t* user_stack;
	
	uint32_t sleep_timer;
	uint8_t status;
	
	uint32_t uid;
	struct process_struct* next;
}process_t;

void InitMultitasking();
struct cpu* Schedule(struct cpu* old_cpu);
process_t* CreateUserProcess(void (*entry)(), vmm_context_t* context);
process_t* GetProcessByPid(uint32_t pid);

struct cpu* KillCurrentProcess();
void KillProcess(process_t* process);

extern process_t* current_process;

void ListProcesses();

#endif
