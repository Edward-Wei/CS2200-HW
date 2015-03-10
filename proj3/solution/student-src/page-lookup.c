#include "swapfile.h"
#include "statistics.h"
#include "pagetable.h"
#include <stdio.h>

/*******************************************************************************
 * Looks up an address in the current page table. If the entry for the given
 * page is not valid, increments count_pagefaults and traps to the OS.
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t pagetable_lookup(vpn_t vpn, int write) {
    printf("###Pagetable lookup called\n");
    //pfn_t pfn = ;
    
    pte_t *pte = &(current_pagetable[vpn]);
    
    if (!pte->valid) {
        count_pagefaults++;
        printf("Pagefault hit");
        pte->pfn = pagefault_handler(vpn, write);
        pte = &(current_pagetable[vpn]);
    }
    
    //pte->valid = 1;
    pte->used = 1;
    if (write) {
        pte->dirty = write;
    }

    return pte->pfn;
}
