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
int coremapStartAddress;

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
		
		////kprintf("Ram Available: %u . HIGH: %u  LOW: %u \n", ramsize, high, low);
		
		num_frames = (ramsize/ (PAGE_SIZE + sizeof(struct page)));
		
		//kprintf("NUM FRAMES AVAILABLE: %d \n", num_frames);
		//num_frames--; //array goes from 0 to num_frames-1
		//now that we have the number of frames, we know how many frames we can allocate in our array!
		
		coremap = kmalloc(num_frames*sizeof(struct page));
		
		//Keep track of how much memory the coremap is using
		coremapSize = num_frames*sizeof(struct page);
		
		if ((coremapSize % PAGE_SIZE) != 0){
		
			coremapStartAddress = low + coremapSize + (PAGE_SIZE - (coremapSize % PAGE_SIZE)); //Pad if necessary.
		
		}
		
		//ensure we are page aligned.
		assert (coremapStartAddress % PAGE_SIZE == 0); 
		
		//set initial values for the array.
		for (j = 0; j<num_frames; j++){
		
		coremap[j].frame_start_addr = coremapStartAddress + (j*PAGE_SIZE);
		coremap[j].allocation_length = 0;
		coremap[j].allocated_flag = 0;
		coremap[j].user_kernel_flag = 0;
		coremap[j].shared_counter = 0;
		}
		
		coremap_init_complete = 1; //coremap setup.
		
		lock_release(vm_lock);
		
		//kprintf("LEAVING VM_BOOTSTRAP \n");

		}
		

static
paddr_t
getppages(unsigned long npages)
{
	int spl;
	paddr_t addr;
	spl = splhigh();
	
	
	////kprintf("IM IN GETPPAGES AND THE VAL FOR COREMAP_INIT IS %d \n", coremap_init_complete);
	
	if(coremap_init_complete ==1){
	//kprintf(" GOING INTO ALLOCATE_FRAME \n");
	addr = allocate_frame(npages);
	}
	
	else{
	
	//kprintf("GOOD BOY! \n");
	addr = ram_stealmem(npages);
	}
	
	splx(spl);
	return addr;
}

