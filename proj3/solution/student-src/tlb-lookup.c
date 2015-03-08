#include <stdlib.h>
#include <stdio.h>
#include "tlb.h"
#include "pagetable.h"
#include "global.h" /* for tlb_size */
#include "statistics.h"
#include "page-splitting.h"

/*******************************************************************************
 * Looks up an address in the TLB. If no entry is found, attempts to access the
 * current page table via cpu_pagetable_lookup().
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t tlb_lookup(vpn_t vpn, int write) {
    printf("###TLB lookup called\n");
 
    
    pfn_t pfn;
    
    /*
     * FIX ME : Step 6
     */
    int i;
    tlbe_t *temp = NULL;
    for (i = 0; i < tlb_size; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn) {
            count_tlbhits++;
            temp = tlb + i;
            break;
        }
    }
    
    /*
     * Search the TLB for the given VPN. Make sure to increment count_tlbhits if
     * it was a hit!
     */
    
    /* If it does not exist (it was not a hit), call the page table reader */
    if (temp == NULL) {
        pfn = pagetable_lookup(vpn, write);
    } else {
        pfn = temp->pfn;
    }
    
    
    /*
     * Replace an entry in the TLB if we missed. Pick invalid entries first,
     * then do a clock-sweep to find a victim.
     */
    
    if (temp == NULL) {
        for (i = 0; i < tlb_size; i++) {
            if (!tlb[i].valid) {
                temp = tlb + i;
                temp->pfn = pfn;
                temp->vpn = vpn;
                temp->valid = 1;
                break;
            }
        }
        
        if (temp == NULL) {
            for (i = 0; i < tlb_size; i++) {
                if (!tlb[i].used) {
                    temp = tlb + i;
                    temp->pfn = pfn;
                    temp->vpn = vpn;
                    temp->valid = 1;
                    break;
                } else {
                    tlb[i].used = 0;
                }
                
                if (i == tlb_size) {
                    i = 0;
                }
            }
        }
    }
    
    
    
    /*
     * Perform TLB house keeping. This means marking the found TLB entry as
     * accessed and if we had a write, dirty. We also need to update the page
     * table in memory with the same data.
     *
     * We'll assume that this write is scheduled and the CPU doesn't actually
     * have to wait for it to finish (there wouldn't be much point to a TLB if
     * we didn't!).
     */
    
    if (write) {
        temp->dirty = write;
        //current_pagetable[vpn].dirty = write;
    }
    
    temp->used = 1;
    //current_pagetable[vpn].used = 1;
    printf("### pfn is :%d\n", pfn);
    printf("### vpn should be: %d\n", vpn);
    return pfn;
}

