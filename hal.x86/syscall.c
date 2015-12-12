#include <syscall.h>
#include <multitasking.h>
#include <debug.h>
#include <video.h>
#include <message.h>
#include <vmm.h>
#include <idt.h>
#include <elf.h>
#include <kmessage.h>

// TODO: Syscalls einrichten
struct cpu* Syscall(struct cpu* cpu_old)
{
	switch(cpu_old->eax)
	{
	// putch
	case 1: 
		kputch((char) cpu_old->ebx);
		DebugPrintf("%c", (char) cpu_old->ebx);
		break;
	// exit
	case 2:
		DebugPrintf("[ SYSCALL ] Process %d wants to exit with code %d\n", current_process->pid, cpu_old->ebx);
		cpu_old = KillCurrentProcess();
		
		//DebugPrintf("Current process is now: %d %x\n", current_process->pid, cpu_old);
		break;

	// send_message
	case 3: {
			message_t* msg = (message_t*) cpu_old->ebx;
			process_t* proc = GetProcessByPid(msg->receiver);
			cpu_old->eax = ksend_message(current_process, proc, msg);
		}		
		break;

	// receive_message
	case 4: {
			message_t* msg = (message_t*) cpu_old->ebx;
			cpu_old->eax = kreceive_message(current_process, msg, cpu_old->ecx);
		}
		break;
	
	// sbrk
	case 5: {
			if(cpu_old->ebx <= 0)
			{
				DebugLog("[ SYSCALL ] Can't sbrk process!");
				cpu_old->eax = -1;
				break;
			}

			// FIXME: Limit fuer sbrk einfuehren, sonst kann jeder Prozess alles an Speicher haben!
			// FIXME: Auf 4k boundary prüfen!
			vmm_alloc(current_process->context, USERSPACE_PAGEPOOL + current_process->sbrk_state , cpu_old->ebx);
			cpu_old->eax = USERSPACE_PAGEPOOL + current_process->sbrk_state;			

			current_process->sbrk_state += cpu_old->ebx;
			
			// DebugLog("[ SYSCALL ] Got memory for process!");
		}
		break;
		
	// register_irq
	case 6:
		if(current_process->uid != ROOT_UID)
		{
			DebugPrintf("[ SYSCALL ] Process with UID %d tried to request an IRQ handler! Access denied!\n", current_process->uid);
			break;
		}
		
		registerHandlerProcess(cpu_old->ebx, current_process->pid);
		break;
	case 7:
		if(current_process->uid != ROOT_UID)
		{
			DebugPrintf("[ SYSCALL ] Process with UID %d tried to request the deletion of an IRQ handler! Access denied!\n", current_process->uid);
			break;
		}
		
		resetHandlerProcess(cpu_old->ebx, current_process->pid);
		break;
	
	// set_timer
	case 8:
		// DebugPrintf("Setting timer for process %d to %d\n", current_process->pid, cpu_old->ebx);
		current_process->sleep_timer = getTickCount() + cpu_old->ebx;
		current_process->status = PROCESS_SLEEPING;
		cpu_old = Schedule(cpu_old);
		break;
	
	// launch_elf_program
	case 9: {
		vmm_context_t* context = CreateUsermodeContext(0);
		function_t entry = ParseElf(cpu_old->ebx, context);
		
		if(entry == NULL)
		{
			DebugLog("[ SYSCALL ] Could not load ELF-Image!");
			break;
		}
		
		CreateUserProcess(entry, context);
	}
	break;
		
	default: DebugPrintf("[ SYSCALL ] Unknown syscall: 0x%x\n", cpu_old->eax);
	}
	
	return cpu_old;
}