paddr_t allocate_frame(unsigned long npages){

lock_acquire(vm_lock);
	//kprintf("I'M GETTING PAGES USING MY COREMAP \n");
	
	i = 0;
	j = 0;
	
	if (npages == 1){ //we need to allocate only 1 page -- not necessary but do this just incase
			while ( i < num_frames){
					if (coremap[i].allocated_flag){
						//kprintf(" SORRY BRO, THAT FRAME IS TAKEN. \n");
						i = i + coremap[i].allocation_length;
						j = 0;
					}
					else{
						//kprintf(" CONGRATS BRO, WE FOUND A FRAME FOR YOU. \n");
						//We can allocate this frame for them.
						coremap[i].allocated_flag = 1; //now allocated.
						coremap[i].allocation_length = 1;
						coremap[i].shared_counter = 0; //FIGURE THIS OUT!!!!
						
						//kprintf(" THE START ADDR OF THIS FRAME IS %x \n", coremap[i].frame_start_addr);
						
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
			
			//kprintf(" UH OH, THERE IS NOT ENOUGH CONTIGUOUS MEMORY!! \n");
			lock_release(vm_lock);
			return ENOMEM;
			}
			
			else{
			
			//kprintf(" CONGRATS BRO, WE FOUND MEM FOR YOU. \n");
			
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
	
	//kprintf ("WHY YOU HERE ... THATS IMPOSSIBLE. \n");
	lock_release(vm_lock);
	return -1;

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
	
	int coremapIndex = (new_addr - coremapStartAddress)/PAGE_SIZE; //this will give us the number of frames that need to be released.
	
	//Check for possible errors.
	if (coremapIndex > num_frames){
	
		lock_release(vm_lock);
		//return -1;
	}
	
	else if (coremapIndex < num_frames && coremapIndex + coremap[coremapIndex].allocation_length > num_frames){
		
		//kprintf("...what is going on here .. \n");
		lock_release(vm_lock);
		//return -1;
	}
	
	else{
		
		//if the flag is allocated, and the frame has an allocated length, we must deallocate
		if (coremap[coremapIndex].allocated_flag && coremap[coremapIndex+i].allocation_length != 0){
		
			for (i = 0; i < coremap[coremapIndex].allocation_length; i++){
			
					//CHECK IF SHARED OR NOT.
					if(coremap[coremapIndex+i].shared_counter == 0){
						//kprintf("Unallocating non-shared frame \n");
						//possibly unallocate other shit.
						coremap[coremapIndex+i].allocated_flag = 0;
					}
					
					//We are sharing this frame
					else{
						//kprintf("Freeing shared frame \n");
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
	//kprintf("Entering VM_FAULT \n");
	
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	
	//paddr_t paddr; //Since DUMBVM is not in use, we now handle the mapping of VADDR -> PADDR
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;
	int check;
	vaddr_t L1_pgtbl, L2_pgtbl, pg_offset;
	pgtbl_entry *page_table;
	
	spl = splhigh();

	//Before we mess with the faultaddress (VADDR), lets strip the offset.
	pg_offset = faultaddress & 0x00000fff //The offset is 12 bits!
	
	//Strip the offset, only get the page number section.
	faultaddress &= PAGE_FRAME;

	//kprintf("We just stripped the offset from the faultaddr \n");

	//WE NEED TO TAKE CARE OF ALL POSSIBLE CASES EVENTUALLY!
	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
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
	assert((as->pg_table != NULL)); //Check that the page table is NULL at start.
	
	//LEAVE THE SAME AS DUMBVM!!
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - as->Region[2].numFrames * PAGE_SIZE; //IMPORTANT: We are currently starting the stackbase after we automatically allocate 12 PAGES. CHANGE for on-demand paging
	stacktop = USERSTACK;

	//NOTE, THIS STUFF WAS USED FOR THE OLD PADDR. We do not need it.
	/*
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
	*/
	/* make sure it's page-aligned */
	//assert((paddr & PAGE_FRAME)==paddr);

	
	
	//If we get passed here. It is NOT a memory access ERROR.
	//kprintf("Not a memory access error. Now lets prep to get the first-level page table ready! \n");
	
	page_table = as->page_table;
	
	
	//For a 2-Level Page Table, we are told the page number is 20 bits. Since we have two levels, each level is 10 bits.
	
	L1_pgtbl = faultaddress & 0xffc00000;
	L1_pgtbl = L1_pgtbl >> 22; //32-22 = highest 10 bits
	
	L2_pgtbl = faultaddress & 0x003ff000;
	L2_pgtbl = L2_pgtbl >> 12; //32-12 = middle 10 bits, and we zero out the high 10 bits.
	
	pgtbl_entry* temp = (pgtbl_entry*)(page_table[l1Index].p_address);
	(pgtbl_entry*)(((paddr_t)temp) &= PAGE_FRAME);

	//FIND THE REGION THE TLB_MISS WAS IN!!
	sector = findSector(faultaddress);
	
	//Now we have to realize that there are three possibilities if we get a miss/fault.
	
	//Option 1. If we get a miss, and the L2 PT exists, AND the pgtbl_entry is valid. All we have to do is UPDATE THE TLB.
	//Option 2. If we have a miss, and the L2 PT exits, BUT the pgtbl_entry does not exist. We create the pgtbl_entry, and UPDATE THE TLB.
	//Option 3. If we have a miss, and the L2 PT DNE, we create the L2 PT, create the pgtbl_entry, and UPDATE THE TLB.
	
	if (page_table[L1_pgtbl].p_address & TLBLO_VALID){
		
		//OPTION 1
		if (temp[L2_pgtbl].p_address & TLBLO_VALID){
			//kprintf("Level 2 Page Table Exists. Just going to update the TLB! \n");
			update_TLB_helper(&page_table, faultaddress, L1_pgtbl, L2_pgtbl);
			splx(spl);
			return 0;
		}
		
		//OPTION 2
		else{
			check = create_pgtbl_entry_helper(&page_table, L1_pgtbl, L2_pgtbl, faultaddress, sector);
			
			if (check == ENOMEM){
				//kprintf("Something is wrong here. We Could not create an L2 pgtbl_entry \n");
				return ENOMEM;
			}
			
			//kprintf("Created an L2 pgtbl_entry and now will update the TLB! \n");
			as->num_frames;//Increase num frames just in case
			update_TLB_helper(&page_table, L1_pgtbl, L2_pgtbl);
			splx(spl);
			return 0;
		}
	
	}
	else{
		//kprintf("NO L2 PAGE TABLE EXISTS! Create one A$AP ROCKY \n");
		page_table[L1_pgtbl].p_address=create_pgtbl_helper();
		
		//kprintf("NO pgtbl_entry EXISTS! Create one A$AP ROCKY \n");
		check = create_pgtbl_entry_helper(&page_table, L1_pgtbl, L2_pgtbl, faultaddress, sector);
		
		if (check == ENOMEM){
				//kprintf("Something is wrong here. We Could not create an L2 pgtbl_entry \n");
				return ENOMEM;
			}
			page_table[L1_pgtbl].p_address |= TLBLO_VALID;
			//kprintf("Created an L2 pgtbl_entry and now will update the TLB! \n");
			as->num_frames++; //Since we created a new page table. We have allocated a new frame!
			update_TLB_helper(&page_table, L1_pgtbl, L2_pgtbl);
			splx(spl);
			return 0;
		
	}
	
	//We should never get here...
	splx(spl);
	return EFAULT;

}

pgtbl_entry* create_pgtbl_helper(){

	//kprintf("Creating a Page Table. \n");
	int i = 0;
	
	//Create a pointer that we will return by reference.
	pgtbl_entry *temp_pgtbl = (pgtbl_entry*)kmalloc(1024*sizeof(struct pgtbl_entry)); //1024 = 2^10 entries.
	
	for (i=0; i<1024; i++){
		P[i].p_address = 0;
	}
	
	//kprintf("Page Table has been allocated. Returning now. \n");
	return temp_pgtbl;
}

int create_pgtbl_entry_helper(pgtbl_entry **pg_tbl, vaddr_t L1_index, vaddr_t L2_index, vaddr_t faultaddress, int sector){

	paddr_t allocated_frame = allocate_frame(1); //We need to allocate a frame for the pgtbl_entry.
	
	//we need to assign the address space the values for the code, data, stack, and heap regions.
	//NOTE: We still need to update our addrspace to accept the heap sector using sbrk!!!
	struct addrspace *temp_addrspace;
	temp_addrspace = curthread->t_vmspace;
	int current_index = 0;
	int number_of_pages = 0;

	if (allocated_frame == ENOMEM){
		
		//kprintf("WTF IS GOING ON WITH OUR COREMAP PAGE ALLOCATION? \n");
		return ENOMEM;
	}

	//DETERMIND THE CORRECT REGION!!
	//NOTE: IF REGION == 2 GROW THE STACK RIGHT AWAY!!
	
	if(sector == 0)
	{
		number_of_pages = temp_addrspace->Region[0].npages;

		for(current_index = 0; current_index < number_of_pages; current_index++ )
		{
			if(faultaddress >= (temp_addrspace->as_vbase1 + (current_index*PAGE_SIZE)) && faultaddress < (temp_addrspace->as_vbase1 + ((current_index+1) * PAGE_SIZE)))
			{
				break;
			}
		}
	}

	else if(sector == 1)
	{
		number_of_pages = temp_addrspace->Region[1].npages;
		for(current_index = 0; current_index < number_of_pages; current_index++ )
		{
			if(faultaddress >= (temp_addrspace->as_vbase2 + (current_index*PAGE_SIZE)) && faultaddress < (temp_addrspace->as_vbase2 + ((current_index+1) * PAGE_SIZE)))
			{
				break;
			}
		}
	}
	
	//LOAD ONE FULL PAGE RIGHT AWAY
   if (sector == 2)
	{
		temp_addrspace->as_stackpbase = allocatedFrame;
		temp_addrspace->Region[sector].numFrames++;
	}

	//NOW, IF WE HIT A CODE, DATA REGION, WE MUST LOAD IT FROM FILE!
	else if(temp_addrspace->Region[sector].fileSize >= ((current_index + 1)*PAGE_SIZE))
	{
		load_seg(temp_addrspace->my_vode, (temp_addrspace->Region[sector].fileOffset + ((current_index) * PAGE_SIZE)),
		PADDR_TO_KVADDR(allocatedFrame), PAGE_SIZE, PAGE_SIZE,
		temp_addrspace->Region[sector].executable);
		temp_addrspace->Region[sector].numFrames ++;
	}

	else if(temp_addrspace->Region[sector].fileSize <= ((current_index)*PAGE_SIZE)
			&& temp_addrspace->Region[sector].memSize > ((current_index)*PAGE_SIZE))
	{

		//ZERO OUT THE WHOLE PAGE
		bzero((void*)PADDR_TO_KVADDR(allocatedFrame), PAGE_SIZE);//ZERO A PAGE!!!!!

		temp_addrspace->Region[sector].numFrames ++;
	}

	else if(temp_addrspace->Region[sector].fileSize <= ((current_index + 1)*PAGE_SIZE)
			&& temp_addrspace->Region[sector].memSize > ((current_index)*PAGE_SIZE))
	{
			load_seg(temp_addrspace->my_vode, (temp_addrspace->Region[sector].fileOffset + ((current_index) * PAGE_SIZE)),
			PADDR_TO_KVADDR(allocatedFrame), PAGE_SIZE, (temp_addrspace->Region[sector].fileSize - ((current_index)*PAGE_SIZE)),
			temp_addrspace->Region[sector].executable);

			temp_addrspace->Region[sector].numFrames ++;
	}


	pgtbl_entry* temp = (pgtbl_entry*)((*pg_tbl)[L1_index].p_address);
	(pgtbl_entry*)(((paddr_t)temp) &= PAGE_FRAME);

	temp[L2_index].p_address = allocatedFrame;//Assign physical addr to pte
	temp[L2_index].p_address | = TLBLO_VALID; //SET THIS TO VALID! WE NOW HAVE A VALID page table entry

	if(temp_addrspace->Region[sector].writeable) //set this to writable if the sector says so.
	{
		temp[L2_index].p_address | = TLBLO_DIRTY; //setting to writeable
	}
	
	return 0;
}

void update_TLB_helper(pgtbl_entry **pg_tbl, vaddr_t faultaddress, vaddr_t L1_index, vaddr_t L2_index){
	
	paddr_t entryLo;
	int i=0;

	pgtbl_entry* temp =(pgtbl_entry*)((*pg_tbl)[L1_index].p_address);
	(pgtbl_entry*)(((paddr_t)temp) &= PAGE_FRAME);

	entryLo = temp[L2_index].p_address;
	entryLo &= TLBLO_PPAGE; //mask out the bottom 12 bits 
	entryLo |= TLBLO_VALID; //set the entry to valid
	//If we have created a new TLB entry we don't really need to do this check, but it
	//doesn't hurt us to have it here!
	if(temp[L2_index].p_address & TLBLO_DIRTY) //check if dirty
	{
		entryLo |= TLBLO_DIRTY; //its filthy.
	}

	TLB_Random(faultaddress, entryLo); //store in a random entry in the TLB.
}


int findSector(paddr_t faultaddress)
{
	struct addrspace *as;
	as = curthread->t_vmspace;
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	int sector;

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + (as->Region[0].npages * PAGE_SIZE);
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + (as->Region[1].npages * PAGE_SIZE);
	stackbase = USERSTACK - ((as->myRegions[2].numFrames) * PAGE_SIZE); 
	stacktop = USERSTACK;

	faultaddress &= PAGE_FRAME;

	//This is the stack region!!!
	if(faultaddress >= (stackbase - PAGE_SIZE) && faultaddress < stacktop)
		{
			sector = 2;
		}
	//This is the code region!!
	else if ((faultaddress >= vbase1 && faultaddress < vtop1))
	{
				sector = 0; 
	}

	//This is the data region!!
	else if ((faultaddress >= vbase2 && faultaddress < vtop2))
	{
		sector = 1; 
	}
	
	//We should never get here, but just in case!!!
	else if ((faultaddress >= stackbase && faultaddress < stacktop))
	{
		sector = 2; 

	}

	return region;
}

int load_demand(struct vnode *v, off_t offset, vaddr_t vaddr,
	     size_t memsize, size_t filesize,
	     int is_executable)
{
	struct uio u;
	int result;
	size_t fillamt;

	if (filesize > memsize) {
		//kprintf("ELF: warning: segment filesize > segment memsize\n");
		filesize = memsize;
	}

	DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n",
	      (unsigned long) filesize, (unsigned long) vaddr);

	u.uio_iovec.iov_kbase = (vaddr_t)vaddr;
	u.uio_iovec.iov_len = memsize;   // length of the memory space
	u.uio_resid = filesize;          // amount to actually read
	u.uio_offset = offset;
	u.uio_segflg = UIO_SYSSPACE;//is_executable ? UIO_USERISPACE : UIO_USERSPACE;
	u.uio_rw = UIO_READ; //FUCK WRITING YO! AIN'T LIKE I BE IN SCHOOL!
	u.uio_space = NULL;//curthread->t_vmspace;

	result = VOP_READ(v, &u);
	if (result) {
		return result;
	}

	if (u.uio_resid != 0) {
		/* short read; problem with executable? */
		kprintf("ELF: short read on segment - file truncated?\n");
		return ENOEXEC;
	}

	/* Fill the rest of the memory space (if any) with zeros */
	fillamt = memsize - filesize;
	if (fillamt > 0)
	{
		//DEBUG(DB_EXEC, "ELF: Zero-filling %lu more bytes\n",
		      //(unsigned long) fillamt);
		u.uio_offset += fillamt;//u.uio_resid += fillamt;
		/*result =*/ bzero((void*)(vaddr + filesize), fillamt);
				//uiomovezeros(fillamt, &u);
		//u.uio_resid = 0;
		fillamt = 0;
	}
	fillamt -= fillamt;
	fillamt = 0;
	fillamt = 0;
	fillamt = 0;
	fillamt = 0;
	fillamt = 0;




	return result;
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
	as->num_frames = 0;
	as->page_table = create_pgtbl_helper();
	as->my_vnode = NULL;
	
	int i = 0;
		for(i = 0; i < 4; i++)
	{
		if(i == 2) // NOTE THIS IS THE STACK!!
		{
			as->Region[i].readable = 4; //force the stack to be readable
			as->Region[i].writeable = 2; //force the stack to be writable
			as->Region[i].executable = 0; //force the stack to be NOT-EXEC
		}
		else
		{
			as->Region[i].readable = 0;
			as->Region[i].writeable = 0;
			as->Region[i].executable = 0;
		}
		
		as->Region[i].fileOffset = 0;
		as->Region[i].fileSize = 0;
		as->Region[i].memSize = 0;
		as->Region[i].numFrames = 0;
		as->Region[i].npages = 0;
		
		
	}
	return as;
}

void
as_destroy(struct addrspace *as)
{
	VOP_DECREF(as->my_vnode);
	
	int i=0;
	int j=0;
	
	//We will now loop through the first level page table and the second and clean up all the frames.
	//This will avoid memory leaks!
	for(i = 0; i < 1024; i++)
	{
		if(as->page_table[i].p_address & TLBLO_VALID)
		{

			pgtbl_entry* temp = (pgtbl_entry*)(as->page_table[i].p_address);
			(pgtbl_entry*)(((paddr_t)temp) &= PAGE_FRAME);


			for(j = 0; j < 1024; j++)
			{
				if(temp[j].p_address & TLBLO_VALID)
				{
					free_frame((temp[j].p_address)); //free the 2nd LEVEL PTE IF VALID!!
				}
			}
			free_frame(KVADDR_TO_PADDR(as->page_table[i].p_address)); //free the 2nd LEVEL PAGE TABLE
		}
	}
	free_frame((paddr_t)KVADDR_TO_PADDR((vaddr_t)as->page_table)); //free the 1st LEVEL PAGE TABLE
	
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
		 int readable, int writeable, int executable, int sector)
{
	size_t npages; 

	sector= sector -1; //NOTE, THE SECTOR IN LOAD_ELF STARTS AT 1, WE NEED TO DECREMENT BY ONE FOR OUR ALGO TO WORK.
	
	/* Align the sector. First, the base... */
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
		as->Region[sector].readable = readable;
		as->Region[sector].writeable = writeable;
		as->Region[sector].executable = executable;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		as->Region[sector].readable = readable;
		as->Region[sector].writeable = writeable;
		as->Region[sector].executable = executable;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	//kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

//THIS FUNCTION IS UNNCESSARY
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
	assert(temp_addrspace->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

//FOR USES THIS. GET MORE INFO ON THIS.
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	VOP_INCREF(old->my_node);
	
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	//MAKE A SHALLOW COPY!
	new->Region[0] = old->Region[0];
	new->Region[1] = old->Region[1];
	new->Region[2] = old->Region[2];
	
	//Create a copy to the same vnode for the same file.
	new->my_node = old->my_vnode;
	new->page_table = old->page_table;
	
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
		old->Region[2].numFrames*PAGE_SIZE);
	
	*ret = new;
	return 0;
}


