#include <vmm.h>
#include <types.h>
#include <debug.h>
#include <heap.h>

// Kernel Kontext
vmm_context_t* kernel_context = NULL;
vmm_context_t* current_context = NULL;

void activate_memory_context(vmm_context_t* context)
{
	 __asm volatile("mov %0, %%cr3" : : "r" (context->pagedir));
	 current_context = context;
}

vmm_context_t* create_context()
{
	vmm_context_t* context = pmm_alloc(PAGEPOOL_START, PAGEPOOL_END);
	assert(context != NULL);
	
	context->pagedir = pmm_alloc(PAGEPOOL_START, PAGEPOOL_END);
	assert(context->pagedir != NULL);
	
	int i = 0;
	for(; i < 1024; i++)
	{
		context->pagedir[i] = 0;
	}
	
	return context;
}

void destroy_context(vmm_context_t* context)
{
	//return;
	assert(context != NULL);
	assert(context != kernel_context);

	pmm_info();

	int i = 0;
	for(; i < 1024; i++)
	{
		if(context->pagedir[i] != 0 && context->pagedir[i] != kernel_context->pagedir[i])
		{
			uintptr_t* pagetable = (uintptr_t*) context->pagedir[i];
			
			for(int j = 0; j < 1024; j++)
				if(pagetable[j] && pagetable[j] > HEAP_END)
				{
					pmm_free((void*)(pagetable[j] & ~0xFFF));
				}
			
			//DebugPrintf("Freeing page: 0x%x\n", context->pagedir[i] & 0xFFFFFF00);
			pmm_free((void*)(context->pagedir[i] & 0xFFFFFF00));
		}
	}

	pmm_free(context->pagedir);
	pmm_free(context);

	pmm_info();
}

vmm_context_t* CreateUsermodeContext(int kernel_write)
{
	vmm_context_t* context = create_context();
	
	int i = 0;
	for(; i < 1024; i++)
	{
		context->pagedir[i] = kernel_context->pagedir[i];
	}
	
	// FIXME: Der gesamte Kernel ist schreibbar im usermode?
	//map_range(context, VM_KERNEL_START, VM_KERNEL_START,
	//		  VM_KERNEL_END-VM_KERNEL_START, kernel_write);
	
	// Heap mappen
	//map_range(context, HEAP_START, HEAP_START, HEAP_SIZE, 1);
	
	return context;
}

static void map_page(vmm_context_t* context, uintptr_t virt, uintptr_t phys, uint32_t userspace)
{
	uint32_t pageIndex = virt / 0x1000;
	uint32_t pdIndex = pageIndex / 1024;
	uint32_t ptIndex = pageIndex % 1024;
	
	uint32_t* pageTable;
	int i;
	
	uint32_t flags = 0;
	
	if(userspace)
		flags = VMM_USED | VMM_WRITE | VMM_USERSPACE;
	else
		flags = VMM_USED | VMM_WRITE;

	// Wir brauchen 4k-Alignment
	assert(!((virt & 0xFFF) || (phys & 0xFFF)));
		
	// Page Table heraussuchen bzw. anlegen
	if (context->pagedir[pdIndex] & 0x1)
	{
		// Page Table ist schon vorhanden
		pageTable = (uint32_t*) (context->pagedir[pdIndex] & ~0xFFF);
	}
	else
	{
		// Neue Page Table muss angelegt werden
		pageTable = (uint32_t*) pmm_alloc(PAGEPOOL_START, PAGEPOOL_END);
		
		assert(pageTable != NULL);
		
		for (i = 0; i < 1024; i++)
		{
			pageTable[i] = 0;
		}
		
		context->pagedir[pdIndex] = (uint32_t) pageTable | flags;
	}
	
	// Neues Mapping in the Page Table eintragen
	pageTable[ptIndex] = phys | flags;
	__asm volatile("invlpg %0" : : "m" (*(char*)virt)); 
}

void map_range(vmm_context_t* context, uintptr_t start_virt, 
			   uintptr_t start_phys, uint32_t size, uint32_t userspace)
{
	int i;
	for(i = start_virt; i <= start_virt + size; i += PAGESIZE)
	{
		map_page(context, i, start_phys, userspace);
		start_phys += PAGESIZE;
	}
}

void vmm_alloc(vmm_context_t* context, uintptr_t vaddr, uint32_t sz)
{
	int i;
	uintptr_t end = vaddr + sz;
	
	for(i = vaddr; i < end; i += PAGESIZE)
	{
		uintptr_t phys_page = (uintptr_t) pmm_alloc_start(HEAP_END);
		assert(phys_page != 0);
		map_page(context, i, phys_page, 1);
	}
}

void InitVmm(struct multiboot_info* mb_info)
{
	DebugPrintf("[ VMM ] Kernel at 0x%x - 0x%x\n", VM_KERNEL_START, VM_KERNEL_END);
	DebugPrintf("[ VMM ] Pagepool at 0x%x - 0x%x\n", PAGEPOOL_START, PAGEPOOL_END);
	
	uint32_t cr0;
	kernel_context = create_context();
	
	// FIXME: Der gesamte Kernel ist schreibbar im usermode?
	map_range(kernel_context, VM_KERNEL_START, VM_KERNEL_START,
			  VM_KERNEL_END - VM_KERNEL_START, 1);
	
	map_range(kernel_context, PAGEPOOL_START, PAGEPOOL_START,
			  PAGEPOOL_END - PAGEPOOL_START, 1);
	
	// Heap mappen
	map_range(kernel_context, HEAP_START, HEAP_START, HEAP_SIZE, 1);
	
	// Kernel Speicherkontext aktivieren
	activate_memory_context(kernel_context);	
	
	// Paging einschalten
	__asm volatile("mov %%cr0, %0" : "=r" (cr0));
	cr0 |= (1 << 31);
	__asm volatile("mov %0, %%cr0" : : "r" (cr0)); 
	
	DebugLog("[ VMM ] Virtual Memory management succesfully initialized.");
}
