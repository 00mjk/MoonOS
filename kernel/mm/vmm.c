#include "vmm.h"
#include "libk/kassert.h"
#include "../amd64/validity.h"
#include "../drivers/gfx/gfx.h"
#include "../int/gdt.h"
#include "../int/idt.h"
#include "../util/ptr.h"
#include <stdbool.h>

struct page_directory *vmm_pml4;

//Level4 page table aka the contents of CR3
static uint64_t lv4_pg;

static struct page_directory *vmm_get_pml(
    struct page_directory *entry,
    size_t level, int flags);
static uint64_t vmm_get_lv4();

struct page_directory *get_pml4()
{
    return vmm_pml4;
}

void vmm_init()
{
    init_gdt();
    init_idt();

    assert((vmm_pml4 = pmm_alloc()) != NULL);

    printk("vmm", "pml4 resides at 0x%x\n", vmm_pml4);

    // My impl for page mapping
    for (uint64_t i = MM_BASE; i < MM_BASE + (PAGE_SIZE * 4); i += PAGE_SIZE) {
        vmm_map(vmm_pml4, i, i, 0x7);
    }

    // Pasted from limine for testing as a sanity check- Doesn't work, the hunt for bugs continues
    // for (uint64_t i = 0; i < 0x80000000; i += 0x200000) {
    //     vmm_map(vmm_pml4, 0xffffffff80000000 + i, i, 0x03);
    // }

    debug("Old PML4: %llx\n", cr_read(CR3)); // Bootloader pml4
    PAGE_LOAD_CR3(GENERIC_CAST(uint64_t, vmm_pml4));

    debug("New PML4: %llx\n", cr_read(CR3)); // Kernel pml4

    printk("vmm", "Initialised vmm\n");
}

//Return the address of the level4 page table
static uint64_t vmm_get_lv4()
{
    return lv4_pg;
}

static struct page_directory *vmm_get_pml(struct page_directory *entry, size_t level, int flags)
{
    if (entry->page_tables->present)
    {
        entry->page_tables[level].readwrite = (flags & FLAGS_RW) ? 1 : 0;
        entry->page_tables[level].supervisor = (flags & FLAGS_PRIV) ? 1 : 0;
        entry->page_tables[level].writethrough = (flags & FLAGS_WT) ? 1 : 0;
        return entry;
    }
    else
    {
        uint64_t addr = VOID_PTR_TO_64(pmm_alloc());
        entry->page_tables[level] = vmm_create_entry(addr, flags);
        return entry;
    }
}

void vmm_map(struct page_directory *pml4, size_t vaddr, size_t paddr, int flags)
{
    //TODO:
    //Performance tweak: Use an algorithm to save an address
    //which has been mapped already and check if the address is already mapped here
    page_info_t info = vmm_dissect_vaddr(vaddr);

    struct page_directory *pml3, *pml2, *pml1 = NULL;
    assert(((pml3 = vmm_get_pml(pml4, info.lv4, flags)) != NULL));
    assert(((pml2 = vmm_get_pml(pml3, info.lv3, flags)) != NULL));
    assert(((pml1 = vmm_get_pml(pml2, info.lv2, flags)) != NULL));
    pml1->page_tables[info.lv1] = vmm_create_entry(paddr + info.page_offset, flags);
}

//Same as vmm_map except that it doesn't check if a page has been mapped allowing you to edit the page flags
void vmm_remap(struct page_directory *pml4, size_t vaddr, size_t paddr, int flags)
{
    page_info_t info = vmm_dissect_vaddr(vaddr);

    struct page_directory *pml3, *pml2, *pml1 = NULL;
    assert(((pml3 = vmm_get_pml(pml4, info.lv4, flags)) != NULL));
    assert(((pml2 = vmm_get_pml(pml3, info.lv3, flags)) != NULL));
    assert(((pml1 = vmm_get_pml(pml2, info.lv2, flags)) != NULL));
    pml1->page_tables[info.lv1] = vmm_create_entry(paddr + info.page_offset, flags);
}

