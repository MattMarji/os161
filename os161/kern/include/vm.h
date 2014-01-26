#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/



/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);

/* ADDED HELPER FUNCTION TO ALLOCATE PAGES */
paddr_t allocate_frame(unsigned long n);

/* ADDED HELPER FUNCTION TO CREATE PAGE TABLE */
pgtbl_entry** create_pgtbl_helper();

/* ADDED HELPER FUNCTION TO CREATE PAGE TABLE ENTRY */
int create_PTE_helper(pgtbl_entry ***pg_tbl, vaddr_t L1_index, vaddr_t L2_index);

/* ADDED HELPER FUNCTION TO UPDATE THE TLB! */
void update_TLB_helper(pgtbl_entry ***pg_tbl, vaddr_t faultaddress, vaddr_t L1_index, vaddr_t L2_index);

//Define what a page table entry is, in our page table.
struct pgtbl_entry{

	int valid_flag;
	int read_write_flag;
	paddr_t p_address;
	pgtbl_entry** L2_TABLE;
}

//Define what a page is in our coremap.
struct page{
	paddr_t frame_start_addr;
	u_int32_t allocation_length;

	//We need flags
	u_int32_t allocated_flag; //keep track of physical memory and see if it is available or not. (0 = not alloc, 1 = alloc)
	u_int32_t user_kernel_flag; // keep track if frame is for user/kernel (0 = flag not set, 1 = user, 2 = kern)
	u_int32_t shared_counter; //keep count of if this frame is shared by multiple pages ( 0 = flag not set, 1 = one user, 2 = two users ...etc.)
};

#endif /* _VM_H_ */
