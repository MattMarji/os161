#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <array.h>
#include <synch.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

//We initialize our coremap and necessary variables here.
u_int32_t coremap_initialized;
int num_frames; //MapLen
int coremap_init_complete = 0; //flag to let the OS know we are ready to use the coremap.
struct lock *vm_lock;
struct page* coremap; //use the functions in array.c to create a dynamic array. This just makes life easier.
int i,j; //store temps just in case.
int memStartAddress;

//NOTE: ALL DEFINITIONS FOR COREMAP ARE IN kern\include\vm.h
void
vm_bootstrap(void)
{

//STEP 1: Create and initialize any necessary variables here.
		coremap_init_complete = 0;
		vm_lock = lock_create("vm_lock");
		lock_acquire(vm_lock);
		
		u_int32_t high, low,ramsize;
		int coremapSize; //Total size of coremap...
		
//STEP 2: Get the available ram and frames the hacked-way. Setup coremap.
		ram_getsize_hacked(&low,&high); //returns available memory.
		ramsize = high-low;
		
		kprintf("Ram Available: %u . HIGH: %u  LOW: %u \n", ramsize, high, low);
		
		num_frames = (ramsize/ (PAGE_SIZE + sizeof(struct page)));
		
		kprintf("NUM FRAMES AVAILABLE: %d \n", num_frames);
		//num_frames--; //array goes from 0 to num_frames-1
		//now that we have the number of frames, we know how many frames we can allocate in our array!
		
		coremap = kmalloc(num_frames*sizeof(struct page));
		
		//Keep track of how much memory the coremap is using
		coremapSize = num_frames*sizeof(struct page);
		
		if ((coremapSize % PAGE_SIZE) != 0){
		
			memStartAddress = low + coremapSize + (PAGE_SIZE - (coremapSize % PAGE_SIZE)); //Pad if necessary.
		
		}
		
		assert (memStartAddress % PAGE_SIZE == 0);
		
		//set initial values for the array.
		for (j = 0; j<num_frames; j++){
		
		coremap[j].frame_start_addr = memStartAddress + (j*PAGE_SIZE);
		coremap[j].allocation_length = 0;
		coremap[j].allocated_flag = 0;
		coremap[j].user_kernel_flag = 0;
		coremap[j].shared_counter = 0;
		}
		
		coremap_init_complete = 1; //coremap setup.
		
		lock_release(vm_lock);
		
		kprintf("LEAVING VM_BOOTSTRAP \n");

		}
		

static
paddr_t
getppages(unsigned long npages)
{
	int spl;
	paddr_t addr;
	//spl = splhigh();
	
	//kprintf("IM IN GETPPAGES AND THE VAL FOR COREMAP_INIT IS %d \n", coremap_init_complete);
	
	if(coremap_init_complete ==1){

	
	//kprintf("I'M GETTING PAGES USING MY FUCKING COREMAP \n");
	lock_acquire(vm_lock);
	i = 0;
	j = 0;
	
	if (npages == 1){ //we need to allocate only 1 page
			while ( i < num_frames){
					if (coremap[i].allocated_flag){
						kprintf(" SORRY BRO, THAT FRAME IS TAKEN. \n");
						i = i + coremap[i].allocation_length;
						j = 0;
					}
					else{
						kprintf(" CONGRATS BRO, WE FOUND A FRAME FOR YOU. \n");
						//We can allocate this frame for them.
						coremap[i].allocated_flag = 1; //now allocated.
						coremap[i].allocation_length = 1;
						coremap[i].shared_counter = 0; //FIGURE THIS OUT!!!!
						
						kprintf(" THE START ADDR OF THIS FRAME IS %x \n", coremap[i].frame_start_addr);
						
						lock_release(vm_lock);
						return coremap[i].frame_start_addr;
					}
			}
	
		}
		
	else{
			while ((i+npages) < num_frames && j < npages){
				
				if(coremap[i].allocated_flag){
					i = i + coremap[i].allocation_length;
					j = 0;
				}
				
				else{
					
					if (coremap[i+j].allocated_flag){
						i = i+j;
						j = 0;
					}
					
					else{
						j++;
					}
				
				}
			
			}
			
			if (j != npages){
			
			kprintf(" UH OH, THERE IS NOT ENOUGH CONTIGUOUS MEMORY!! \n");
			lock_release(vm_lock);
			return ENOMEM;
			}
			
			else{
			
			kprintf(" CONGRATS BRO, WE FOUND MEM FOR YOU. \n");
			
			int temp = i;
			
			coremap[i].allocation_length = npages;
				while( i < j){
					
					//FIGURE OUT SHIT ABOUT U/K OWNED FRAME.
					coremap[i].allocated_flag = 1;
					coremap[i].shared_counter = 0;
					i++;
				
				}
			
				lock_release(vm_lock);
				return coremap[temp].frame_start_addr;
			}
	}
	
	kprintf ("WHY YOU HERE ... THATS IMPOSSIBLE. \n");
	lock_release(vm_lock);
	return -1;
}
	
	else{
	
	kprintf("GOOD BOY! \n");
	addr = ram_stealmem(npages);
	}
	
	//splx(spl);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr)
{
	paddr_t new_addr; //translate v_addr to p_addr.
	new_addr = KVADDR_TO_PADDR(addr);
	
	lock_acquire(vm_lock);
	i = 0;
	j = 0;
	
	int coremapIndex = (new_addr - memStartAddress)/PAGE_SIZE; //this will give us the number of frames that need to be released.
	
	//Check for possible errors.
	if (coremapIndex > num_frames){
	
		kprintf("Yeah, we fucked up ... n > num_frames somehow \n");
		lock_release(vm_lock);
		//return -1;
	}
	
	else if (coremapIndex < num_frames && coremapIndex + coremap[coremapIndex].allocation_length > num_frames){
		
		kprintf("MORE FUCKERY .. \n");
		lock_release(vm_lock);
		//return -1;
	}
	
	else{
		
		//if the flag is allocated, and the frame has an allocated length, we must deallocate
		if (coremap[coremapIndex].allocated_flag && coremap[coremapIndex+i].allocation_length != 0){
		
			for (i = 0; i < coremap[coremapIndex].allocation_length; i++){
			
					//CHECK IF SHARED OR NOT.
					if(coremap[coremapIndex+i].shared_counter == 0){
						kprintf("Unallocating non-shared frame \n");
						//possibly unallocate other shit.
						coremap[coremapIndex+i].allocated_flag = 0;
					}
					
					//We are sharing this frame
					else{
						kprintf("Freeing shared frame \n");
						//WE CANNOT UNALLOCATE THE FRAME SINCE SOMEONE ELSE IS USING IT!!!!
						coremap[coremapIndex+i].shared_counter--;
						//possibly do other shit here?!
					}
			
			}
			coremap[coremapIndex].allocation_length = 0;
			lock_release(vm_lock);
		}
			
	}
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	assert(as->as_vbase1 != 0);
	assert(as->as_pbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_pbase2 != 0);
	assert(as->as_npages2 != 0);
	assert(as->as_stackpbase != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		splx(spl);
		return EFAULT;
	}

	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
}

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	return as;
}

void
as_destroy(struct addrspace *as)
{
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	assert(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	assert(new->as_pbase1 != 0);
	assert(new->as_pbase2 != 0);
	assert(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
}