void vmm_unmap(struct page_directory *pml4, size_t vaddr, int flags)
{
    page_info_t info = vmm_dissect_vaddr(vaddr);
    struct page_directory *pml3, *pml2, *pml1 = NULL;
    assert(((pml3 = vmm_get_pml(pml4, info.lv4, flags)) != NULL));
    assert(((pml2 = vmm_get_pml(pml3, info.lv3, flags)) != NULL));
    assert(((pml1 = vmm_get_pml(pml2, info.lv2, flags)) != NULL));

    if (pml1->page_tables[info.lv1].address != PAGE_NOT_PRESENT)
    {
        printk("vmm", "Unmap address 0x%llx\n", pml1->page_tables[info.lv1].address);
        void *pte_address = VAR_TO_VOID_PTR(uint64_t, pml1->page_tables[info.lv1].address << 12);
        pmm_free(pte_address);
        pml1->page_tables[info.lv1] = vmm_purge_entry();
        TLB_FLUSH(vaddr);
    }
    else
    {
        //TODO: Add "Did you mean to access this address %llx" by finding the closest number
        //to the vaddr in the list of mapped pages (whether that be higher or lower, doesn't matter)
        printk("vmm", "Virtual address 0x%llx is already unmapped- Skipping TLB flush\n", vaddr);
    }
}

struct pte vmm_create_entry(uint64_t paddr, int flags)
{
    return (struct pte){
        .present = 1,
        .readwrite = (flags & FLAGS_RW) ? 1 : 0,
        .supervisor = (flags & FLAGS_PRIV) ? 1 : 0,
        .writethrough = (flags & FLAGS_WT) ? 1 : 0,
        .cache_disabled = 0,
        .accessed = 0,
        .dirty = 0,
        .ignore = 0,
        .global = 0,
        .avail = 0,
        .address = paddr >> 12
    };
}

struct pte vmm_purge_entry()
{
    struct pte purged_entry = vmm_create_entry(0x0, 0x0);
    purged_entry.present = 0x0;
    return purged_entry;
}

page_info_t vmm_dissect_vaddr(uint64_t virt_addr)
{
    page_info_t pg_info;
    const int bitmask = 0x1FF;

    pg_info.page_offset = virt_addr & 0xfff;
    virt_addr >>= 12;

    pg_info.lv1 = virt_addr & bitmask;
    virt_addr >>= 9;

    pg_info.lv2 = virt_addr & bitmask;
    virt_addr >>= 9;

    pg_info.lv3 = virt_addr & bitmask;
    virt_addr >>= 9;

    pg_info.lv4 = virt_addr & bitmask;

    return pg_info;
}

// static uint64_t *pml4 = 0;

// void vmm_init()
// {
//     // pml4 = cr_read(CR3);
//     init_gdt();
//     init_idt();

//     pml4 = pmm_alloc();
//     debug("%llx\n", pml4);
//     vmm_map(0xA00000000, 0xA00000000, 0x3);
//     PAGE_LOAD_CR3(pml4);
// }

// uint64_t *get(uint64_t *pml, uint64_t idx)
// {
//     if (pml[idx] & 1)
//     {
//         return (uint64_t *)(size_t)(pml[idx] & ~((uint64_t)0xfff));
//     }
//     else
//     {
//         uint64_t *addr = pmm_alloc();
//         pml[idx] = (size_t)addr | 0b111;
//         return addr;
//     }
// }

// void vmm_map(uint64_t vaddr, uint64_t paddr, int flags)
// {
//     page_info_t info = vmm_dissect_vaddr(vaddr);
//     uint64_t *pml3, *pml2, *pml1 = NULL;
//     pml3 = get(pml4, info.lv4);
//     pml2 = get(pml3, info.lv3);
//     pml1 = get(pml2, info.lv2);
//     pml1[info.lv1] = paddr + info.page_offset | flags;
//     debug("pml4 - 0x%llx\n", pml4);
//     debug("pml3 - 0x%llx\n", pml3);
//     debug("pml2 - 0x%llx\n", pml2);
//     debug("pml1 - 0x%llx\n", pml1);
// }