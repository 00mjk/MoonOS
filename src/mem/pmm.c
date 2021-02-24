#include "pmm.h"

//Start and end addresses of the kernel specified by the linker and needed to mark that memory as reserved / in use
extern volatile const u32int kernel_start;
extern volatile const u32int kernel_end;

static u32int bitmap[BITMAP_SZ];

/*
    1. Mark the whole memory area as used/reserved (1)
    2. Ask the BIOS / GRUB which memory area's are available and mark those as free
    3. The BIOS does not know about the kernel or where it resides in memory so we must mark this memory area as used
*/

static struct multiboot_mmap *mmap_start;
static struct multiboot_mmap *mmap_end;

void pmm_init(struct multiboot *ptr)
{
    debug("pmm_init: Initialising pmm...\n");
    mmap_start = ptr->mmap_addr;
    mmap_end = (void*) ((u32int*)ptr->mmap_addr + ptr->mmap_length);

    while (mmap_start < mmap_end)
    {
        if (mmap_start->type == PMM_FREE)
        {
            //Here we have the start and end address of the memory map provided by GRUB
            u32int *addr = mmap_start->base;
            u32int *addr_end = addr + mmap_start->length;

            while (addr < addr_end) {
                pmm_free((void*)addr); //Set's a bit as free (0 i think.)
                addr += 0x1000; //Page align
            }
        }
        mmap_start++;
    }
    debug("pmm_init: Marking kernel memory as reserved ( %x -> %x )\n", (u32int)&kernel_start, (u32int)&kernel_end);
}

//Need to set a bit or something. Read up somemore and look at other projects for help
void pmm_mark_as_used()
{

}

void pmm_free(void *memory)
{
    
}